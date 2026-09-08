#include "CoastalActionAudio.h"
#include "CoastalAudioOptionsComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
bool UCoastalActionAudio::Initialize(UCoastalInteractionBridge* Bridge,UCoastalAudioOptionsComponent* Routing,USoundBase* Sound)
{
    if(Source || !IsRegistered() || !Bridge || !Routing || Routing->GetOwner()!=GetOwner()
        || !Routing->IsAudioReady() || !Sound || !Sound->IsPlayable() || Sound->IsLooping()
        || !Sound->IsPlayWhenSilent() || Sound->GetDuration()<=0 || Sound->GetDuration()>10)return false;
    Source=Bridge; Options=Routing; Clip=Sound;
    Source->OnActionNotice.AddDynamic(this,&UCoastalActionAudio::Action);
    ReleaseHandle=Options->OnRoutingReleased.AddUObject(this,&UCoastalActionAudio::Stop);
    return true;
}
void UCoastalActionAudio::Action(ECoastalActionResult Result)
{
    if(Result!=ECoastalActionResult::Applied || !IsValid(Options) || !Options->IsAudioReady())return;
    Stop();
    Voice=NewObject<UAudioComponent>(GetOwner(),NAME_None,RF_Transient);
    Voice->bAutoActivate=false; Voice->bAutoDestroy=false; Voice->bStopWhenOwnerDestroyed=true;
    Voice->bIsUISound=true; Voice->bAllowSpatialization=false; Voice->bSuppressSubtitles=true;
    Voice->SoundClassOverride=Options->GetEffectsClassForPlayback();
    Voice->AudioDeviceID=GetWorld()->GetAudioDevice().GetDeviceID();
    Voice->SetSound(Clip); Voice->RegisterComponent(); Voice->Play();
}
void UCoastalActionAudio::Stop()
{
    auto* Old=Voice.Get(); Voice=nullptr;
    if(IsValid(Old)){Old->Stop();Old->DestroyComponent();}
}
void UCoastalActionAudio::EndPlay(const EEndPlayReason::Type Reason)
{
    Stop();
    if(IsValid(Source))Source->OnActionNotice.RemoveDynamic(this,&UCoastalActionAudio::Action);
    if(IsValid(Options))Options->OnRoutingReleased.Remove(ReleaseHandle);
    Source=nullptr;Options=nullptr;Clip=nullptr;
    Super::EndPlay(Reason);
}
