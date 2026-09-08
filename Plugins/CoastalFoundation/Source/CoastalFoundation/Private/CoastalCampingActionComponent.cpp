#include "CoastalCampingActionComponent.h"

#include "CoastalInteractionBridge.h"
#include "CoastalPlacementLibrary.h"
#include "CoastalSaveCoordinator.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/Skeleton.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

UCoastalCampingActionComponent::UCoastalCampingActionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
    bAutoActivate = true;
    PresentationMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(
        TEXT("/Game/CampingAnimations/Demo/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")));
    WarmHandsAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
        TEXT("/Game/CampingAnimations/Animations/AS_WarmingUpByTheFire.AS_WarmingUpByTheFire")));
    RestByFireAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(
        TEXT("/Game/CampingAnimations/Animations/AS_ByTheFire.AS_ByTheFire")));
}

bool UCoastalCampingActionComponent::InitializeCamping(UCoastalInteractionBridge* Interaction)
{
    auto* Pawn = Cast<ACharacter>(GetOwner());
    auto* PC = IsValid(Pawn) ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    auto* Move = IsValid(Pawn) ? Pawn->GetCharacterMovement() : nullptr;
    auto* Mesh = IsValid(Pawn) ? Pawn->GetMesh() : nullptr;
    TArray<UCoastalCampingActionComponent*> Owners;
    if (IsValid(Pawn)) Pawn->GetComponents(Owners);
    if (!IsInGameThread() || bInitialized || bStopped || !IsRegistered() || !IsComponentTickEnabled()
        || !GetWorld() || GetWorld()->GetNetMode() != NM_Standalone || !IsValid(Pawn)
        || !IsValid(PC) || !PC->IsLocalController() || PC->GetPawn() != Pawn
        || !IsValid(Move) || !IsValid(Mesh) || !IsValid(Interaction)
        || Interaction->GetOwner() != Pawn || !IsValid(Interaction->GetCoordinator())
        || Owners.Num() != 1 || Owners[0] != this || CampsiteActorTag.IsNone()
        || !FMath::IsFinite(CampsiteRadiusCm) || CampsiteRadiusCm < 50.0f || CampsiteRadiusCm > 500.0f)
    {
        LastDetail = TEXT("Camping unavailable: use one registered component on the fixed local character with valid campsite settings.");
        return false;
    }

    LoadedPresentationMesh = PresentationMesh.LoadSynchronous();
    LoadedWarmHands = WarmHandsAnimation.LoadSynchronous();
    LoadedRestByFire = RestByFireAnimation.LoadSynchronous();
    const bool AssetsMatch = IsValid(LoadedPresentationMesh) && IsValid(LoadedWarmHands)
        && IsValid(LoadedRestByFire) && IsValid(LoadedPresentationMesh->GetSkeleton())
        && LoadedWarmHands->GetSkeleton() == LoadedPresentationMesh->GetSkeleton()
        && LoadedRestByFire->GetSkeleton() == LoadedPresentationMesh->GetSkeleton()
        && LoadedWarmHands->GetPlayLength() > 0.0f && LoadedRestByFire->GetPlayLength() > 0.0f;
    if (!AssetsMatch)
    {
        LoadedPresentationMesh = nullptr; LoadedWarmHands = nullptr; LoadedRestByFire = nullptr;
        LastDetail = TEXT("Camping unavailable: the supplied Quinn mesh and campsite clips are missing or do not share one skeleton.");
        return false;
    }

    LoadShelterAssets();
    Character = Pawn; Controller = PC; Movement = Move; PlayerMesh = Mesh; Bridge = Interaction;
    bInitialized = true; LastDetail.Empty();
    return true;
}

bool UCoastalCampingActionComponent::BindingValid() const
{
    return bInitialized && !bStopped && IsRegistered() && IsComponentTickEnabled()
        && GetWorld() && GetWorld()->GetNetMode() == NM_Standalone
        && IsValid(Character) && GetOwner() == Character && IsValid(Controller)
        && Controller->IsLocalController() && Controller->GetPawn() == Character
        && Character->GetController() == Controller && IsValid(Movement)
        && Character->GetCharacterMovement() == Movement && IsValid(PlayerMesh)
        && Character->GetMesh() == PlayerMesh && IsValid(Bridge)
        && Bridge->GetOwner() == Character && IsValid(Bridge->GetCoordinator());
}

