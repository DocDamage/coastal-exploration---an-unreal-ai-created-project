#include "CoastalUISessionComponent.h"
#include "CoastalPanelWidget.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalCampingActionComponent.h"
#include "CoastalCharacterUI.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "CoreGlobals.h"
namespace
{
    FName Blocker(const coastal::PanelTicket& Ticket)
    { return FName(*FString::Printf(TEXT("coastal.ui.%llu"), static_cast<unsigned long long>(Ticket.id))); }
}
bool UCoastalUISessionComponent::Push(coastal::PanelKind Kind, bool bFeedback)
{
    if (!bInitialized || !IsValid(Controller) || !IsValid(Bridge)) return false;
    if (!Panels.IsEmpty()) Flow.RememberFocus(Panels.Last()->Ticket, Panels.Last()->FocusIndex());
    const auto Ticket = Flow.Push(Kind, GFrameCounter);
    if (!Ticket.id) return false;
    auto* Widget = CreateWidget<UCoastalPanelWidget>(Controller, UCoastalPanelWidget::StaticClass());
    if (!Widget || !Bridge->AcquireUIBlocker(Blocker(Ticket)))
    { Flow.Pop(Ticket, true); UIError = TEXT("Unable to create a menu or acquire its input blocker."); return false; }
    if (!Panels.IsEmpty()) Panels.Last()->SetVisibility(ESlateVisibility::Collapsed);
    Widget->Setup(this, Ticket); Panels.Add(Widget);
    Widget->AddToViewport(20 + Panels.Num()); Widget->Refresh();
    ApplyInputOwnership(); Widget->FocusDefault(0, false); Widget->ScrollToTop();
    RefreshAudioPlayback();
    if (bFeedback) OnMenuFeedback.Broadcast(Kind == coastal::PanelKind::Journal ? TEXT("journal") : TEXT("confirm"));
    return true;
}
void UCoastalUISessionComponent::CloseTop(bool bFeedback)
{
    if (Panels.IsEmpty() || !Flow.Pop(Panels.Last()->Ticket, IsValid(Saves) && Saves->HasActiveCampaign())) return;
    auto* Old = Panels.Pop().Get();
    if (Old->Ticket.kind == coastal::PanelKind::CharacterCreator)
        if (auto* Creator = FindCoastalCharacterCreator(IsValid(Bridge) ? Bridge->GetOwner() : nullptr)) Creator->EndCharacterEdit();
    if (Old->Ticket.kind == coastal::PanelKind::ItemDetails) CloseItemPreview();
    if (Old->Ticket.kind == coastal::PanelKind::ConfirmDisplay) CancelDisplay();
    if (IsValid(Bridge))
    {
        Bridge->ReleaseUIBlocker(Blocker(Old->Ticket));
        if (Old->Ticket.kind == coastal::PanelKind::Storage)
        { Bridge->CloseStorage(); StorageId = NAME_None; bTransferPending = false; bViewsValid = false; }
        if (Old->Ticket.kind == coastal::PanelKind::Transcript)
        { Bridge->CancelTranscript(); TranscriptToken.Invalidate(); }
    }
    Old->RemoveFromParent();
    if (Panels.IsEmpty()) ReleaseInputOwnership();
    else
    {
        Panels.Last()->SetVisibility(ESlateVisibility::Visible);
        const int Focus = Flow.Top()->focus; Panels.Last()->Refresh(); Panels.Last()->FocusDefault(Focus);
    }
    RefreshAudioPlayback();
    if (bFeedback) OnMenuFeedback.Broadcast(TEXT("cancel"));
}
void UCoastalUISessionComponent::NotifyMenuFocus(UCoastalPanelWidget* Sender)
{
    if (IsInitialized() && !Panels.IsEmpty() && Panels.Last() == Sender && Sender->HasBeenPresented())
        OnMenuFeedback.Broadcast(TEXT("select"));
}
void UCoastalUISessionComponent::ClearPanels()
{
    if (auto* Creator = FindCoastalCharacterCreator(IsValid(Bridge) ? Bridge->GetOwner() : nullptr)) Creator->EndCharacterEdit();
    SuspendAudioPlayback(); CancelDisplay(); CloseItemPreview();
    for (const auto& Widget : Panels)
    {
        if (IsValid(Bridge)) Bridge->ReleaseUIBlocker(Blocker(Widget->Ticket));
        Widget->RemoveFromParent();
    }
    Panels.Reset(); Flow.Reset(Epoch);
    JournalSelection = PendingJournalEntry = NAME_None; bReadTextPending = false;
    if (IsValid(Bridge)) { Bridge->CloseStorage(); Bridge->CancelTranscript(); }
    StorageId = PendingStorageId = NAME_None; TranscriptToken.Invalidate(); PendingTranscriptToken.Invalidate();
    bTransferPending = false; bViewsValid = false; PendingTransfer = {}; BackpackView = {}; StorageView = {};
    if (IsValid(Bridge)) Bridge->ReleaseUIBlocker(TEXT("coastal.ui.fatal_fallback"));
    ReleaseInputOwnership();
}
void UCoastalUISessionComponent::RefreshTop()
{
    if (Panels.IsEmpty()) return;
    const int32 Focus = Panels.Last()->FocusIndex();
    const FName Command = Panels.Last()->FocusedCommand(); Panels.Last()->Refresh();
    Panels.Last()->RestoreCommandFocus(Command, Focus, !bReadTextPending);
    if (bReadTextPending) Panels.Last()->ScrollToTop();
    bReadTextPending = false;
}
void UCoastalUISessionComponent::EnterRecovery()
{
    if (Flow.RecoveryRequired()) return;
    ClearPanels();
    // Push through the normal widget path before making recovery irrevocable.
    if (!Push(coastal::PanelKind::Recovery))
    {
        Flow.RequireRecovery(GFrameCounter); // Sticky even when widget creation failed.
        if (IsValid(Bridge)) Bridge->AcquireUIBlocker(TEXT("coastal.ui.fatal_fallback"));
        ApplyInputOwnership();
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3600, FColor::Red,
            TEXT("COASTAL RECOVERY REQUIRED: exit and relaunch. No further saving/interactions."));
        return;
    }
    const auto Ticket = Flow.RequireRecovery(GFrameCounter);
    // Replace the old ticket/blocker consistently; never leave an unowned blocker.
    Bridge->ReleaseUIBlocker(Blocker(Panels.Last()->Ticket));
    Panels.Last()->Ticket = Ticket; Bridge->AcquireUIBlocker(Blocker(Ticket));
    SaveNotice = TEXT("RECOVERY REQUIRED. Saving and interaction are disabled. Exit and relaunch a validated save.");
}
void UCoastalUISessionComponent::OpenPause()
{
    if (!bInitialized || HasModal() || !IsValid(Saves) || Saves->IsPlayerReturnActive()) return;
    CancelCampingAction();
    Push(Saves->HasActiveCampaign() ? coastal::PanelKind::Pause : coastal::PanelKind::Session);
}
void UCoastalUISessionComponent::OpenInventory()
{
    if (!bInitialized || HasModal() || !IntegrationReady() || !Saves->HasActiveCampaign() || Saves->IsBusy()) return;
    CancelCampingAction();
    StorageId = NAME_None; bStorageSide = false; SelectedRow = -1; ReadViews(); Push(coastal::PanelKind::Inventory);
}
void UCoastalUISessionComponent::OpenJournal()
{
    if (!bInitialized || HasModal() || !IntegrationReady() || !Saves->HasActiveCampaign() || Saves->IsBusy()) return;
    CancelCampingAction();
    Push(coastal::PanelKind::Journal);
}

void UCoastalUISessionComponent::InitializeCampingActions()
{
    TArray<UCoastalCampingActionComponent*> Owners;
    if (IsValid(Bridge) && IsValid(Bridge->GetOwner())) Bridge->GetOwner()->GetComponents(Owners);
    if (Owners.Num() == 1 && Owners[0]->InitializeCamping(Bridge)) CampingActions = Owners[0];
    else if (!Owners.IsEmpty()) CampingActions = Owners[0];
}

void UCoastalUISessionComponent::CancelCampingAction()
{
    if (IsValid(CampingActions) && CampingActions->IsActionActive()) CampingActions->CancelAction();
}
