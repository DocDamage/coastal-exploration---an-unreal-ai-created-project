#include "CoastalPlayerRecoveryComponent.h"
#include "CoastalSafetyVolume.h"
#include "CoastalPlacementLibrary.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSwimmingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

UCoastalPlayerRecoveryComponent::UCoastalPlayerRecoveryComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}
bool UCoastalPlayerRecoveryComponent::SettingsValid() const
{ return FMath::IsFinite(FallBoundaryZ) && FMath::Abs(FallBoundaryZ) < 10000000.0f; }
bool UCoastalPlayerRecoveryComponent::BelowBoundary(const FVector& Position) const
{
    return (bInitialized ? bBoundFallBoundary : bEnableFallBoundary)
        && Position.Z < (bInitialized ? BoundFallBoundaryZ : FallBoundaryZ);
}
bool UCoastalPlayerRecoveryComponent::InitializeRecovery(UCoastalSaveCoordinator* Coordinator,
    UCoastalInteractionBridge* Interaction, FTransform InitialDryFallback)
{
    auto* Character = Cast<ACharacter>(GetOwner());
    auto* PC = IsValid(Character) ? Cast<APlayerController>(Character->GetController()) : nullptr;
    if (!IsInGameThread() || bInitialized || bStopped || !IsRegistered() || !SettingsValid()
        || !IsValid(Character) || !IsValid(PC) || !PC->IsLocalController() || PC->GetPawn() != Character
        || !GetWorld() || GetWorld()->GetNetMode() != NM_Standalone
        || !IsValid(Coordinator) || !Coordinator->IsConfigured() || Coordinator->IsBusy()
        || Coordinator->GetWorld() != GetWorld() || Coordinator->GetPlayerCharacter() != Character
        || !IsValid(Interaction) || Interaction->GetOwner() != Character || Interaction->GetCoordinator() != Coordinator
        || !Character->GetCharacterMovement() || !Character->GetCapsuleComponent() || Character->GetCapsuleComponent()->IsSimulatingPhysics()
        || !UCoastalPlacementLibrary::IsDryDestination(Character, InitialDryFallback)) return false;
    TArray<UCoastalPlayerRecoveryComponent*> Owners; Character->GetComponents(Owners);
    if (Owners.Num() != 1 || Owners[0] != this) return false;
    // KillZ must not destroy the pawn before the recovery boundary can catch it.
    if (bEnableFallBoundary && GetWorld()->GetWorldSettings()->bEnableWorldBoundsChecks
        && FallBoundaryZ <= GetWorld()->GetWorldSettings()->KillZ + 500.0f)
    { LastDetail = TEXT("Set the return boundary at least 500 cm above KillZ; do not disable world bounds blindly."); return false; }
    Volumes.Reset();
    for (TActorIterator<ACoastalSafetyVolume> It(GetWorld()); It; ++It)
    {
        if (!It->IsAuthoredCorrectly()) { LastDetail = TEXT("Invalid safety volume: ") + It->GetName(); return false; }
        if (It->Kind == ECoastalSafetyKind::DryCheckpoint
            && !UCoastalPlacementLibrary::IsDryDestination(Character, It->Destination()))
        { LastDetail = TEXT("Checkpoint ReturnPoint is not a valid dry capsule destination: ") + It->GetName(); return false; }
        Volumes.Add(*It);
    }
    Player = Character; Controller = PC; Saves = Coordinator; Bridge = Interaction;
    Swimming = Character->FindComponentByClass<UCoastalSwimmingComponent>();
    Fallback = InitialDryFallback;
    bBoundFallBoundary = bEnableFallBoundary; BoundFallBoundaryZ = FallBoundaryZ; bBoundFadeCamera = bFadeCamera;
    Epoch = Saves->GetSessionEpoch(); bInitialized = true;
    AddTickPrerequisiteComponent(Player->GetCharacterMovement());
    if (IsValid(Swimming)) AddTickPrerequisiteComponent(Swimming);
    Saves->AddTickPrerequisiteComponent(this);
    LastDetail = TEXT("Safe-return source initialized; no campaign or save was created."); return true;
}
bool UCoastalPlayerRecoveryComponent::InspectVolumes(ACoastalSafetyVolume*& Hazard,
    ACoastalSafetyVolume*& Checkpoint) const
{
    Hazard = Checkpoint = nullptr;
    const auto* Capsule = Player->GetCapsuleComponent();
    bool bAmbiguousCheckpoint = false;
    for (const auto& Entry : Volumes)
    {
        auto* V = Entry.Get();
        if (!IsValid(V) || !V->IsAuthoredCorrectly()) return false;
        if (!V->TouchesCapsule(Player->GetActorLocation(), Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight())) continue;
        if (V->IsHazard())
        {
            // Any route boundary dominates DeepWater, independent of actor iteration order.
            if (!Hazard || (Hazard->Kind == ECoastalSafetyKind::DeepWater
                && V->Kind != ECoastalSafetyKind::DeepWater)) Hazard = V;
        }
        else if (Checkpoint) bAmbiguousCheckpoint = true;
        else Checkpoint = V;
    }
    // Overlapping checkpoint regions never select a destination by actor iteration order.
    if (bAmbiguousCheckpoint) Checkpoint = nullptr;
    return true;
}
void UCoastalPlayerRecoveryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    if (!IsInitialized() || Flow.Phase() == coastal::ReturnPhase::Failed) return;
    if (!IsValid(Saves) || !IsValid(Bridge) || !IsValid(Player) || !IsValid(Controller)
        || Controller->GetPawn() != Player || Bridge->GetCoordinator() != Saves)
    { FailReturn(TEXT("The safe-return owner lost its bound player/session. Relaunch rather than rebind.")); return; }
    if (Saves->IsRecoveryRequired()) { FailReturn(TEXT("Campaign recovery is required. No spatial return was attempted.")); return; }
    if (Flow.Active()) { AdvanceReturn(DeltaTime); return; }
    if (Epoch != Saves->GetSessionEpoch()) { Epoch = Saves->GetSessionEpoch(); Visit.Reset(); }
    if (!Saves->HasActiveCampaign() || Saves->IsBusy() || !Bridge->AllowsWorldInput()
        || UGameplayStatics::IsGamePaused(this)) { Visit.Interrupted(); return; }
    ACoastalSafetyVolume *Hazard = nullptr, *Checkpoint = nullptr;
    if (!InspectVolumes(Hazard, Checkpoint)) { FailReturn(TEXT("A registered safety volume disappeared or became invalid.")); return; }
    const bool bProtectedBoundedWater = Hazard && Hazard->Kind == ECoastalSafetyKind::DeepWater
        && IsValid(Swimming) && Swimming->IsRecoveryProtected();
    if ((Hazard && !bProtectedBoundedWater) || BelowBoundary(Player->GetActorLocation()))
    {
        BeginReturn(Hazard && Hazard->Kind == ECoastalSafetyKind::DeepWater
            ? TEXT("Deep water: returning to dry ground.") : TEXT("Outside the safe route: returning to dry ground.")); return;
    }
    const bool bDry = Checkpoint && Player->GetCharacterMovement()->IsMovingOnGround()
        && UCoastalPlacementLibrary::IsDryDestination(Player, Player->GetActorTransform());
    if (Visit.Observe(Checkpoint ? Checkpoint->GetUniqueID() : 0, bDry, DeltaTime)
        && Saves->UpdateDryCheckpoint(Checkpoint->Destination()))
    {
        Visit.Recorded(); Publish(ECoastalReturnNotice::CheckpointRecorded,
            TEXT("Dry checkpoint recorded. Campaign save queued; not yet verified."));
    }
}
void UCoastalPlayerRecoveryComponent::Publish(ECoastalReturnNotice Result, const FString& Detail)
{
    LastDetail = Detail;
    UE_LOG(LogTemp, Display, TEXT("Coastal safe return: %d | %s"), static_cast<int32>(Result), *Detail);
    OnReturnNotice.Broadcast(Result, Detail);
}
void UCoastalPlayerRecoveryComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    bStopped = true;
    if (IsValid(Saves) && (Flow.Active() || (Reason == EEndPlayReason::Destroyed && Saves->HasActiveCampaign()))) Saves->FailPlayerReturn(TEXT("Safe-return component stopped during relocation. Relaunch."));
    Flow.Stop(); ClearFade(); ReleaseControls();
    if (IsValid(Swimming)) RemoveTickPrerequisiteComponent(Swimming);
    if (IsValid(Saves)) Saves->RemoveTickPrerequisiteComponent(this);
    Super::EndPlay(Reason);
}
