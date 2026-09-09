#include "CoastalCombatAudioComponent.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalCombatComponent.h"
#include "CoastalUISessionComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"

UCoastalCombatAudioComponent::UCoastalCombatAudioComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    EncounterStartSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
        TEXT("/Game/Coastal/Audio/M3Combat/SW_M3Combat_encounter_start.SW_M3Combat_encounter_start")));
    PlayerHitSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
        TEXT("/Game/Coastal/Audio/M3Combat/SW_M3Combat_player_hit.SW_M3Combat_player_hit")));
}

bool UCoastalCombatAudioComponent::InitializeAudio(UCoastalCombatComponent* Combat,
    UCoastalUISessionComponent* UI, UCoastalAudioOptionsComponent* Routing)
{
    auto* PC = Cast<APlayerController>(GetOwner());
    if (Source || bStopped || !IsRegistered() || !IsValid(PC) || !PC->IsLocalController()
        || !IsValid(Combat) || Combat->GetOwner() != PC->GetPawn() || !Combat->IsInitialized()
        || !IsValid(UI) || UI->GetOwner() != PC || !UI->IsInitialized()
        || !IsValid(Routing) || Routing->GetOwner() != PC || !Routing->IsAudioReady())
        return false;
    USoundBase* Encounter = EncounterStartSound.LoadSynchronous();
    USoundBase* Hit = PlayerHitSound.LoadSynchronous();
    for (USoundBase* Sound : {Encounter, Hit})
        if (!IsValid(Sound) || !Sound->IsPlayable() || Sound->IsLooping() || !Sound->IsPlayWhenSilent()
            || !FMath::IsFinite(Sound->GetDuration()) || Sound->GetDuration() <= 0 || Sound->GetDuration() > 5)
        {
            LastDetail = TEXT("Combat audio requires two bounded, virtualized, non-looping imported sources.");
            return false;
        }
    Source = Combat; Menus = UI; Options = Routing;
    Clips.Add(TEXT("encounter_start"), Encounter); Clips.Add(TEXT("player_hit"), Hit);
    CueHandle = Source->OnCombatPresentation.AddUObject(this, &UCoastalCombatAudioComponent::Cue);
    ReleaseHandle = Options->OnRoutingReleased.AddUObject(this, &UCoastalCombatAudioComponent::StopVoice);
    LastDetail = TEXT("Two listening-pending combat cues use the existing Effects route.");
    return true;
}

bool UCoastalCombatAudioComponent::IsAudioReady() const
{
    auto* PC = Cast<APlayerController>(GetOwner());
    return !bStopped && IsValid(Source) && Source->IsInitialized() && IsValid(PC)
        && PC->GetPawn() == Source->GetOwner() && IsValid(Menus) && Menus->IsInitialized()
        && IsValid(Options) && Options->IsAudioReady() && Clips.Num() == 2;
}

void UCoastalCombatAudioComponent::Cue(FName Name, FVector)
{
    const TObjectPtr<USoundBase>* Sound = Clips.Find(Name);
    if (!Sound || !IsAudioReady()) return;
    const auto Context = Menus->GetAudioPlaybackContext();
    if (!Context.active || !Context.epoch || Context.interrupted || !Context.ambienceAllowed) return;
    StopVoice();
    Voice = NewObject<UAudioComponent>(GetOwner(), NAME_None, RF_Transient);
    Voice->bAutoActivate = false; Voice->bAutoDestroy = false; Voice->bStopWhenOwnerDestroyed = true;
    Voice->bIsUISound = false; Voice->bAllowSpatialization = false; Voice->bSuppressSubtitles = true;
    Voice->SoundClassOverride = Options->GetEffectsClassForPlayback();
    Voice->AudioDeviceID = GetWorld()->GetAudioDevice().GetDeviceID();
    Voice->SetSound(*Sound); Voice->RegisterComponent();
    if (!Voice->IsRegistered()) { StopVoice(); return; }
    VoiceEpoch = Context.epoch; LastCue = Name; ++SubmissionCount; Voice->Play();
}

void UCoastalCombatAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (!Voice) return;
    if (!IsAudioReady()) { StopVoice(); return; }
    const auto Context = Menus->GetAudioPlaybackContext();
    if (!Context.active || Context.interrupted || Context.epoch != VoiceEpoch)
    { StopVoice(); return; }
    Voice->SetPaused(!Context.ambienceAllowed);
    if (Context.ambienceAllowed && !Voice->IsPlaying()) StopVoice();
}

void UCoastalCombatAudioComponent::StopVoice()
{
    UAudioComponent* Old = Voice.Get(); Voice = nullptr; VoiceEpoch = 0;
    if (IsValid(Old)) { Old->Stop(); Old->DestroyComponent(); }
}

void UCoastalCombatAudioComponent::ReleaseAudio()
{
    if (bStopped) return;
    bStopped = true; StopVoice();
    if (IsValid(Source)) Source->OnCombatPresentation.Remove(CueHandle);
    if (IsValid(Options)) Options->OnRoutingReleased.Remove(ReleaseHandle);
    CueHandle.Reset(); ReleaseHandle.Reset(); Source = nullptr; Menus = nullptr; Options = nullptr; Clips.Reset();
}

void UCoastalCombatAudioComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseAudio();
    Super::EndPlay(Reason);
}