AActor* UCoastalCampingActionComponent::FindNearbyCampsite(ECoastalCampingAction Action) const
{
    if (!BindingValid()) return nullptr;
    AActor* Nearest = nullptr;
    float NearestDistanceSquared = FMath::Square(CampsiteRadiusCm);
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        const FName Tag = Action == ECoastalCampingAction::RestInShelter ? ShelterActorTag : CampsiteActorTag;
        if (Tag.IsNone() || *It == Character || !It->ActorHasTag(Tag)) continue;
        const float DistanceSquared = FVector::DistSquared(Character->GetActorLocation(), It->GetActorLocation());
        if (DistanceSquared <= NearestDistanceSquared)
        {
            Nearest = *It;
            NearestDistanceSquared = DistanceSquared;
        }
    }
    return Nearest;
}

bool UCoastalCampingActionComponent::IsNearCampsite() const
{
    return FindNearbyCampsite() != nullptr;
}

bool UCoastalCampingActionComponent::HasForeignMontage() const
{
    const UAnimInstance* Instance = IsValid(PlayerMesh) ? PlayerMesh->GetAnimInstance() : nullptr;
    return IsValid(Instance) && Instance->IsAnyMontagePlaying();
}

bool UCoastalCampingActionComponent::MarkerDestination(AActor* Marker, FTransform& OutTransform) const
{
    if (!IsValid(Marker)) return false;
    OutTransform = FTransform(FRotator(0.0, Marker->GetActorRotation().Yaw, 0.0),
        Marker->GetActorLocation(), FVector::OneVector);
    return UCoastalPlacementLibrary::IsDryDestination(Character, OutTransform);
}

bool UCoastalCampingActionComponent::CanOfferAction(ECoastalCampingAction Action) const
{
    if (!BindingValid() || bActionActive || !IsValid(AnimationFor(Action)) || !IsValid(MeshFor(Action)) || HasForeignMontage()
        || PlayerMesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint
        || !IsValid(PlayerMesh->GetAnimInstance()) || !IsValid(PlayerMesh->GetAnimClass())
        || !PlayerMesh->bEnableAnimation || PlayerMesh->bPauseAnims || !PlayerMesh->IsComponentTickEnabled()
        || !Movement->IsMovingOnGround() || Character->bIsCrouched) return false;
    const UCoastalSaveCoordinator* Saves = Bridge->GetCoordinator();
    if (!IsValid(Saves) || !Saves->IsConfigured() || !Saves->HasActiveCampaign()
        || Saves->IsBusy() || Saves->IsRecoveryRequired() || Saves->IsPlayerReturnActive()) return false;
    FTransform Destination;
    FHitResult PathHit;
    return MarkerDestination(FindNearbyCampsite(Action), Destination) && SweepToMarker(Destination, PathHit);
}

bool UCoastalCampingActionComponent::SweepToMarker(const FTransform& Destination, FHitResult& OutHit) const
{
    const UCapsuleComponent* Capsule = IsValid(Character) ? Character->GetCapsuleComponent() : nullptr;
    if (!IsValid(Capsule)) return false;
    const float Radius = Capsule->GetScaledCapsuleRadius();
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    if (!FMath::IsFinite(Radius) || !FMath::IsFinite(HalfHeight) || Radius <= 0.0f || HalfHeight < Radius)
        return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CoastalCampingPlacement), false, Character);
    return !GetWorld()->SweepSingleByProfile(OutHit, Character->GetActorLocation(), Destination.GetLocation(),
        FQuat::Identity, TEXT("Pawn"), FCollisionShape::MakeCapsule(Radius,
            FMath::Max(Radius, HalfHeight - 1.0f)), Params);
}

