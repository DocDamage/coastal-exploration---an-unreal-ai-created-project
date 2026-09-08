#include "CoastalUISessionComponent.h"
#include "CoastalAudioPlaybackComponent.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalPanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void UCoastalUISessionComponent::InitializeAudioPlayback()
{
    if (!IsValid(Controller)) return;
    TArray<UCoastalAudioPlaybackComponent*> Owners;
    Controller->GetComponents<UCoastalAudioPlaybackComponent>(Owners);
    if (Owners.Num() > 1)
    { AudioPlaybackNotice = TEXT("Playback unavailable: keep one controller playback component. No duplicate source was initialized."); return; }
    if (Owners.Num() == 1)
    {
        AudioPlayback = Owners[0];
        AudioPlayback->InitializePlayback(this, AudioOptions);
        // Optional audio failure is visible via LastDetail; never fail a healthy campaign or transcript.
    }
}
coastal::AudioPlaybackContext UCoastalUISessionComponent::GetAudioPlaybackContext() const
{
    coastal::AudioPlaybackContext Context;
    if (!IsInitialized() || !IntegrationReady()) return Context;
    Context.epoch = Saves->GetSessionEpoch();
    Context.active = Saves->HasActiveCampaign();
    Context.interrupted = bSessionReset || bRecoveryPending || Epoch != Context.epoch
        || Saves->IsBusy() || Saves->IsRecoveryRequired() || Saves->IsPlayerReturnActive() || Flow.RecoveryRequired();
    Context.ambienceAllowed = !HasModal() && GetWorld() && !GetWorld()->IsPaused() && Bridge->AllowsWorldInput();
    const auto* Top = Flow.Top();
    if (!Context.interrupted && Top && Top->ticket.kind == coastal::PanelKind::Transcript
        && TranscriptToken.IsValid() && !Panels.IsEmpty() && IsValid(Panels.Last())
        && Flow.IsTop(Panels.Last()->Ticket) && Panels.Last()->IsInViewport()
        && Panels.Last()->IsVisible())
    {
        Context.transcript = Top->ticket;
        Context.presented = Panels.Last()->HasBeenPresented();
    }
    return Context;
}
void UCoastalUISessionComponent::RefreshAudioPlayback()
{ if (IsValid(AudioPlayback)) AudioPlayback->RefreshPlayback(); }
void UCoastalUISessionComponent::SuspendAudioPlayback()
{ if (IsValid(AudioPlayback)) AudioPlayback->SuspendPlayback(); }
