#include "CoastalPlacementLibrary.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalWorldObject.h"
#include "FirstSignalComponent.h"
#include "CoastalDestinationQuestRules.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/CommandLine.h"
#endif

bool UCoastalSaveCoordinator::SafeDestination(const FTransform& Transform) const
{
    return UCoastalPlacementLibrary::IsDryDestination(Player.Get(), Transform);
}
bool UCoastalSaveCoordinator::ApplyNative(const FCoastalCampaignSnapshot& Saved, const FTransform& Destination, bool bIsRollback)
{
    TMap<FName, const FCoastalWorldRecord*> SavedRecords;
    for (const auto& Record : Saved.World)
    {
        const auto* Found = Objects.FindByPredicate([&](const auto& Object)
            { return IsValid(Object) && Object->WorldId == Record.WorldId; });
        if (!Found) { UE_LOG(LogTemp, Warning, TEXT("Coastal restore: missing registered object %s"), *Record.WorldId.ToString()); return false; }
        if ((*Found)->Kind != Record.Kind || SavedRecords.Contains(Record.WorldId))
        { UE_LOG(LogTemp, Warning, TEXT("Coastal restore: invalid or duplicate record %s"), *Record.WorldId.ToString()); return false; }
        SavedRecords.Add(Record.WorldId, &Record);
    }
    TArray<bool> RestoredStates;
    RestoredStates.Reserve(Objects.Num());
    for (const auto& Object : Objects)
    {
        if (!IsValid(Object)) return false;
        bool bActive = false;
        if (FCoastalDestinationQuestRules::IsFirstSignalMap(MapId))
        {
            if (FCoastalDestinationQuestRules::ResolveRestoreState(Object->WorldId, SavedRecords, bActive)
                == ECoastalRestoreRecordResolution::Reject)
            { UE_LOG(LogTemp, Warning, TEXT("Coastal restore: required record %s is missing"), *Object->WorldId.ToString()); return false; }
        }
        else
        {
            const FCoastalWorldRecord* const* Record = SavedRecords.Find(Object->WorldId);
            if (!Record) return false;
            bActive = (*Record)->bActive;
        }
        RestoredStates.Add(bActive);
    }
    // Apply only after the complete record set has passed preflight. Missing approved
    // destination records are deliberately reset, including campaign switches and rollback.
    for (int32 Index = 0; Index < Objects.Num(); ++Index)
        Objects[Index]->ApplyNativeState(RestoredStates[Index]);
    Journal = Saved.Journal;
    CampaignId = Saved.CampaignId;
    Generation = Saved.Generation;
    DryCheckpoint = Saved.DryCheckpoint;
    if (!Mission->RestoreSnapshot(Saved.FirstSignal)) { UE_LOG(LogTemp, Warning, TEXT("Coastal restore: mission rejected snapshot")); return false; }
#if WITH_DEV_AUTOMATION_TESTS
    // Explicit disposable-session fault: exercise late restore plus failed rollback.
    if (HasActiveCampaign() && GetActiveSaveSet().ToString().StartsWith(TEXT("coastal_test_"))
        && FParse::Param(FCommandLine::Get(), TEXT("CoastalFailNativeRestore"))
        && (!bIsRollback || !FParse::Param(FCommandLine::Get(), TEXT("CoastalFailProviderRollback"))))
    {
        UE_LOG(LogTemp, Warning, TEXT("COASTAL_TEST_NATIVE_RESTORE_FAILURE after real inventory/world/mission restore"));
        return false;
    }
#endif
    // Check again AFTER restoring door collision; the saved world can differ from live geometry.
    FTransform FinalDestination = Destination;
    if (!SafeDestination(FinalDestination)) FinalDestination = Saved.DryCheckpoint;
    if (!SafeDestination(FinalDestination)) { UE_LOG(LogTemp, Warning, TEXT("Coastal restore: restored world blocked both destinations")); return false; }
    // UE's MoveComponent can return false when the requested change is below its
    // transform tolerance. A restore to the current, already validated placement
    // is successful without moving; real moves still require checked teleport.
    const bool bAlreadyPlaced = Player->GetActorLocation().Equals(FinalDestination.GetLocation(), UE_KINDA_SMALL_NUMBER)
        && Player->GetActorRotation().Equals(FinalDestination.Rotator(), UE_KINDA_SMALL_NUMBER);
    if (!bAlreadyPlaced && !Player->TeleportTo(FinalDestination.GetLocation(), FinalDestination.Rotator(), false, false))
    { UE_LOG(LogTemp, Warning, TEXT("Coastal restore: checked player placement failed")); return false; }
    Player->GetCharacterMovement()->StopMovementImmediately();
    return SafeDestination(Player->GetActorTransform());
}
ECoastalSaveResult UCoastalSaveCoordinator::RestoreCoherently(const FCoastalCampaignSnapshot& Saved)
{
    FString Error;
    FCoastalCampaignSnapshot Previous;
    if (!Capture(Previous, Error)) return Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("Cannot capture rollback state: ") + Error);
    FTransform Destination = SafeDestination(Saved.PlayerTransform) ? Saved.PlayerTransform : Saved.DryCheckpoint;
    if (!SafeDestination(Destination))
        return Notice(ECoastalSaveResult::RestoreFailed, TEXT("Saved position and dry checkpoint are unsafe. Live state was not changed."));
    if (Inventory->RestoreInventory(Saved.Inventory) != ECoastalProviderResult::Ready)
        return Notice(ECoastalSaveResult::RestoreFailed, TEXT("Atomic inventory restore rejected. Live state must remain unchanged."));
    if (ApplyNative(Saved, Destination)) return ECoastalSaveResult::Loaded;
    // Late placement/world failure: roll back ALL sections, not just mission flags.
    FCoastalInventorySnapshot RollbackInventory = Previous.Inventory;
#if WITH_DEV_AUTOMATION_TESTS
    const bool bTestProviderRejection = HasActiveCampaign()
        && GetActiveSaveSet().ToString().StartsWith(TEXT("coastal_test_"))
        && FParse::Param(FCommandLine::Get(), TEXT("CoastalFailNativeRestore"))
        && FParse::Param(FCommandLine::Get(), TEXT("CoastalFailProviderRollback"));
    // Deliberately invalid rollback input exercises the actual adapter's rejection path.
    if (bTestProviderRejection) RollbackInventory.Payload.Reset();
#endif
    const auto InventoryRollbackResult = Inventory->RestoreInventory(RollbackInventory);
    const bool bInventoryRestored = InventoryRollbackResult == ECoastalProviderResult::Ready;
    const bool bNativeRestored = ApplyNative(Previous, Previous.PlayerTransform, true);
#if WITH_DEV_AUTOMATION_TESTS
    if (bTestProviderRejection)
    {
        UE_LOG(LogTemp, Display, TEXT("COASTAL_TEST_PROVIDER_ROLLBACK result=%d native_restored=%d"),
            int32(InventoryRollbackResult), int32(bNativeRestored));
    }
#endif
    if (bInventoryRestored && bNativeRestored)
        return Notice(ECoastalSaveResult::RestoreFailed, TEXT("Restore failed; previous live campaign recovered."));
    Gate.Poison();
    return Notice(ECoastalSaveResult::RecoveryRequired,
        TEXT("Rollback failed. Interactions and saves are locked. Exit and relaunch; disk saves were not modified."));
}