ECoastalCampingStartResult UCoastalCampingActionComponent::StartAction(ECoastalCampingAction Action)
{
    if (bActionActive) { LastDetail = TEXT("A campsite action is already playing."); return ECoastalCampingStartResult::AlreadyActive; }
    if (!BindingValid() || !IsValid(AnimationFor(Action)) || !IsValid(MeshFor(Action)))
    { LastDetail = TEXT("Camping action unavailable: its character, bridge, or supplied animation binding is invalid."); return ECoastalCampingStartResult::Unavailable; }
    const UCoastalSaveCoordinator* Saves = Bridge->GetCoordinator(); if (!IsValid(Saves) || Saves->IsPlayerReturnActive())
    { LastDetail = TEXT("Camping action did not start during player recovery."); return ECoastalCampingStartResult::InputBlocked; }
    if (!Bridge->AllowsWorldInput() || Controller->IsMoveInputIgnored() || Controller->IsLookInputIgnored())
    { LastDetail = TEXT("Camping action did not start because gameplay input is not currently available."); return ECoastalCampingStartResult::InputBlocked; }
    if (HasForeignMontage()) { LastDetail = TEXT("Camping action is waiting for the current character animation to finish."); return ECoastalCampingStartResult::ForeignAnimation; }
    if (PlayerMesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint
        || !IsValid(PlayerMesh->GetAnimInstance()) || !IsValid(PlayerMesh->GetAnimClass())
        || !PlayerMesh->bEnableAnimation || PlayerMesh->bPauseAnims || !PlayerMesh->IsComponentTickEnabled()
        || !Movement->IsMovingOnGround() || Character->bIsCrouched)
    { LastDetail = TEXT("Camping action requires the standing player locomotion animation state."); return ECoastalCampingStartResult::Unavailable; }
    AActor* Marker = FindNearbyCampsite(Action);
    if (!IsValid(Marker))
    { LastDetail = TEXT("Move closer to the campsite action marker."); return ECoastalCampingStartResult::TooFar; }
    FTransform Destination;
    if (!MarkerDestination(Marker, Destination))
    { LastDetail = TEXT("Camping action did not start: the authored marker is not a valid dry capsule destination."); return ECoastalCampingStartResult::UnsafeDestination; }
    FHitResult PathHit;
    if (!SweepToMarker(Destination, PathHit))
    { LastDetail = TEXT("Camping action did not start: the path to the pose marker is blocked."); return ECoastalCampingStartResult::PathBlocked; }

    const FTransform BeforeSnap = Character->GetActorTransform();
    FHitResult MoveHit;
    const bool bMoved = Character->SetActorLocationAndRotation(Destination.GetLocation(), Destination.GetRotation(),
        true, &MoveHit, ETeleportType::TeleportPhysics);
    const bool bAtDestination = bMoved
        && FVector::DistSquared(Character->GetActorLocation(), Destination.GetLocation()) <= 1.0f
        && FMath::Abs(FMath::FindDeltaAngleDegrees(Character->GetActorRotation().Yaw,
            Destination.Rotator().Yaw)) <= 0.1f
        && UCoastalPlacementLibrary::IsDryDestination(Character, Character->GetActorTransform());
    if (!bAtDestination)
    {
        Character->SetActorLocationAndRotation(BeforeSnap.GetLocation(), BeforeSnap.GetRotation(),
            false, nullptr, ETeleportType::TeleportPhysics);
        LastDetail = TEXT("Camping action did not start: collision prevented exact safe placement at the pose marker.");
        return ECoastalCampingStartResult::PathBlocked;
    }

    Movement->StopMovementImmediately();
    Character->StopJumping();
    Character->ConsumeMovementInputVector();
    ActiveAction = Action;
    OriginalMesh = PlayerMesh->GetSkeletalMeshAsset();
    OriginalAnimClass = PlayerMesh->GetAnimClass();
    PlayerMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode, true);
    PlayerMesh->SetSkeletalMesh(MeshFor(Action), true);
    PlayerMesh->PlayAnimation(AnimationFor(Action), false);
    UAnimSingleNodeInstance* SingleNode = PlayerMesh->GetSingleNodeInstance();
    if (!IsValid(SingleNode) || SingleNode->GetAnimationAsset() != AnimationFor(Action) || !SingleNode->IsPlaying())
    {
        // This validation is synchronous; no other game-thread writer can interleave before rollback.
        PlayerMesh->SetAnimation(nullptr);
        PlayerMesh->SetSkeletalMesh(OriginalMesh, true);
        PlayerMesh->SetAnimInstanceClass(OriginalAnimClass);
        Character->SetActorLocationAndRotation(BeforeSnap.GetLocation(), BeforeSnap.GetRotation(),
            false, nullptr, ETeleportType::TeleportPhysics);
        OriginalMesh = nullptr; OriginalAnimClass = nullptr;
        LastDetail = TEXT("Camping action could not start the supplied animation; player presentation was restored.");
        return ECoastalCampingStartResult::Failed;
    }

    ActiveMarker = Marker;
    ActionStartLocation = Character->GetActorLocation();
    bActionActive = true;
    LastDetail = Action == ECoastalCampingAction::RestInShelter
        ? TEXT("Resting in shelter. Move, jump, or open a menu to stop.")
        : Action == ECoastalCampingAction::WarmHands
        ? TEXT("Warming hands by the fire. Move, jump, or open a menu to stop.")
        : TEXT("Resting by the fire. Move, jump, or open a menu to stop.");
    return ECoastalCampingStartResult::Started;
}

