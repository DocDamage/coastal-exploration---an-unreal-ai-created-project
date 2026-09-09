#include "CoastalUISessionComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalPanelWidget.h"
#include "CoastalCampingActionComponent.h"
#include "CoastalCompanionUI.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CoreGlobals.h"

void UCoastalUISessionComponent::QuitWithoutSave()
{ UKismetSystemLibrary::QuitGame(this, Controller, EQuitPreference::Quit, false); }
void UCoastalUISessionComponent::Command(UCoastalPanelWidget* Sender, FName Id)
{
    using K = coastal::PanelKind;
    const bool EmergencyDisplayCancel = IsValid(Sender) && Sender->Ticket.kind == K::ConfirmDisplay
        && (Id == TEXT("back") || Id == TEXT("display_revert"));
    if (!bInitialized || !IsValid(Sender) || Panels.IsEmpty() || Panels.Last() != Sender
        || (!Sender->HasBeenPresented() && !EmergencyDisplayCancel) || !Flow.ClaimCommand(Sender->Ticket, GFrameCounter)) return;
    if (bRecoveryPending || (IsValid(Saves) && Saves->IsRecoveryRequired()))
    { if (!Flow.RecoveryRequired()) EnterRecovery(); else if (Id == TEXT("exit_without_save")) QuitWithoutSave(); return; }
    if (!IsValid(Saves) || bSessionReset || Epoch != Saves->GetSessionEpoch()) return;
    const K Kind = Sender->Ticket.kind;
    // Whitelist actual commands for this screen; native callers cannot bypass disabled buttons.
    FText Title, Body; TArray<FCoastalUIChoice> Choices; Present(Kind, Title, Body, Choices);
    if (Id != TEXT("back") && !Choices.ContainsByPredicate([Id](const auto& C) { return C.Command == Id && C.bEnabled; })) return;
    if (Id == TEXT("back")) { CloseTop(true); return; }
    if ((Kind == K::Inventory || Kind == K::Storage) && Id.ToString().StartsWith(TEXT("select_item.")))
    { SelectInventoryItem(Id); return; }
    if (Kind == K::Journal && Id.ToString().StartsWith(TEXT("read_entry.")))
    { SelectJournalEntry(Id); return; }
    if (Kind == K::Display || Kind == K::ConfirmDisplay || Id == TEXT("display_options"))
    { DisplayCommand(Id); return; }
    if (Kind == K::Settings || Id == TEXT("options")) { OptionsCommand(Id); return; }
    if (Kind == K::Recovery)
    { if (Id == TEXT("exit_without_save")) QuitWithoutSave(); return; }
    if (Kind == K::Pause && Id == TEXT("companion_toggle"))
    {
        CloseTop();
        bool bApplied = false;
        if (!HasModal())
            if (auto* Companion = FindCoastalCompanionCommands(GetWorld()))
                if (Companion->CanCommandCompanion())
                    bApplied = Companion->SetCompanionFollowing(!Companion->IsCompanionFollowing());
        if (!bApplied) ActionNotice = TEXT("The dog is not ready for a command. Try again after returning to dry ground.");
        return;
    }
    if (Kind == K::Pause && (Id == TEXT("camping_warm_hands") || Id == TEXT("camping_rest") || Id == TEXT("shelter_rest")))
    {
        const ECoastalCampingAction Action = Id == TEXT("shelter_rest") ? ECoastalCampingAction::RestInShelter
            : Id == TEXT("camping_warm_hands")
            ? ECoastalCampingAction::WarmHands : ECoastalCampingAction::RestByFire;
        CloseTop();
        if (!HasModal() && IsValid(CampingActions)) CampingActions->StartAction(Action);
        return;
    }
    if (Id == TEXT("quit")) { Push(K::ConfirmExit); return; }
    if (Id == TEXT("exit_without_save")) { QuitWithoutSave(); return; }
    if (Id == TEXT("save") || Id == TEXT("save_exit"))
    {
        if (!IntegrationReady() || Saves->IsBusy()) return;
        const bool Saved = Saves->SaveNow() == ECoastalSaveResult::Saved;
        OnMenuFeedback.Broadcast(Saved ? TEXT("confirm") : TEXT("error"));
        if (Saved && Id == TEXT("save_exit")) QuitWithoutSave();
        return;
    }
    if (Id == TEXT("sessions")) { Push(K::Session); return; }
    if (Id == TEXT("journal")) { Push(K::Journal); return; }
    if (Id == TEXT("inventory"))
    { StorageId = NAME_None; bStorageSide = false; SelectedRow = -1; ReadViews(); Push(K::Inventory); return; }
    if (Id == TEXT("start") || Id == TEXT("continue"))
    {
        if (SelectedSaveSet.IsNone()) { UIError = TEXT("Choose a valid save-set name first."); return; }
        PendingSaveSet = SelectedSaveSet; PendingSessionCommand = Id;
        if (Saves->HasActiveCampaign()) Push(K::ConfirmSession); else CompleteSessionCommand();
        return;
    }
    if (Id == TEXT("confirm_session"))
    { CompleteSessionCommand(); if (!bSessionReset && !bRecoveryPending) CloseTop(); return; }
    if (Id == TEXT("fresh_name"))
    { SelectedSaveSet = FName(*(TEXT("m1_") + FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower())); bRefreshPending = true; return; }
    if (Id == TEXT("next_save"))
    {
        const int32 Next = coastal::CycleSelection(SaveSets.IndexOfByKey(SelectedSaveSet), 1, SaveSets.Num());
        if (SaveSets.IsValidIndex(Next)) SelectedSaveSet = SaveSets[Next];
        bRefreshPending = true; return;
    }
    if (Id == TEXT("acknowledge"))
    {
        const auto Result = Bridge->AcknowledgeTranscript(TranscriptToken);
        if (Result == ECoastalActionResult::Applied || Result == ECoastalActionResult::AlreadyApplied) CloseTop();
        return;
    }
    if (Id == TEXT("previous_item")) { MoveSelection(-1); return; }
    if (Id == TEXT("next_item")) { MoveSelection(1); return; }
    if (Id == TEXT("switch_side"))
    { bStorageSide = !bStorageSide; SelectedRow = -1; ReadViews(); bRefreshPending = true; return; }
    if (Id == TEXT("refresh_inventory")) { ReadViews(); bRefreshPending = true; return; }
    if (Id == TEXT("inspect"))
    {
        if (!bViewsValid || !SelectedView().Items.IsValidIndex(SelectedRow)) return;
        const auto& Item = SelectedView().Items[SelectedRow];
        ItemDetail = FText::FromString(Item.DisplayName.ToString() + TEXT("\n\n") + Item.Description.ToString()
            + FString::Printf(TEXT("\n\nQuantity: %d\nSize: %d x %d spaces"), Item.Quantity, Item.Size.X, Item.Size.Y));
        Push(K::ItemDetails); return;
    }
    if (Id == TEXT("transfer")) { TransferSelected(false); return; }
    if (Id == TEXT("retry_transfer")) { TransferSelected(true); return; }
    if (Id == TEXT("cancel_transfer"))
    { bTransferPending = false; PendingTransfer = {}; ReadViews(); bRefreshPending = true; }
}
