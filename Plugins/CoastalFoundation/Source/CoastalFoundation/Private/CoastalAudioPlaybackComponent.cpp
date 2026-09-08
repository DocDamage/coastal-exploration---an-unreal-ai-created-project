#include "CoastalAudioPlaybackComponent.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalUISessionComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Templates/UnrealTemplate.h"

UCoastalAudioPlaybackComponent::UCoastalAudioPlaybackComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}
bool UCoastalAudioPlaybackComponent::SourcesValid() const
{
    const auto Info = [](USoundBase* Sound)
    {
        return coastal::PlaybackSourceInfo{Sound->IsPlayable(), Sound->IsLooping(),
            Sound->IsPlayWhenSilent(), Sound->GetDuration()};
    };
    if (!BoundAmbience && !BoundRadio) return false;
    if (BoundAmbience && (!IsValid(BoundAmbience) || !coastal::ValidAmbienceSource(Info(BoundAmbience)))) return false;
    if (BoundRadio && (!IsValid(BoundRadio) || !coastal::ValidRadioSource(Info(BoundRadio)))) return false;
    return true;
}
bool UCoastalAudioPlaybackComponent::InitializePlayback(UCoastalUISessionComponent* UI,
    UCoastalAudioOptionsComponent* Routing)
{
    auto* World = GetWorld(); auto* PC = Cast<APlayerController>(GetOwner());
    if (!IsInGameThread() || bInitialized || bStopped || !bEnablePlayback || !IsRegistered()
        || !IsComponentTickEnabled() || !World || !World->IsGameWorld() || World->GetNetMode() != NM_Standalone
        || !IsValid(PC) || !PC->IsLocalController() || !PC->GetLocalPlayer()
        || World->GetNumPlayerControllers() != 1 || !IsValid(UI) || !UI->IsInitialized()
        || UI->GetOwner() != PC || !UI->IsRegistered() || !UI->IsComponentTickEnabled()
        || !IsValid(Routing) || Routing->GetOwner() != PC || !Routing->IsAudioReady())
    {
        LastDetail = TEXT("Playback unavailable: opt in on the local controller after native UI and actual routed audio options are ready. Text remains usable.");
        return false;
    }
    TArray<UCoastalAudioPlaybackComponent*> Owners; PC->GetComponents<UCoastalAudioPlaybackComponent>(Owners);
    if (Owners.Num() != 1 || Owners[0] != this)
    { LastDetail = TEXT("Playback unavailable: duplicate controller playback owners."); return false; }
    BoundAmbience = AmbienceLoop; BoundRadio = RadioTransmission;
    if (!SourcesValid())
    {
        BoundAmbience = nullptr; BoundRadio = nullptr;
        LastDetail = TEXT("Playback unavailable: assign a playable looping ambience and/or finite non-looping radio (up to 900 seconds). Assigned sounds must support PlayWhenSilent. No audio assets are supplied.");
        return false;
    }
    Device = World->GetAudioDevice();
    AmbienceBus = Routing->GetAmbienceClassForPlayback(); RadioBus = Routing->GetRadioClassForPlayback();
    if (!Device.IsValid() || !IsValid(AmbienceBus) || !IsValid(RadioBus)
        || !Playback.Begin(BoundAmbience != nullptr, BoundRadio != nullptr))
    { LastDetail = TEXT("Playback unavailable: routing binding could not be captured."); Device.Reset(); return false; }
    Session = UI; AudioOptions = Routing; bInitialized = true; LastDetail.Empty();
    RoutingReleasedHandle = Routing->OnRoutingReleased.AddUObject(this, &UCoastalAudioPlaybackComponent::RoutingReleased);
    AddTickPrerequisiteComponent(UI);
    // No Play here: initial session menus stay silent, and loaded mute is already submitted.
    return true;
}
bool UCoastalAudioPlaybackComponent::IsPlaybackReady() const
{
    auto* World = GetWorld(); auto* PC = Cast<APlayerController>(GetOwner());
    if (!bInitialized || bStopped || !Playback.Active() || !IsRegistered() || !IsComponentTickEnabled()
        || !World || !World->IsGameWorld() || World->GetNetMode() != NM_Standalone
        || !IsValid(PC) || !PC->IsLocalController() || World->GetNumPlayerControllers() != 1
        || !IsValid(Session) || Session->GetOwner() != PC || !Session->IsInitialized()
        || !Session->IsRegistered() || !Session->IsComponentTickEnabled()
        || !IsValid(AudioOptions) || AudioOptions->GetOwner() != PC || !AudioOptions->IsAudioReady()
        || AudioOptions->GetAmbienceClassForPlayback() != AmbienceBus
        || AudioOptions->GetRadioClassForPlayback() != RadioBus || !Device.IsValid()) return false;
    const auto CurrentDevice = World->GetAudioDevice();
    return CurrentDevice.IsValid() && CurrentDevice.GetDeviceID() == Device.GetDeviceID() && SourcesValid();
}
void UCoastalAudioPlaybackComponent::RefreshPlayback()
{
    if (!IsInGameThread() || !bInitialized || bStopped || bRefreshing) return;
    TGuardValue<bool> Guard(bRefreshing, true);
    if (!IsPlaybackReady())
    { Fail(TEXT("Playback stopped: fixed UI, source or routing/device binding lost. Correct the host and relaunch; the campaign is unchanged.")); return; }
    Execute(Playback.Step(Session->GetAudioPlaybackContext()));
}
void UCoastalAudioPlaybackComponent::SuspendPlayback()
{
    if (IsInGameThread() && bInitialized && !bStopped) Execute(Playback.Suspend());
}
void UCoastalAudioPlaybackComponent::TickComponent(float Delta, ELevelTick TickType,
    FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Delta, TickType, TickFunction); RefreshPlayback();
}
void UCoastalAudioPlaybackComponent::RoutingReleased()
{ Fail(TEXT("Playback stopped before its volume routing was removed. Relaunch after correcting audio ownership; text and campaign remain available.")); }
void UCoastalAudioPlaybackComponent::Fail(const TCHAR* Detail)
{
    if (bStopped) return;
    LastDetail = Detail; ReleasePlayback();
    UE_LOG(LogTemp, Warning, TEXT("Coastal Playback: %s"), *LastDetail);
}
void UCoastalAudioPlaybackComponent::ReleasePlayback()
{
    if (bStopped) return;
    bStopped = true; bInitialized = false;
    Playback.Release();
    // Always clean up owned components, including a partially allocated/failed Play attempt.
    DestroyVoice(RadioVoice); DestroyVoice(AmbienceVoice);
    if (IsValid(AudioOptions)) AudioOptions->OnRoutingReleased.Remove(RoutingReleasedHandle);
    RoutingReleasedHandle.Reset();
    if (IsValid(Session)) RemoveTickPrerequisiteComponent(Session);
    Session = nullptr; AudioOptions = nullptr;
    BoundAmbience = nullptr; BoundRadio = nullptr; AmbienceBus = nullptr; RadioBus = nullptr; Device.Reset();
}
void UCoastalAudioPlaybackComponent::OnUnregister()
{ if (bInitialized) ReleasePlayback(); Super::OnUnregister(); }
void UCoastalAudioPlaybackComponent::EndPlay(const EEndPlayReason::Type Reason)
{ ReleasePlayback(); Super::EndPlay(Reason); }
