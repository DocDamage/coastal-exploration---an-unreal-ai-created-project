#include "CoastalActionAudio.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalUISessionComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "CoreGlobals.h"

UCoastalActionAudio::UCoastalActionAudio()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bTickEvenWhenPaused=true;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}
bool UCoastalActionAudio::Initialize(UCoastalInteractionBridge* Bridge,UCoastalAudioOptionsComponent* Routing,
    UCoastalUISessionComponent* UI,const TMap<FName,USoundBase*>& Sounds)
{
    auto* PC=Cast<APlayerController>(GetOwner());
    if(Source || !IsRegistered() || !PC || !PC->IsLocalController() || !IsValid(Bridge)
        || Bridge->GetOwner()!=PC->GetPawn() || !IsValid(Routing) || Routing->GetOwner()!=PC
        || !Routing->IsAudioReady() || !IsValid(UI) || UI->GetOwner()!=PC || !UI->IsInitialized())return false;
    for(const TCHAR* Id:{TEXT("select"),TEXT("confirm"),TEXT("cancel"),TEXT("error"),TEXT("pickup"),
        TEXT("door_open"),TEXT("door_close"),TEXT("storage"),TEXT("paper"),TEXT("journal")})
    {
        auto* const* Found=Sounds.Find(FName(Id));
        auto* Sound=Found?*Found:nullptr;
        if(!IsValid(Sound) || !Sound->IsPlayable() || Sound->IsLooping() || !Sound->IsPlayWhenSilent()
            || !FMath::IsFinite(Sound->GetDuration()) || Sound->GetDuration()<=0 || Sound->GetDuration()>10)return false;
    }
    Source=Bridge; Options=Routing; Menus=UI;
    for(const auto& Pair:Sounds)Clips.Add(Pair.Key,Pair.Value);
    ActionHandle=Source->OnActionFeedback.AddUObject(this,&UCoastalActionAudio::Action);
    MenuHandle=Menus->OnMenuFeedback.AddUObject(this,&UCoastalActionAudio::Menu);
    ReleaseHandle=Options->OnRoutingReleased.AddUObject(this,&UCoastalActionAudio::Stop);
    return true;
}
FName UCoastalActionAudio::ResultCue(ECoastalActionResult Result,FName SuccessCue)
{
    if(Result==ECoastalActionResult::Applied)return SuccessCue;
    // Idempotent repeats, stale focus and held/blocked input do not produce fresh success or errors.
    switch(Result)
    {
    case ECoastalActionResult::MissingItems: case ECoastalActionResult::NoSpace:
    case ECoastalActionResult::TooFar: case ECoastalActionResult::Occluded:
    case ECoastalActionResult::Failed: case ECoastalActionResult::NotConfigured:
        return TEXT("error");
    default:return NAME_None;
    }
}
void UCoastalActionAudio::Action(ECoastalActionResult Result,FName Cue,FVector Location)
{
    Cue=ResultCue(Result,Cue);
    Play(Cue,Location,Cue==TEXT("door_open") || Cue==TEXT("door_close"));
}
void UCoastalActionAudio::Menu(FName Cue) { Play(Cue,FVector::ZeroVector,false); }
void UCoastalActionAudio::Play(FName Cue,FVector Location,bool bSpatial)
{
    const auto* Sound=Clips.Find(Cue);
    if(Cue.IsNone() || !Sound || !IsValid(Options) || !Options->IsAudioReady()
        || !IsValid(Menus) || !Menus->IsInitialized() || LastFrame==GFrameCounter)return;
    const auto Context=Menus->GetAudioPlaybackContext();
    if(Context.interrupted)return;
    Stop();
    Voice=NewObject<UAudioComponent>(GetOwner(),NAME_None,RF_Transient);
    Voice->bAutoActivate=false; Voice->bAutoDestroy=false; Voice->bStopWhenOwnerDestroyed=true;
    Voice->bIsUISound=!bSpatial; Voice->bAllowSpatialization=bSpatial; Voice->bSuppressSubtitles=true;
    Voice->SoundClassOverride=Options->GetEffectsClassForPlayback();
    Voice->AudioDeviceID=GetWorld()->GetAudioDevice().GetDeviceID();
    if(bSpatial)
    {
        Voice->bOverrideAttenuation=true;
        Voice->AttenuationOverrides.bAttenuate=true;
        Voice->AttenuationOverrides.bSpatialize=true;
        Voice->AttenuationOverrides.AttenuationShapeExtents=FVector(150.0f,0,0);
        Voice->AttenuationOverrides.FalloffDistance=1500.0f;
    }
    Voice->SetSound(Sound->Get()); Voice->RegisterComponent(); Voice->SetWorldLocation(Location); Voice->Play();
    VoiceEpoch=Context.epoch; bWorldVoice=bSpatial; LastFrame=GFrameCounter; LastCue=Cue; ++SubmissionCount;
}
void UCoastalActionAudio::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    if(!Voice)return;
    const auto Context=IsValid(Menus)?Menus->GetAudioPlaybackContext():coastal::AudioPlaybackContext{};
    auto* PC=Cast<APlayerController>(GetOwner());
    if(!IsValid(Source) || !IsValid(PC) || PC->GetPawn()!=Source->GetOwner() || !IsValid(Options)
        || !Options->IsAudioReady() || Context.interrupted || Context.epoch!=VoiceEpoch
        || (bWorldVoice && !Context.ambienceAllowed) || !Voice->IsPlaying())Stop();
}
void UCoastalActionAudio::Stop()
{
    auto* Old=Voice.Get(); Voice=nullptr;
    if(IsValid(Old)){Old->Stop();Old->DestroyComponent();}
}
void UCoastalActionAudio::EndPlay(const EEndPlayReason::Type Reason)
{
    Stop();
    if(IsValid(Source))Source->OnActionFeedback.Remove(ActionHandle);
    if(IsValid(Menus))Menus->OnMenuFeedback.Remove(MenuHandle);
    if(IsValid(Options))Options->OnRoutingReleased.Remove(ReleaseHandle);
    Source=nullptr; Options=nullptr; Menus=nullptr; Clips.Reset();
    Super::EndPlay(Reason);
}
