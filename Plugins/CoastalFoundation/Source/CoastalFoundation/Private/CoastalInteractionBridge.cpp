#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalWorldObject.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalStoryLibrary.h"
#include "FirstSignalComponent.h"
#include "CoastalDestinationQuestRules.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CoreGlobals.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

namespace
{
    struct FDispatchEnd
    {
        coastal::InteractionDispatchGate& Gate;
        ~FDispatchEnd() { Gate.End(); }
    };
    struct FMutationEnd
    {
        UCoastalSaveCoordinator* Saves;
        ~FMutationEnd() { Saves->EndMutation(); }
    };
}
UCoastalInteractionBridge::UCoastalInteractionBridge()
{ PrimaryComponentTick.bCanEverTick = false; }
bool UCoastalInteractionBridge::Configure(UCoastalSaveCoordinator* Coordinator)
{
    if (IsValid(Saves) || !IsValid(Coordinator) || !Coordinator->IsConfigured() || Coordinator->Player != GetOwner() || Coordinator->GetWorld() != GetWorld()) return false;
    Saves = Coordinator;
    return true;
}
bool UCoastalInteractionBridge::AcquireUIBlocker(FName Token)
{
    if (!IsInGameThread() || Token.IsNone() || UIBlockers.Contains(Token) || InputRevision == MAX_uint64) return false;
    UIBlockers.Add(Token); ++InputRevision;
    return true;
}
void UCoastalInteractionBridge::ReleaseUIBlocker(FName Token)
{
    if (IsInGameThread() && UIBlockers.Remove(Token) > 0 && InputRevision != MAX_uint64) ++InputRevision;
}
void UCoastalInteractionBridge::CloseStorage() { OpenStorage.Reset(); }
void UCoastalInteractionBridge::CancelTranscript() { TranscriptToken.Invalidate(); }
ECoastalActionResult UCoastalInteractionBridge::Emit(ECoastalActionResult Result)
{ OnActionNotice.Broadcast(Result); return Result; }
ECoastalActionResult UCoastalInteractionBridge::FromInventory(ECoastalInventoryCommit Result)
{
    switch (Result)
    {
    case ECoastalInventoryCommit::Committed: return ECoastalActionResult::Applied;
    case ECoastalInventoryCommit::AlreadyCommitted: return ECoastalActionResult::AlreadyApplied;
    case ECoastalInventoryCommit::MissingItems: return ECoastalActionResult::MissingItems;
    case ECoastalInventoryCommit::NotConfigured: return ECoastalActionResult::NotConfigured;
    case ECoastalInventoryCommit::NoSpace: return ECoastalActionResult::NoSpace;
    default: return ECoastalActionResult::Failed;
    }
}
ECoastalActionResult UCoastalInteractionBridge::CheckTarget(ACoastalWorldObject* Target, bool bCheckUI) const
{
    if (!IsInGameThread() || !IsValid(Saves) || !Saves->HasActiveCampaign() || !Saves->IsConfigured())
        return ECoastalActionResult::NotConfigured;
    if (Saves->IsRecoveryRequired() || InputRevision == MAX_uint64) return ECoastalActionResult::Failed;
    if (Saves->IsBusy()) return ECoastalActionResult::Busy;
    if (bCheckUI && (!UIBlockers.IsEmpty() || UGameplayStatics::IsGamePaused(this))) return ECoastalActionResult::BlockedByUI;
    const auto* OwnerCharacter = Cast<ACharacter>(GetOwner());
    const auto* PC = OwnerCharacter ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
    if (!IsValid(PC) || !PC->IsLocalController() || PC->GetPawn() != OwnerCharacter)
        return ECoastalActionResult::InvalidTarget;
    if (!Saves->OwnsObject(Target) || !IsValid(Saves->Player) || !FMath::IsFinite(ReachCm))
        return ECoastalActionResult::InvalidTarget;
    if (Target->Kind == ECoastalObjectKind::Pickup && Target->IsActive()) return ECoastalActionResult::AlreadyApplied;
    const auto* Player = Saves->Player.Get();
    const FVector Point = Target->GetInteractionPoint();
    const float SafeReach = FMath::Clamp(ReachCm, 10.0f, 500.0f);
    if (FVector::DistSquared(Player->GetActorLocation(), Point) > FMath::Square(SafeReach)) return ECoastalActionResult::TooFar;
    FVector Eyes;
    FRotator Rotation;
    Player->GetActorEyesViewPoint(Eyes, Rotation);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CoastalInteractionSight), false, Player);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Eyes, Point, ECC_Visibility, Params)
        && Hit.GetActor() != Target) return ECoastalActionResult::Occluded;
    return ECoastalActionResult::Applied;
}
ECoastalActionResult UCoastalInteractionBridge::TryInteract(ACoastalWorldObject* Target)
{
    if (!IsInGameThread() || !DispatchGate.Begin(GFrameCounter)) return ECoastalActionResult::SuppressedInput;
    FDispatchEnd Dispatch{DispatchGate};
    const auto Check = CheckTarget(Target, true);
    if (Check != ECoastalActionResult::Applied) return Emit(Check);
    if (!Saves->BeginMutation()) return Emit(ECoastalActionResult::Busy);
    ECoastalActionResult Result;
    FName JournalToPresent;
    {
        FMutationEnd Guard{Saves};
        Result = ApplyAction(Target);
        if (Result == ECoastalActionResult::Applied || Result == ECoastalActionResult::AlreadyApplied)
        {
            Saves->RequestSave();
            if (Target->Kind == ECoastalObjectKind::Discovery) JournalToPresent = Target->JournalEntry;
        }
    }
    if (!JournalToPresent.IsNone()) OnJournalRequested.Broadcast(JournalToPresent);
    return Emit(Result);
}
ECoastalActionResult UCoastalInteractionBridge::ApplyAction(ACoastalWorldObject* Target)
{
    auto* Mission = Saves->GetMission();
    switch (Target->Kind)
    {
    case ECoastalObjectKind::Pickup:
    {
        const auto Result = FromInventory(Saves->GetInventory()->TryCollectWorldItem(Target->WorldId, Target->PickupItem));
        if (Result == ECoastalActionResult::Applied || Result == ECoastalActionResult::AlreadyApplied)
            Target->ApplyNativeState(true); // Never hide before a committed insertion.
        return Result;
    }
    case ECoastalObjectKind::Door:
        Target->ApplyNativeState(!Target->IsActive());
        return ECoastalActionResult::Applied;
    case ECoastalObjectKind::Discovery:
        if (Target->JournalEntry.IsNone()) return ECoastalActionResult::Failed;
        if (!FCoastalDestinationQuestRules::CanRead(Target->WorldId,
            Mission->ExportSnapshot().bMessageHeard, Saves->Journal))
            return ECoastalActionResult::Failed;
        {
            const bool bAlreadyRead = Target->IsActive();
            Saves->Journal.AddUnique(Target->JournalEntry);
            Target->ApplyNativeState(true);
            return bAlreadyRead ? ECoastalActionResult::AlreadyApplied : ECoastalActionResult::Applied;
        }
    case ECoastalObjectKind::MaintenanceNote:
        Saves->Journal.AddUnique(TEXT("journal.first_signal.maintenance"));
        Target->ApplyNativeState(true);
        Mission->ReadMaintenanceNote();
        return ECoastalActionResult::Applied;
    case ECoastalObjectKind::Storage:
        if (!OnStorageRequested.IsBound()) return ECoastalActionResult::NotConfigured;
        OpenStorage = Target;
        StorageEpoch = Saves->GetSessionEpoch();
        OnStorageRequested.Broadcast(Target->WorldId);
        return ECoastalActionResult::Applied;
    case ECoastalObjectKind::Radio:
        if (!Mission->ExportSnapshot().bRadioInspected)
        { Mission->InspectRadio(); return ECoastalActionResult::Applied; }
        if (!Mission->ExportSnapshot().bRadioRepaired)
            return FromInventory(Mission->RequestRadioRepair(Saves->GetInventory()));
        if (!OnTranscriptRequested.IsBound()) return ECoastalActionResult::NotConfigured;
        TranscriptToken = FGuid::NewGuid();
        TranscriptEpoch = Saves->GetSessionEpoch();
        OnTranscriptRequested.Broadcast(UCoastalStoryLibrary::RadioTranscript(), TranscriptToken);
        return ECoastalActionResult::Applied;
    default: return ECoastalActionResult::InvalidTarget;
    }
}
ECoastalActionResult UCoastalInteractionBridge::AcknowledgeTranscript(FGuid Token)
{
    if (!IsInGameThread() || !DispatchGate.Begin(GFrameCounter)) return ECoastalActionResult::SuppressedInput;
    FDispatchEnd Dispatch{DispatchGate};
    if (!IsValid(Saves) || !Token.IsValid() || Token != TranscriptToken
        || TranscriptEpoch != Saves->GetSessionEpoch()) return Emit(ECoastalActionResult::InvalidTarget);
    if (!Saves->BeginMutation()) return Emit(ECoastalActionResult::Busy);
    ECoastalActionResult Result = ECoastalActionResult::Failed;
    {
        FMutationEnd Guard{Saves};
        if (Saves->GetMission()->ExportSnapshot().bRadioRepaired)
        {
            // All journal inserts precede the mission notification inside one mutation.
            Saves->Journal.AddUnique(TEXT("journal.first_signal.transmission"));
            Saves->Journal.AddUnique(TEXT("journal.north_reach.lead"));
            const auto Listen = Saves->GetMission()->FinishRadioTransmission();
            Result = Listen == ECoastalListenResult::AlreadyApplied ? ECoastalActionResult::AlreadyApplied : ECoastalActionResult::Applied;
            TranscriptToken.Invalidate();
            Saves->RequestSave();
        }
    }
    return Emit(Result);
}
ECoastalActionResult UCoastalInteractionBridge::TransferWithOpenStorage(const FCoastalTransferRequest& Request)
{
    if (!IsInGameThread() || !DispatchGate.Begin(GFrameCounter)) return ECoastalActionResult::SuppressedInput;
    FDispatchEnd Dispatch{DispatchGate};
    if (!IsValid(Saves) || StorageEpoch != Saves->GetSessionEpoch()) return Emit(ECoastalActionResult::InvalidTarget);
    const auto Check = CheckTarget(OpenStorage.Get(), false);
    if (Check != ECoastalActionResult::Applied) return Emit(Check);
    const FName StorageId = OpenStorage->WorldId;
    const FName Backpack(TEXT("container.player"));
    const bool bAllowedPair = (Request.SourceContainer == Backpack && Request.DestinationContainer == StorageId)
        || (Request.SourceContainer == StorageId && Request.DestinationContainer == Backpack);
    if (!bAllowedPair || !Request.OperationId.IsValid() || !Request.ItemInstanceId.IsValid() || Request.Quantity <= 0)
        return Emit(ECoastalActionResult::InvalidTarget);
    if (!Saves->BeginMutation()) return Emit(ECoastalActionResult::Busy);
    ECoastalActionResult Result;
    {
        FMutationEnd Guard{Saves};
        Result = FromInventory(Saves->GetInventory()->TryTransfer(Request));
        if (Result == ECoastalActionResult::Applied || Result == ECoastalActionResult::AlreadyApplied) Saves->RequestSave();
    }
    return Emit(Result);
}

ECoastalActionResult UCoastalInteractionBridge::ValidateOpenStorage() const
{
    if (!IsValid(Saves) || StorageEpoch != Saves->GetSessionEpoch()) return ECoastalActionResult::InvalidTarget;
    return CheckTarget(OpenStorage.Get(), false);
}
