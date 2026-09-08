#include "CoastalSaveCoordinator.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalWorldObject.h"
#include "CoastalContractLibrary.h"
#include "FirstSignalComponent.h"
#include "CoastalDestinationQuestRules.h"
#include "Core/TransactionRules.h"
#include "GameFramework/Character.h"

bool UCoastalSaveCoordinator::Capture(FCoastalCampaignSnapshot& Out, FString& Error) const
{
    if (!Ready()) { Error = TEXT("Campaign provider or player is unavailable."); return false; }
    Out = {};
    Out.CampaignId = CampaignId;
    Out.Generation = Generation;
    Out.MapId = MapId;
    Out.PlayerTransform = Player->GetActorTransform();
    Out.DryCheckpoint = DryCheckpoint;
    Out.FirstSignal = Mission->ExportSnapshot();
    Out.Journal = Journal;
    for (const auto& Object : Objects)
    {
        if (!IsValid(Object)) { Error = TEXT("Registered world actor was destroyed."); return false; }
        FCoastalWorldRecord Record;
        Record.WorldId = Object->WorldId;
        Record.Kind = Object->Kind;
        Record.bActive = Object->IsActive();
        Out.World.Add(Record);
    }
    if (Inventory->ExportInventory(Out.Inventory) != ECoastalProviderResult::Ready)
    { Error = TEXT("Inventory export failed."); return false; }
    return Validate(Out, false, Error) == ECoastalProviderResult::Ready;
}
ECoastalProviderResult UCoastalSaveCoordinator::Validate(const FCoastalCampaignSnapshot& Saved,
    bool bPersisted, FString& Error) const
{
    auto Bad = [&Error](const TCHAR* Message)
    { Error = Message; return ECoastalProviderResult::Failed; };
    if (Saved.SchemaVersion != 1 || Saved.FirstSignal.SchemaVersion != 1
        || Saved.Inventory.SchemaVersion != 1 || Saved.MapId != MapId)
    { Error = TEXT("Unsupported schema or map. Migration is required."); return ECoastalProviderResult::Incompatible; }
    if (!Saved.CampaignId.IsValid() || Saved.Inventory.CampaignId != Saved.CampaignId
        || Saved.Generation < (bPersisted ? 1 : 0)) return Bad(TEXT("Invalid campaign or generation."));
    if (Saved.PlayerTransform.ContainsNaN() || Saved.DryCheckpoint.ContainsNaN()
        || !Saved.PlayerTransform.GetRotation().IsNormalized() || !Saved.DryCheckpoint.GetRotation().IsNormalized()
        || !Saved.PlayerTransform.GetScale3D().Equals(FVector::OneVector, 0.001)
        || !Saved.DryCheckpoint.GetScale3D().Equals(FVector::OneVector, 0.001))
        return Bad(TEXT("Invalid player/checkpoint transform."));
    if (Saved.Inventory.ProviderId.IsNone() || Saved.Inventory.ProviderVersion.IsEmpty()
        || Saved.Inventory.Payload.IsEmpty() || Saved.Inventory.Payload.Num() > 12 * 1024 * 1024
        || Saved.Inventory.Receipts.Num() > 4096 || Saved.Journal.Num() > 256
        || Saved.World.Num() > 256
        || (!FCoastalDestinationQuestRules::IsFirstSignalMap(MapId) && Saved.World.Num() != Objects.Num()))
        return Bad(TEXT("Snapshot sections are incomplete or oversized."));
    TMap<FName, FString> Receipts;
    for (const auto& Receipt : Saved.Inventory.Receipts)
    {
        if (Receipt.TransactionId.IsNone() || Receipt.Fingerprint.IsEmpty() || Receipt.Fingerprint.Len() > 16384
            || Receipts.Contains(Receipt.TransactionId)) return Bad(TEXT("Malformed or duplicate receipt."));
        Receipts.Add(Receipt.TransactionId, Receipt.Fingerprint);
    }
    TSet<FName> Entries;
    for (const auto& Entry : Saved.Journal)
    {
        if (Entry.IsNone() || !coastal::ValidLogicalId(TCHAR_TO_UTF8(*Entry.ToString())) || Entries.Contains(Entry))
            return Bad(TEXT("Malformed or duplicate journal entry."));
        Entries.Add(Entry);
    }
    const auto& Q = Saved.FirstSignal;
    const coastal::Facts Facts{Q.bCabinVisited, Q.bRadioInspected, Q.bDockDiscovered,
        Q.bMaintenanceNoteRead, Q.bRadioRepaired, Q.bMessageHeard};
    const FName RepairId = Mission->GetRepairTransactionId();
    if (!coastal::CoherentMission(Facts, Receipts.Contains(RepairId),
        Entries.Contains(TEXT("journal.first_signal.transmission")), Entries.Contains(TEXT("journal.north_reach.lead")),
        Entries.Contains(TEXT("journal.first_signal.maintenance")))) return Bad(TEXT("Mission/ledger/journal mismatch."));
    if (FCoastalDestinationQuestRules::IsFirstSignalMap(MapId)
        && !FCoastalDestinationQuestRules::ValidateSavedManifest(
            Saved.World, Saved.Journal, Q.bMessageHeard, Error))
        return ECoastalProviderResult::Failed;
    FString RepairFingerprint;
    UCoastalContractLibrary::MakeRequirementsFingerprint(Mission->GetRepairRequirements(), RepairFingerprint);
    if (Receipts.Contains(RepairId) && Receipts[RepairId] != RepairFingerprint)
        return Bad(TEXT("Repair receipt has different requirements."));
    TSet<FName> Seen, ExpectedPickups;
    for (const auto& Record : Saved.World)
    {
        if (Seen.Contains(Record.WorldId)) return Bad(TEXT("Duplicate saved world ID."));
        Seen.Add(Record.WorldId);
        const auto* Found = Objects.FindByPredicate([&](const auto& O) { return IsValid(O) && O->WorldId == Record.WorldId; });
        if (!Found || (*Found)->Kind != Record.Kind) return Bad(TEXT("World manifest mismatch."));
        const ACoastalWorldObject* Object = Found->Get();
        if (Record.Kind == ECoastalObjectKind::Pickup)
        {
            const FName Id = UCoastalContractLibrary::PickupTransactionId(Record.WorldId);
            ExpectedPickups.Add(Id);
            FString Expected;
            if (!UCoastalContractLibrary::MakeRequirementsFingerprint({Object->PickupItem}, Expected)
                || Record.bActive != Receipts.Contains(Id)
                || (Receipts.Contains(Id) && Receipts[Id] != Expected))
                return Bad(TEXT("Pickup state/receipt mismatch."));
        }
        if (Record.Kind == ECoastalObjectKind::Discovery
            && (Object->JournalEntry.IsNone() || Record.bActive != Entries.Contains(Object->JournalEntry)))
            return Bad(TEXT("Discovery state/journal mismatch."));
        if (Record.Kind == ECoastalObjectKind::MaintenanceNote && Record.bActive != Q.bMaintenanceNoteRead)
            return Bad(TEXT("Maintenance note state mismatch."));
        if ((Record.Kind == ECoastalObjectKind::Radio || Record.Kind == ECoastalObjectKind::Storage) && Record.bActive)
            return Bad(TEXT("Radio/storage cannot own a duplicated mission/container flag."));
    }
    for (const auto& Receipt : Receipts)
        if (Receipt.Key.ToString().StartsWith(TEXT("pickup.")) && !ExpectedPickups.Contains(Receipt.Key))
            return Bad(TEXT("Orphan pickup receipt."));
    if (!IsValid(Inventory)) return ECoastalProviderResult::NotConfigured;
    const auto Result = Inventory->ValidateInventory(Saved.Inventory);
    if (Result != ECoastalProviderResult::Ready) Error = TEXT("AGIS rejected inventory, definitions, grids or ledger.");
    return Result;
}
