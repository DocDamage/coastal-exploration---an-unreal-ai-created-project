#include "CoastalAudioPlaybackComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundAttenuation.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UAudioComponent* UCoastalAudioPlaybackComponent::CreateVoice(USoundBase* Sound, USoundClass* Bus, bool bRadio)
{
    if (!IsPlaybackReady() || !IsValid(Sound) || !IsValid(Bus)) return nullptr;
    auto* Voice = NewObject<UAudioComponent>(GetOwner(), NAME_None, RF_Transient);
    if (!Voice) return nullptr;
    Voice->bAutoActivate = false; Voice->bAutoDestroy = false;
    Voice->bStopWhenOwnerDestroyed = true; Voice->bCanPlayMultipleInstances = false;
    Voice->bAllowSpatialization = false;
    // Local 2D bed/transcript, not an emitter attached to the radio mesh. Disable distance effects too.
    Voice->bOverrideAttenuation = true;
    Voice->AttenuationOverrides.bAttenuate = false;
    Voice->AttenuationOverrides.bSpatialize = false;
    Voice->bIsUISound = bRadio; // The existing transcript modal deliberately pauses the game.
    Voice->bSuppressSubtitles = true; // Coastal owns the full readable transcript; no duplicate native subtitle layer.
    Voice->SoundClassOverride = Bus; Voice->AudioDeviceID = Device.GetDeviceID();
    Voice->SetSound(Sound); Voice->SetVolumeMultiplier(1.0f); Voice->SetPitchMultiplier(1.0f);
    Voice->RegisterComponent();
    if (!Voice->IsRegistered()) { Voice->DestroyComponent(); return nullptr; }
    return Voice;
}
void UCoastalAudioPlaybackComponent::DestroyVoice(TObjectPtr<UAudioComponent>& Voice)
{
    auto* Old = Voice.Get(); Voice = nullptr;
    if (IsValid(Old)) { Old->Stop(); Old->DestroyComponent(); }
}
void UCoastalAudioPlaybackComponent::Execute(const coastal::AudioPlaybackCommands& Commands)
{
    // Stop old session/panel sources BEFORE starting any new source. No completion delegates are bound.
    if (Commands.stopRadio) DestroyVoice(RadioVoice);
    if (Commands.stopAmbience) DestroyVoice(AmbienceVoice);
    if (Commands.pauseAmbience && IsValid(AmbienceVoice)) AmbienceVoice->SetPaused(true);
    if (Commands.resumeAmbience && IsValid(AmbienceVoice)) AmbienceVoice->SetPaused(false);
    if (Commands.startAmbience && IsPlaybackReady())
    {
        DestroyVoice(AmbienceVoice); AmbienceVoice = CreateVoice(BoundAmbience, AmbienceBus, false);
        if (IsValid(AmbienceVoice)) AmbienceVoice->Play(0.0f);
        else LastDetail = TEXT("Ambience playback allocation failed. No automatic retry this campaign session; the game remains usable.");
    }
    if (Commands.startRadio && IsPlaybackReady())
    {
        DestroyVoice(RadioVoice); RadioVoice = CreateVoice(BoundRadio, RadioBus, true);
        if (IsValid(RadioVoice)) RadioVoice->Play(0.0f);
        else LastDetail = TEXT("Radio playback allocation failed. Read the transcript and use Continue normally; no automatic retry for this panel.");
    }
    // Play/Stop are command submission, not a device acknowledgement. Never poll IsPlaying()
    // to restart a finished, muted, virtualized, concurrency-rejected or failed source.
}
