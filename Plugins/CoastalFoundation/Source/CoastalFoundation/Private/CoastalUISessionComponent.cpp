#include "CoastalUISessionComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalPanelWidget.h"
#include "CoastalHUDWidget.h"
#include "CoastalLocalOptions.h"
#include "CoastalDisplaySettings.h"
#include "CoastalLookInputComponent.h"
#include "CoastalSprintComponent.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalAudioPlaybackComponent.h"
#include "CoastalCampingActionComponent.h"
#include "CoastalStoryLibrary.h"
#include "FirstSignalComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UCoastalUISessionComponent::UCoastalUISessionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}
bool UCoastalUISessionComponent::InitializeUI(UCoastalSaveCoordinator* Coordinator,
    UCoastalInteractionBridge* Interaction, FTransform NewCampaignCheckpoint)
{
    auto* PC = Cast<APlayerController>(GetOwner());
    if (bInitialized || bClosing || !IsInGameThread() || !IsValid(PC) || !PC->IsLocalController()
        || !PC->GetLocalPlayer() || !IsValid(Coordinator) || !IsValid(Interaction)
        || Coordinator->GetWorld() != GetWorld() || Interaction->GetWorld() != GetWorld()
        || Interaction->GetOwner() != PC->GetPawn()) return false;
    TArray<UCoastalUISessionComponent*> Owners;
    PC->GetComponents<UCoastalUISessionComponent>(Owners);
    if (Owners.Num() != 1 || Owners[0] != this) return false;
    Controller = PC; Saves = Coordinator; Bridge = Interaction; InitialCheckpoint = NewCampaignCheckpoint;
    if (!InstallMenuInput()) { RemoveMenuInput(); Controller = nullptr; Saves = nullptr; Bridge = nullptr; return false; }
    if (!InitializeOptions()) { ShutdownUI(); return false; }
    InitializeCampingActions();
    Recovery = Interaction->GetOwner()->FindComponentByClass<UCoastalPlayerRecoveryComponent>();
    if (IsValid(Recovery)) Recovery->OnReturnNotice.AddUniqueDynamic(this, &UCoastalUISessionComponent::ReturnNotified);
    Saves->OnSaveNotice.AddUniqueDynamic(this, &UCoastalUISessionComponent::SaveNotified);
    Bridge->OnActionNotice.AddUniqueDynamic(this, &UCoastalUISessionComponent::ActionNotified);
    Bridge->OnStorageRequested.AddUniqueDynamic(this, &UCoastalUISessionComponent::StorageRequested);
    Bridge->OnTranscriptRequested.AddUniqueDynamic(this, &UCoastalUISessionComponent::TranscriptRequested);
    Bridge->OnJournalRequested.AddUniqueDynamic(this, &UCoastalUISessionComponent::JournalRequested);
    Epoch = Saves->GetSessionEpoch(); Flow.Reset(Epoch); bInitialized = true;
    HUD = CreateWidget<UCoastalHUDWidget>(PC, UCoastalHUDWidget::StaticClass());
    if (!HUD) { ShutdownUI(); return false; }
    HUD->Setup(this); HUD->AddToViewport(1);
    if (Saves->HasActiveCampaign()) SelectedSaveSet = Saves->GetActiveSaveSet();
    if (Saves->IsRecoveryRequired()) EnterRecovery();
    else if (!Push(coastal::PanelKind::Session)) { ShutdownUI(); return false; }
    InitializeAudioPlayback();
    InitializeDisplay();
    return true;
}
bool UCoastalUISessionComponent::IntegrationReady() const
{
    return IsValid(Controller) && IsValid(Saves) && IsValid(Bridge) && Saves->IsConfigured()
        && Bridge->GetCoordinator() == Saves && Bridge->GetOwner() == Controller->GetPawn();
}
void UCoastalUISessionComponent::SaveNotified(ECoastalSaveResult Result, FString Detail)
{
    SaveNotice = Detail;
    bRecoveryPending |= Result == ECoastalSaveResult::RecoveryRequired;
    bSessionReset |= Result == ECoastalSaveResult::StartedNew || Result == ECoastalSaveResult::Loaded
        || Result == ECoastalSaveResult::RecoveredPrevious;
    if (bSessionReset || bRecoveryPending) { SuspendAudioPlayback(); CancelDisplay(); }
    if (bSessionReset || bRecoveryPending) CancelCampingAction();
    // Callbacks arrive INSIDE the IO guard. Only stop presentation; never recreate/acknowledge a widget here.
}
void UCoastalUISessionComponent::ReturnNotified(ECoastalReturnNotice Result, FString Detail)
{
    SafetyNotice = Detail;
    if (Result == ECoastalReturnNotice::Returning)
    { PendingStorageId = PendingJournalEntry = NAME_None; PendingTranscriptToken.Invalidate(); PendingTranscriptText = FText::GetEmpty(); SuspendAudioPlayback(); CancelDisplay(); CancelCampingAction(); }
    bRecoveryPending |= Result == ECoastalReturnNotice::RestartRequired;
}
void UCoastalUISessionComponent::ActionNotified(ECoastalActionResult Result)
{
    switch (Result)
    {
    case ECoastalActionResult::SuppressedInput: return; // Expected repeated/held edge; do not spam the HUD.
    case ECoastalActionResult::StaleFocus: ActionNotice = TEXT("Select the object again before interacting."); break;
    case ECoastalActionResult::Applied: ActionNotice = TEXT("Action completed."); break;
    case ECoastalActionResult::AlreadyApplied: ActionNotice = TEXT("Already completed; nothing duplicated."); break;
    case ECoastalActionResult::Busy: ActionNotice = TEXT("Operation busy. No new action accepted."); break;
    case ECoastalActionResult::BlockedByUI: ActionNotice = TEXT("Close the current menu before interacting."); break;
    case ECoastalActionResult::TooFar: ActionNotice = TEXT("Move closer to interact."); break;
    case ECoastalActionResult::Occluded: ActionNotice = TEXT("Something blocks your reach."); break;
    case ECoastalActionResult::InvalidTarget: ActionNotice = TEXT("That interaction is no longer valid."); break;
    case ECoastalActionResult::NotConfigured: ActionNotice = TEXT("DEVELOPMENT ERROR: connect the real inventory/interaction adapter."); break;
    case ECoastalActionResult::MissingItems: ActionNotice = TEXT("The battery and fuse must both be in your backpack."); break;
    case ECoastalActionResult::NoSpace: ActionNotice = TEXT("No space. The source item has not been removed."); break;
    default: ActionNotice = TEXT("Action failed. Check the provider log; do not assume a successful change."); break;
    }
}
void UCoastalUISessionComponent::StorageRequested(FName Id) { PendingStorageId = Id; }
void UCoastalUISessionComponent::TranscriptRequested(FText Text, FGuid Token)
{ PendingTranscriptText = Text; PendingTranscriptToken = Token; }
void UCoastalUISessionComponent::JournalRequested(FName EntryId)
{
    if (IsValid(Saves) && Saves->GetJournal().Contains(EntryId)) PendingJournalEntry = EntryId;
}
void UCoastalUISessionComponent::TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Delta, TickType, TickFunction);
    if (!bInitialized || bClosing) return;
    TickDisplay(); // Real-time watchdog runs even while paused or the campaign coordinator is busy.
    if (!IsValid(Saves) || !IsValid(Bridge) || !IsValid(Controller)
        || Bridge->GetOwner() != Controller->GetPawn())
    { UIError = TEXT("The UI lost its configured player/session. Exit and relaunch."); EnterRecovery(); return; }
    if (bRecoveryPending || Saves->IsRecoveryRequired())
    { bRecoveryPending = false; if (!Flow.RecoveryRequired()) EnterRecovery(); return; }
    if (Saves->IsBusy()) { SuspendAudioPlayback(); return; }
    if (bSessionReset || Epoch != Saves->GetSessionEpoch())
    {
        bSessionReset = false; ClearPanels(); Epoch = Saves->GetSessionEpoch(); Flow.Reset(Epoch);
        SelectedSaveSet = Saves->GetActiveSaveSet(); UIError.Empty(); ActionNotice.Empty(); SafetyNotice.Empty();
    }
    if (!PendingStorageId.IsNone())
    {
        StorageId = PendingStorageId; PendingStorageId = NAME_None; bStorageSide = false; SelectedRow = -1;
        if (Bridge->ValidateOpenStorage() == ECoastalActionResult::Applied && !HasModal())
        { ReadViews(); if (!Push(coastal::PanelKind::Storage, false)) Bridge->CloseStorage(); }
        else { Bridge->CloseStorage(); StorageId = NAME_None; }
    }
    if (PendingTranscriptToken.IsValid())
    {
        TranscriptToken = PendingTranscriptToken; TranscriptText = PendingTranscriptText;
        PendingTranscriptToken.Invalidate(); PendingTranscriptText = FText::GetEmpty();
        if (HasModal() || !Push(coastal::PanelKind::Transcript, false)) Bridge->CancelTranscript();
    }
    if (!PendingJournalEntry.IsNone())
    {
        const FName Entry = PendingJournalEntry; PendingJournalEntry = NAME_None;
        if (!HasModal() && Saves->GetJournal().Contains(Entry))
        {
            JournalSelection = Entry;
            bReadTextPending = true;
            if (Push(coastal::PanelKind::Journal, false))
                ActionNotice = TEXT("Journal opened to the selected record.");
            bReadTextPending = false;
        }
    }
    if (Flow.Contains(coastal::PanelKind::Storage)
        && Bridge->ValidateOpenStorage() != ECoastalActionResult::Applied)
    {
        ClearPanels(); UIError = TEXT("Storage closed: it is no longer reachable. No transfer was retried.");
    }
    if (bRefreshPending) { bRefreshPending = false; RefreshTop(); }
}
FText UCoastalUISessionComponent::Objective() const
{
    if (!IntegrationReady()) return FText::FromString(TEXT("DEV: inventory/interaction integration is not configured."));
    if (!Saves->HasActiveCampaign()) return FText::FromString(TEXT("Start a new campaign or select an existing save set."));
    return UCoastalStoryLibrary::CampaignObjective(
        Saves->GetMission()->GetObjective(Saves->GetInventory()), Saves->GetJournal());
}
FText UCoastalUISessionComponent::CampaignTitle() const
{
    if (!IntegrationReady() || !Saves->HasActiveCampaign()) return FText::FromString(TEXT("FIRST SIGNAL"));
    return UCoastalStoryLibrary::CampaignTitle(
        Saves->GetMission()->GetObjective(Saves->GetInventory()), Saves->GetJournal());
}
FText UCoastalUISessionComponent::Status() const
{
    TArray<FString> Lines;
    for (const FString* Line : {&UIError, &SaveNotice, &ActionNotice, &SafetyNotice, &OptionsNotice})
        if (!Line->IsEmpty()) Lines.Add(*Line);
    if (IsValid(SprintInput) && !SprintInput->LastDetail.IsEmpty()) Lines.Add(SprintInput->LastDetail);
    if (IsValid(AudioOptions) && !AudioOptions->LastDetail.IsEmpty()) Lines.Add(AudioOptions->LastDetail);
    if (!AudioPlaybackNotice.IsEmpty()) Lines.Add(AudioPlaybackNotice);
    if (IsValid(AudioPlayback) && !AudioPlayback->LastDetail.IsEmpty()) Lines.Add(AudioPlayback->LastDetail);
    if (IsValid(CampingActions) && !CampingActions->LastDetail.IsEmpty()) Lines.Add(CampingActions->LastDetail);
    if (IsValid(DisplaySettings) && (Flow.Contains(coastal::PanelKind::Display) || DisplaySettings->Trial().Busy()
        || DisplaySettings->Trial().Phase() == coastal::DisplayPhase::Failed)) Lines.Add(DisplaySettings->Status());
    return FText::FromString(FString::Join(Lines, TEXT("\n")));
}
void UCoastalUISessionComponent::ShutdownUI()
{
    if (IsValid(DisplaySettings)) DisplaySettings->Release();
    DisplaySettings = nullptr;
    if (IsValid(Recovery)) Recovery->OnReturnNotice.RemoveDynamic(this, &UCoastalUISessionComponent::ReturnNotified);
    Recovery = nullptr;
    if (IsValid(Saves)) Saves->OnSaveNotice.RemoveDynamic(this, &UCoastalUISessionComponent::SaveNotified);
    if (IsValid(Bridge))
    {
        Bridge->OnActionNotice.RemoveDynamic(this, &UCoastalUISessionComponent::ActionNotified);
        Bridge->OnStorageRequested.RemoveDynamic(this, &UCoastalUISessionComponent::StorageRequested);
        Bridge->OnTranscriptRequested.RemoveDynamic(this, &UCoastalUISessionComponent::TranscriptRequested);
        Bridge->OnJournalRequested.RemoveDynamic(this, &UCoastalUISessionComponent::JournalRequested);
    }
    // Remove source voices before removing their mute/volume mix, including failed startup cleanup.
    if (IsValid(AudioPlayback)) AudioPlayback->ReleasePlayback();
    AudioPlayback = nullptr;
    if (IsValid(CampingActions)) CampingActions->ReleaseCamping();
    CampingActions = nullptr;
    if (IsValid(AudioOptions)) AudioOptions->ReleaseOptions();
    AudioOptions = nullptr;
    if (IsValid(SprintInput)) SprintInput->ReleaseOptions(); SprintInput = nullptr;
    if (IsValid(LookInput)) LookInput->ReleaseOptions(); LookInput = nullptr;
    ClearPanels(); Options = nullptr; RemoveMenuInput(); if (HUD) HUD->RemoveFromParent(); HUD = nullptr; bInitialized = false;
}
void UCoastalUISessionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    bClosing = true; ShutdownUI(); Super::EndPlay(Reason);
}
