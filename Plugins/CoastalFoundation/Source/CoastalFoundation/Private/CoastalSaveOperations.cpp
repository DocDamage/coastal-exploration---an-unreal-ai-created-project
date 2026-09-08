#include "CoastalSaveCoordinator.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalWorldObject.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    struct FIOEnd
    {
        coastal::OperationGate& Gate;
        ~FIOEnd() { Gate.EndIO(); }
    };
}
ECoastalSaveResult UCoastalSaveCoordinator::StartNewCampaign(FName SaveSet, FTransform InitialDryCheckpoint)
{
    if (!IsInGameThread() || !Gate.BeginIO()) return ECoastalSaveResult::Busy;
    FIOEnd Guard{Gate};
    if (!Ready()) return Notice(ECoastalSaveResult::NotConfigured, TEXT("Connect the provider and configure the director first."));
    if (!ValidSaveSet(SaveSet)) return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("Invalid save-set ID."));
    if (UGameplayStatics::DoesSaveGameExist(SlotName(SaveSet, 0), 0)
        || UGameplayStatics::DoesSaveGameExist(SlotName(SaveSet, 1), 0))
        return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("Save set already exists. Continue it or use a new ID; nothing was deleted."));
    if (!SafeDestination(InitialDryCheckpoint))
        return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("New-game checkpoint is obstructed or has no safe floor."));
    FCoastalCampaignSnapshot New;
    New.CampaignId = FGuid::NewGuid();
    New.MapId = MapId;
    New.PlayerTransform = InitialDryCheckpoint;
    New.DryCheckpoint = InitialDryCheckpoint;
    for (const auto& Object : Objects)
    {
        if (!IsValid(Object)) return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("A registered actor was destroyed."));
        FCoastalWorldRecord Record;
        Record.WorldId = Object->WorldId;
        Record.Kind = Object->Kind;
        New.World.Add(Record);
    }
    if (Inventory->BuildNewInventory(New.CampaignId, New.Inventory) != ECoastalProviderResult::Ready)
        return Notice(ECoastalSaveResult::NotConfigured, TEXT("Provider cannot construct a fresh campaign snapshot."));
    FString Error;
    if (Validate(New, false, Error) != ECoastalProviderResult::Ready)
        return Notice(ECoastalSaveResult::InvalidSnapshot, Error);
    const auto Result = RestoreCoherently(New);
    if (Result != ECoastalSaveResult::Loaded) return Result;
    ActiveSaveSet = SaveSet;
    LastVerifiedSlot = -1;
    ++SessionEpoch;
    Gate.ClearPending();
    Gate.RequestSave();
    return Notice(ECoastalSaveResult::StartedNew, TEXT("New campaign started in memory. First save is queued, not yet confirmed."));
}
ECoastalSaveResult UCoastalSaveCoordinator::LoadCampaign(FName SaveSet)
{
    if (!IsInGameThread() || !Gate.BeginIO()) return ECoastalSaveResult::Busy;
    FIOEnd Guard{Gate};
    if (!Ready()) return Notice(ECoastalSaveResult::NotConfigured, TEXT("Continue blocked: inventory adapter/player is unavailable."));
    if (!ValidSaveSet(SaveSet)) return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("Invalid save-set ID."));
    FCoastalCampaignSnapshot A, B;
    const auto AI = ReadSlot(SaveSet, 0, A);
    const auto BI = ReadSlot(SaveSet, 1, B);
    const auto Choice = coastal::ChooseSlot(AI, BI);
    if (Choice.blocked) return Notice(ECoastalSaveResult::Incompatible, TEXT("Schema/provider incompatibility or tied generations. No automatic overwrite."));
    if (AI.status == coastal::SlotStatus::Valid && BI.status == coastal::SlotStatus::Valid && A.CampaignId != B.CampaignId)
        return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("A/B slots belong to different campaigns. Load refused."));
    if (Choice.index < 0)
    {
        const bool bAbsent = AI.status == coastal::SlotStatus::Missing && BI.status == coastal::SlotStatus::Missing;
        return Notice(bAbsent ? ECoastalSaveResult::NoSave : ECoastalSaveResult::InvalidSnapshot,
            bAbsent ? TEXT("No save exists in this set.") : TEXT("No valid save generation. No new game was started."));
    }
    const auto Result = RestoreCoherently(Choice.index == 0 ? A : B);
    if (Result != ECoastalSaveResult::Loaded) return Result;
    ActiveSaveSet = SaveSet;
    LastVerifiedSlot = Choice.index;
    ++SessionEpoch;
    Gate.ClearPending();
    return Notice(Choice.recovered ? ECoastalSaveResult::RecoveredPrevious : ECoastalSaveResult::Loaded,
        Choice.recovered ? TEXT("One slot is damaged. Recovered the remaining validated generation.") : TEXT("Campaign loaded coherently."));
}
ECoastalSaveResult UCoastalSaveCoordinator::SaveNow()
{
    if (!IsInGameThread() || !Gate.BeginIO()) return ECoastalSaveResult::Busy;
    FIOEnd Guard{Gate};
    if (!Ready() || !HasActiveCampaign())
        return Notice(ECoastalSaveResult::NotConfigured, TEXT("No configured active campaign to save."));
    FCoastalCampaignSnapshot Current;
    FString Error;
    if (!Capture(Current, Error)) return Notice(ECoastalSaveResult::InvalidSnapshot, Error);
    // Re-read both slots before overwriting: protect foreign/future/external changes.
    FCoastalCampaignSnapshot A, B;
    const auto AI = ReadSlot(ActiveSaveSet, 0, A);
    const auto BI = ReadSlot(ActiveSaveSet, 1, B);
    const auto Choice = coastal::ChooseSlot(AI, BI);
    if (Choice.blocked) return Notice(ECoastalSaveResult::Incompatible, TEXT("Save pair is incompatible or ambiguous; write refused."));
    if ((AI.status == coastal::SlotStatus::Valid && A.CampaignId != CampaignId)
        || (BI.status == coastal::SlotStatus::Valid && B.CampaignId != CampaignId))
        return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("Foreign campaign in save set; write refused."));
    if (Choice.index < 0 && (AI.status != coastal::SlotStatus::Missing || BI.status != coastal::SlotStatus::Missing))
        return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("Both slots invalid. Preserved them for recovery."));
    const int64 DiskGeneration = Choice.index < 0 ? 0 : (Choice.index == 0 ? AI.generation : BI.generation);
    if (DiskGeneration > Generation)
        return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("A newer disk generation exists. Reload before saving."));
    std::int64_t Next = 0;
    if (!coastal::NextGeneration(Generation, Next))
        return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("Save generation overflow."));
    Current.Generation = Next;
    const int32 Target = Choice.index < 0 ? 0 : 1 - Choice.index;
    if (!WriteVerified(ActiveSaveSet, Target, Current))
        return Notice(ECoastalSaveResult::DiskFailure, TEXT("Save write/readback validation failed. The other slot was not touched."));
    Generation = Next;
    LastVerifiedSlot = Target;
    Gate.ClearPending();
    return Notice(ECoastalSaveResult::Saved, FString::Printf(TEXT("Verified generation %lld."), static_cast<long long>(Generation)));
}