bool UCoastalCampingActionComponent::OwnsPresentation() const
{
    const UAnimSingleNodeInstance* SingleNode = IsValid(PlayerMesh) ? PlayerMesh->GetSingleNodeInstance() : nullptr;
    return IsValid(SingleNode) && PlayerMesh->GetSkeletalMeshAsset() == MeshFor(ActiveAction)
        && PlayerMesh->GetAnimationMode() == EAnimationMode::AnimationSingleNode
        && SingleNode->GetAnimationAsset() == AnimationFor(ActiveAction);
}

void UCoastalCampingActionComponent::FinishAction(const FString& Detail, bool bCompleted)
{
    if (!bActionActive) return;
    const bool bOwned = OwnsPresentation();
    if (bOwned)
    {
        PlayerMesh->SetAnimation(nullptr);
        PlayerMesh->SetSkeletalMesh(OriginalMesh, true);
        PlayerMesh->SetAnimInstanceClass(OriginalAnimClass);
    }
    bActionActive = false;
    ActiveMarker.Reset();
    OriginalMesh = nullptr;
    OriginalAnimClass = nullptr;
    LastDetail = bOwned ? Detail
        : TEXT("Camping action ended because another system changed player presentation; that newer state was left untouched.");
    if (!bCompleted && IsValid(Movement)) Movement->StopMovementImmediately();
}

void UCoastalCampingActionComponent::CancelAction()
{
    if (IsInGameThread()) FinishAction(TEXT("Campsite action ended."), false);
}

void UCoastalCampingActionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bActionActive) return;
    if (!BindingValid() || !OwnsPresentation())
    { FinishAction(TEXT("Campsite action ended because its player binding changed."), false); return; }
    if (!Bridge->AllowsWorldInput() || Controller->IsMoveInputIgnored() || Controller->IsLookInputIgnored())
    { FinishAction(TEXT("Campsite action ended when gameplay input became unavailable."), false); return; }
    if (!ActiveMarker.IsValid() || !Movement->IsMovingOnGround() || Character->bIsCrouched
        || !Character->GetPendingMovementInputVector().IsNearlyZero(0.01f)
        || !Movement->Velocity.IsNearlyZero(2.0f)
        // Walking maintains a 1.9–2.4 cm floor gap after the marker snap.
        // Permit that vertical adjustment while still cancelling lateral movement.
        || FVector::DistSquared2D(Character->GetActorLocation(), ActionStartLocation) > FMath::Square(2.0f)
        || FMath::Abs(Character->GetActorLocation().Z - ActionStartLocation.Z) > 5.0f
        || FVector::DistSquared(Character->GetActorLocation(), ActiveMarker->GetActorLocation()) > FMath::Square(CampsiteRadiusCm))
    { FinishAction(TEXT("Campsite action ended when the player moved or left the marker."), false); return; }
    const UAnimSingleNodeInstance* SingleNode = PlayerMesh->GetSingleNodeInstance();
    if (!IsValid(SingleNode) || !SingleNode->IsPlaying())
        FinishAction(TEXT("Campsite action finished."), true);
}

void UCoastalCampingActionComponent::ReleaseCamping()
{
    if (bStopped) return;
    FinishAction(TEXT("Campsite action ended."), false);
    bStopped = true; bInitialized = false;
    Bridge = nullptr; Character = nullptr; Controller = nullptr; Movement = nullptr; PlayerMesh = nullptr;
    LoadedPresentationMesh = nullptr; LoadedWarmHands = nullptr; LoadedRestByFire = nullptr;
    LoadedShelterMesh = nullptr; LoadedShelterRest = nullptr;
}

void UCoastalCampingActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseCamping();
    Super::EndPlay(EndPlayReason);
}
