#include "CoastalSoundscape.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalUISessionComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"

UCoastalSoundscape::UCoastalSoundscape()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bTickEvenWhenPaused=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}
bool UCoastalSoundscape::WithinZone(FVector P,FVector C,float Radius,bool Retained)
{
    return !P.ContainsNaN() && !C.ContainsNaN() && FMath::IsFinite(Radius) && Radius>0
        && FMath::Abs(P.Z-C.Z)<=2500 && FVector::DistSquared2D(P,C)<=FMath::Square(Radius+(Retained?400.f:0.f));
}
FName UCoastalSoundscape::SelectScore(FVector Position) const
{
    for(const auto& Zone:Zones)
        if(Zone.Score==CurrentScore && WithinZone(Position,Zone.Center,Zone.Radius,true))return Zone.Score;
    for(const auto& Zone:Zones)
        if(WithinZone(Position,Zone.Center,Zone.Radius,false))return Zone.Score;
    return TEXT("coast");
}
UAudioComponent* UCoastalSoundscape::CreateVoice(USoundBase* Sound,USoundClass* Bus)
{
    auto* Voice=NewObject<UAudioComponent>(GetOwner(),NAME_None,RF_Transient);
    Voice->bAutoActivate=false; Voice->bAutoDestroy=false; Voice->bStopWhenOwnerDestroyed=true;
    Voice->bCanPlayMultipleInstances=false; Voice->bAllowSpatialization=false;
    Voice->bOverrideAttenuation=true; Voice->AttenuationOverrides.bAttenuate=false;
    Voice->AttenuationOverrides.bSpatialize=false; Voice->bSuppressSubtitles=true;
    Voice->SoundClassOverride=Bus; Voice->AudioDeviceID=Device.GetDeviceID();
    Voice->SetSound(Sound); Voice->RegisterComponent();
    if(!Voice->IsRegistered()){Voice->DestroyComponent();return nullptr;}
    return Voice;
}
void UCoastalSoundscape::DestroyVoice(TObjectPtr<UAudioComponent>& Voice)
{
    auto* Old=Voice.Get(); Voice=nullptr;
    if(IsValid(Old)){Old->Stop();Old->DestroyComponent();}
}
void UCoastalSoundscape::ResetVoices()
{
    DestroyVoice(Music); DestroyVoice(Thunder); CurrentScore=NAME_None; MusicGain=0;
    ThunderWait=20; ThunderAge=0; CoverWait=0; CoverTarget=1; ShelterGain=1;
}
void UCoastalSoundscape::Release()
{
    bReady=false; bReleased=true; ResetVoices();
    if(IsValid(Options))Options->OnRoutingReleased.Remove(ReleaseHandle);
    if(IsValid(Menus))RemoveTickPrerequisiteComponent(Menus);
    ReleaseHandle.Reset(); Menus=nullptr; Options=nullptr; Pawn=nullptr;
    Clips.Reset(); MusicBus=nullptr; ThunderBus=nullptr; Device.Reset();
}
void UCoastalSoundscape::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    if(!bReady)return;
    auto* PC=Cast<APlayerController>(GetOwner());
    const auto CurrentDevice=GetWorld()->GetAudioDevice();
    if(!IsValid(Menus) || !Menus->IsInitialized() || !Menus->IsRegistered() || !Menus->IsComponentTickEnabled()
        || !IsValid(Options) || !Options->IsAudioReady() || !IsValid(Pawn) || !PC || PC->GetPawn()!=Pawn
        || !CurrentDevice.IsValid() || CurrentDevice.GetDeviceID()!=Device.GetDeviceID()
        || Options->GetAmbienceClassForPlayback()!=MusicBus || Options->GetEffectsClassForPlayback()!=ThunderBus)
    {Release();return;}
    const auto Context=Menus->GetAudioPlaybackContext();
    if(Context.epoch!=Epoch){ResetVoices();Epoch=Context.epoch;}
    if(!Context.active || !Epoch){ResetVoices();return;}
    const bool Paused=Context.interrupted || !Context.ambienceAllowed;
    if(IsValid(Music))Music->SetPaused(Paused);
    if(IsValid(Thunder))Thunder->SetPaused(Paused);
    if(Paused || !FMath::IsFinite(Delta) || Delta<=0)return;
    const FName Wanted=SelectScore(Pawn->GetActorLocation());
    // Sequential two-second fades keep the score bounded to exactly one voice, even during rapid travel.
    MusicGain=FMath::FInterpConstantTo(MusicGain,CurrentScore==Wanted?1.f:0.f,Delta,.5f);
    if(CurrentScore!=Wanted && MusicGain<=0)
    {
        DestroyVoice(Music); CurrentScore=Wanted;
        Music=CreateVoice(Clips.FindChecked(Wanted),MusicBus);
        if(Music){Music->SetVolumeMultiplier(0);Music->Play();++MusicSubmissions;}
    }
    if(Music)Music->SetVolumeMultiplier(MusicGain);
    // Actual collision cover above the pawn, sampled twice a second; no weather-state or save mutation.
    CoverWait-=Delta;
    if(CoverWait<=0)
    {
        CoverWait=.5f; FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(CoastalThunderCover),false,Pawn);
        const FVector Start=Pawn->GetActorLocation()+FVector(0,0,100);
        CoverTarget=GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+FVector(0,0,1600),ECC_Visibility,Params)?.25f:1.f;
    }
    ShelterGain=FMath::FInterpConstantTo(ShelterGain,CoverTarget,Delta,.5f);
    if(Thunder)
    {
        Thunder->SetVolumeMultiplier(ShelterGain);
        Thunder->SetLowPassFilterEnabled(true);
        Thunder->SetLowPassFilterFrequency(FMath::Lerp(1800.f,18000.f,(ShelterGain-.25f)/.75f));
        ThunderAge+=Delta;
        if(ThunderAge>=Thunder->Sound->GetDuration()+.25f)DestroyVoice(Thunder);
    }
    ThunderWait-=Delta;
    if(ThunderWait<=0 && !Thunder)
    {
        ThunderWait=FMath::FRandRange(55.f,85.f); ThunderAge=0;
        Thunder=CreateVoice(Clips.FindChecked(FMath::RandBool()?FName(TEXT("thunder_far")):FName(TEXT("thunder_soft"))),ThunderBus);
        if(Thunder)
        {
            Thunder->SetVolumeMultiplier(ShelterGain); Thunder->SetLowPassFilterEnabled(true);
            Thunder->SetLowPassFilterFrequency(FMath::Lerp(1800.f,18000.f,(ShelterGain-.25f)/.75f));
            Thunder->Play();++ThunderSubmissions;
        }
    }
}
void UCoastalSoundscape::OnUnregister(){Release();Super::OnUnregister();}
void UCoastalSoundscape::EndPlay(const EEndPlayReason::Type Reason){Release();Super::EndPlay(Reason);}
