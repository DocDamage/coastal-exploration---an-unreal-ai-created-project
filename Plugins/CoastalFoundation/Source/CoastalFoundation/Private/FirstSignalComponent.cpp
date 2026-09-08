#include "FirstSignalComponent.h"
#include "CoastalInventoryAdapter.h"
#include "Core/FirstSignalRules.h"
#include "Templates/UnrealTemplate.h"

namespace
{
    coastal::Facts ToFacts(const FFirstSignalSnapshot& State)
    {
        return {State.bCabinVisited, State.bRadioInspected, State.bDockDiscovered,
            State.bMaintenanceNoteRead, State.bRadioRepaired, State.bMessageHeard};
    }
    coastal::CommitResult ToCore(ECoastalInventoryCommit Result)
    {
        switch (Result)
        {
        case ECoastalInventoryCommit::Committed: return coastal::CommitResult::Committed;
        case ECoastalInventoryCommit::AlreadyCommitted: return coastal::CommitResult::AlreadyCommitted;
        case ECoastalInventoryCommit::MissingItems: return coastal::CommitResult::MissingItems;
        case ECoastalInventoryCommit::NotConfigured: return coastal::CommitResult::NotConfigured;
        default: return coastal::CommitResult::Failed;
        }
    }
}

UFirstSignalComponent::UFirstSignalComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}
void UFirstSignalComponent::NotifyChanged() { OnStateChanged.Broadcast(State); }
void UFirstSignalComponent::MarkCabinVisited()
{
    if (!State.bCabinVisited) { State.bCabinVisited = true; NotifyChanged(); }
}
void UFirstSignalComponent::InspectRadio()
{
    if (!State.bRadioInspected) { State.bRadioInspected = true; NotifyChanged(); }
}
void UFirstSignalComponent::DiscoverDock()
{
    if (!State.bDockDiscovered) { State.bDockDiscovered = true; NotifyChanged(); }
}
void UFirstSignalComponent::ReadMaintenanceNote()
{
    if (!State.bMaintenanceNoteRead) { State.bMaintenanceNoteRead = true; NotifyChanged(); }
}
TArray<FCoastalItemRequirement> UFirstSignalComponent::GetRepairRequirements() const
{
    FCoastalItemRequirement Battery;
    Battery.ItemId = TEXT("item.radio_battery");
    FCoastalItemRequirement Fuse;
    Fuse.ItemId = TEXT("item.marine_fuse");
    return {Battery, Fuse};
}
FName UFirstSignalComponent::GetRepairTransactionId() const
{
    return TEXT("first_signal.radio_repair.v1");
}
ECoastalAvailability UFirstSignalComponent::CheckRepairAvailability(UCoastalInventoryAdapter* Inventory) const
{
    if (State.bRadioRepaired) return ECoastalAvailability::Available;
    if (!IsValid(Inventory)) return ECoastalAvailability::NotConfigured;
    return Inventory->CheckRequirements(GetRepairRequirements());
}
EFirstSignalPhase UFirstSignalComponent::GetObjective(UCoastalInventoryAdapter* Inventory) const
{
    // Completed and uninspected stages do not need to query any inventory provider.
    const bool bCheckInventory = State.bRadioInspected && !State.bRadioRepaired;
    const bool bHasEquipment = bCheckInventory
        && CheckRepairAvailability(Inventory) == ECoastalAvailability::Available;
    switch (coastal::CurrentPhase(ToFacts(State), bHasEquipment))
    {
    case coastal::Phase::FindEquipment: return EFirstSignalPhase::FindEquipment;
    case coastal::Phase::ReturnToRadio: return EFirstSignalPhase::ReturnToRadio;
    case coastal::Phase::ListenToRadio: return EFirstSignalPhase::ListenToRadio;
    case coastal::Phase::Complete: return EFirstSignalPhase::Complete;
    default: return EFirstSignalPhase::InspectRadio;
    }
}
ECoastalInventoryCommit UFirstSignalComponent::RequestRadioRepair(UCoastalInventoryAdapter* Inventory)
{
    if (State.bRadioRepaired) return ECoastalInventoryCommit::AlreadyCommitted;
    if (bRepairInProgress) return ECoastalInventoryCommit::Failed;
    if (!IsValid(Inventory)) return ECoastalInventoryCommit::NotConfigured;
    TGuardValue<bool> RepairGuard(bRepairInProgress, true);

    // Do not precheck availability: a recovered, committed transaction may have
    // already consumed its items. The adapter must return AlreadyCommitted for it.
    const ECoastalInventoryCommit Result = Inventory->TryCommitRequirements(
        GetRepairTransactionId(), GetRepairRequirements());
    coastal::Facts Facts = ToFacts(State);
    if (coastal::ApplyRepairCommit(Facts, ToCore(Result)))
    {
        State.bRadioInspected = Facts.radioInspected;
        State.bRadioRepaired = Facts.radioRepaired;
        NotifyChanged();
    }
    return Result;
}
ECoastalListenResult UFirstSignalComponent::FinishRadioTransmission()
{
    coastal::Facts Facts = ToFacts(State);
    const auto Result = coastal::FinishTransmission(Facts);
    if (Result == coastal::ListenResult::NotRepaired) return ECoastalListenResult::NotRepaired;
    if (Result == coastal::ListenResult::AlreadyApplied) return ECoastalListenResult::AlreadyApplied;
    State.bMessageHeard = Facts.messageHeard;
    NotifyChanged();
    return ECoastalListenResult::Applied;
}
FFirstSignalSnapshot UFirstSignalComponent::ExportSnapshot() const { return State; }
bool UFirstSignalComponent::RestoreSnapshot(const FFirstSignalSnapshot& SavedSnapshot)
{
    if (bRepairInProgress || SavedSnapshot.SchemaVersion != 1
        || !coastal::IsValid(ToFacts(SavedSnapshot))) return false;
    State = SavedSnapshot;
    NotifyChanged();
    return true;
}
void UFirstSignalComponent::ResetForNewGame()
{
    if (bRepairInProgress) return;
    State = FFirstSignalSnapshot{};
    NotifyChanged();
}
