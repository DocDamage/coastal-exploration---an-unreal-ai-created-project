#include "CoastalAudioOptionsComponent.h"
#include "CoastalLocalOptions.h"
#include "AudioDevice.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

UCoastalAudioOptionsComponent::UCoastalAudioOptionsComponent()
{
    PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.bTickEvenWhenPaused = true;
    bAutoActivate = true;
}
bool UCoastalAudioOptionsComponent::ClassesValid() const
{
    if (BoundClasses.Num() != 3) return false;
    std::array<coastal::AudioClassInfo, 3> Info;
    for (int32 I = 0; I < 3; ++I)
    {
        const auto* Class = BoundClasses[I].Get();
        if (!IsValid(Class)) return false;
        Info[I] = {Class->GetUniqueID(), Class->ParentClass == nullptr && Class->ChildClasses.IsEmpty()
            && Class->PassiveSoundMixModifiers.IsEmpty(), Class->Properties.Volume, Class->Properties.Pitch};
    }
    return coastal::ValidAudioClasses(Info);
}
bool UCoastalAudioOptionsComponent::InitializeOptions(UCoastalLocalOptions* Options)
{
    auto* World = GetWorld(); auto* PC = Cast<APlayerController>(GetOwner());
    if (!IsInGameThread() || bInitialized || bStopped || !bUseDedicatedSoundClasses || !IsRegistered()
        || !IsComponentTickEnabled() || !World || !World->IsGameWorld() || World->GetNetMode() != NM_Standalone
        || !World->AllowAudioPlayback() || !IsValid(PC) || !PC->IsLocalController()
        || World->GetNumPlayerControllers() != 1 || !IsValid(Options) || !Options->IsInitialized())
    { LastDetail = TEXT("Audio options unavailable: use one opted-in local controller owner with a live audio device and actual dedicated sound classes."); return false; }
    TArray<UCoastalAudioOptionsComponent*> Owners; PC->GetComponents<UCoastalAudioOptionsComponent>(Owners);
    if (Owners.Num() != 1 || Owners[0] != this)
    { LastDetail = TEXT("Audio options unavailable: duplicate controller audio owners."); return false; }
    BoundClasses = {AmbienceClass, EffectsClass, RadioClass};
    Device = World->GetAudioDevice();
    coastal::AudioGains Gains;
    if (!ClassesValid() || !Device.IsValid() || !coastal::BuildAudioGains(Options->Get(), Gains))
    {
        LastDetail = TEXT("Audio options unavailable: bind three distinct isolated classes (no parents, children or passive mixes), valid authored volume/pitch, and a live audio device.");
        BoundClasses.Empty(); Device.Reset(); return false;
    }
    OwnedMix = NewObject<USoundMix>(this, NAME_None, RF_Transient);
    if (!OwnedMix) { BoundClasses.Empty(); Device.Reset(); LastDetail = TEXT("Audio options mix allocation failed."); return false; }
    OwnedMix->bApplyEQ = false; OwnedMix->InitialDelay = 0; OwnedMix->Duration = -1;
    OwnedMix->FadeInTime = 0; OwnedMix->FadeOutTime = 0;
    // Seed the REAL initial values before activation: no intentional full-volume startup fade.
    for (int32 I = 0; I < 3; ++I)
    {
        FSoundClassAdjuster Adjuster;
        Adjuster.SoundClassObject = BoundClasses[I]; Adjuster.VolumeAdjuster = static_cast<float>(Gains[I]);
        Adjuster.PitchAdjuster = 1; Adjuster.bApplyToChildren = false;
        OwnedMix->SoundClassEffects.Add(Adjuster);
    }
    if (!MixState.Begin(Gains))
    { OwnedMix = nullptr; BoundClasses.Empty(); Device.Reset(); return false; }
    Profile = Options; Controller = PC; bInitialized = true; LastDetail.Empty();
    Device->PushSoundMixModifier(OwnedMix, false, false);
    return true; // Native API is void: command submitted, not an audible-output acknowledgement.
}
bool UCoastalAudioOptionsComponent::IsAudioReady() const
{
    auto* World = GetWorld();
    if (!bInitialized || bStopped || !MixState.Active() || !IsRegistered() || !IsComponentTickEnabled()
        || !IsValid(Profile) || !Profile->IsInitialized() || !IsValid(OwnedMix)
        || !IsValid(Controller) || GetOwner() != Controller || !Controller->IsLocalController()
        || !World || !World->IsGameWorld() || World->GetNetMode() != NM_Standalone
        || World->GetNumPlayerControllers() != 1 || !World->AllowAudioPlayback() || !Device.IsValid()
        || !ClassesValid()) return false;
    const auto CurrentDevice = World->GetAudioDevice();
    return CurrentDevice.IsValid() && CurrentDevice.GetDeviceID() == Device.GetDeviceID();
}
USoundClass* UCoastalAudioOptionsComponent::GetAmbienceClassForPlayback() const
{ return IsAudioReady() ? BoundClasses[0].Get() : nullptr; }
USoundClass* UCoastalAudioOptionsComponent::GetEffectsClassForPlayback() const
{ return IsAudioReady() ? BoundClasses[1].Get() : nullptr; }
USoundClass* UCoastalAudioOptionsComponent::GetRadioClassForPlayback() const
{ return IsAudioReady() ? BoundClasses[2].Get() : nullptr; }
bool UCoastalAudioOptionsComponent::ApplyAudioOptions()
{
    if (!IsInGameThread() || !bInitialized || bStopped) return false;
    if (!IsAudioReady())
    { StopWithDiagnostic(TEXT("Audio options disabled: sound-class/controller/audio-device binding changed. Correct the host and relaunch; no automatic rebind.")); return false; }
    coastal::AudioGains Gains;
    if (!coastal::BuildAudioGains(Profile->Get(), Gains))
    { StopWithDiagnostic(TEXT("Audio options disabled: live preference values are invalid.")); return false; }
    if (!MixState.NeedsUpdate(Gains)) return true;
    for (int32 I = 0; I < 3; ++I)
        Device->SetSoundMixClassOverride(OwnedMix, BoundClasses[I], static_cast<float>(Gains[I]),
            1.0f, static_cast<float>(coastal::AudioOptionFadeSeconds), false);
    return MixState.Submitted(Gains); // no repush, asset mutation or audio-thread read-back claim
}
void UCoastalAudioOptionsComponent::TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Delta, TickType, TickFunction);
    if (bInitialized && !bStopped && !IsAudioReady())
        StopWithDiagnostic(TEXT("Audio options disabled: fixed audio binding lost. Existing preference files and campaign are unchanged; relaunch after correcting the host."));
}
void UCoastalAudioOptionsComponent::StopWithDiagnostic(const TCHAR* Detail)
{
    if (bStopped) return;
    LastDetail = Detail; ReleaseOptions();
    UE_LOG(LogTemp, Warning, TEXT("Coastal Audio: %s"), *LastDetail);
}
void UCoastalAudioOptionsComponent::ReleaseOptions()
{
    // Make reentrant release harmless, and stop dependent sources BEFORE exposing underlying volume.
    if (bStopped) return;
    bStopped = true; bInitialized = false; OnRoutingReleased.Broadcast();
    // Pop only the one mix we pushed, on the captured device, even if the world now uses another.
    if (MixState.Release() && Device.IsValid() && IsValid(OwnedMix)) Device->PopSoundMixModifier(OwnedMix, false);
    bStopped = true; bInitialized = false; Device.Reset();
    Profile = nullptr; Controller = nullptr; BoundClasses.Empty();
    // Keep OwnedMix referenced until this component is collected; the device also owns its active mix state.
}
void UCoastalAudioOptionsComponent::OnUnregister()
{
    if (bInitialized) ReleaseOptions();
    Super::OnUnregister();
}
void UCoastalAudioOptionsComponent::EndPlay(const EEndPlayReason::Type Reason)
{ ReleaseOptions(); Super::EndPlay(Reason); }
