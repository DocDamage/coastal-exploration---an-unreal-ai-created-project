#include "CoastalSwimmingComponent.h"

#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalSwimmingZone.h"
#include "Core/SwimmingRules.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UCoastalSwimmingComponent::UCoastalSwimmingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
    bAutoActivate = true;
}

bool UCoastalSwimmingComponent::SettingsValid() const
{
    return FMath::IsFinite(SwimSpeedCmPerSecond) && SwimSpeedCmPerSecond >= 100.0f
        && SwimSpeedCmPerSecond <= 600.0f && FMath::IsFinite(SurfaceBuoyancy)
        && SurfaceBuoyancy >= 1.05f && SurfaceBuoyancy <= 4.0f
        && FMath::IsFinite(ForwardAnimationSpeedThreshold)
        && ForwardAnimationSpeedThreshold >= 1.0f && ForwardAnimationSpeedThreshold <= 200.0f;
}

bool UCoastalSwimmingComponent::InitializeSwimming(UCoastalInteractionBridge* Interaction)
{
    ACharacter* Pawn = Cast<ACharacter>(GetOwner());
    APlayerController* PC = IsValid(Pawn) ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    UCharacterMovementComponent* Move = IsValid(Pawn) ? Pawn->GetCharacterMovement() : nullptr;
    USkeletalMeshComponent* Mesh = IsValid(Pawn) ? Pawn->GetMesh() : nullptr;
    UCoastalSaveCoordinator* Coordinator = IsValid(Interaction) ? Interaction->GetCoordinator() : nullptr;
    TArray<UCoastalSwimmingComponent*> Owners;
    if (IsValid(Pawn)) Pawn->GetComponents(Owners);
    if (!IsInGameThread() || bInitialized || bStopped || !IsRegistered() || !IsComponentTickEnabled()
        || !SettingsValid() || !GetWorld() || GetWorld()->GetNetMode() != NM_Standalone
        || !IsValid(Pawn) || !IsValid(PC) || !PC->IsLocalController() || PC->GetPawn() != Pawn
        || !IsValid(Move) || !Move->CanEverSwim() || !IsValid(Mesh)
        || !IsValid(Interaction) || Interaction->GetOwner() != Pawn
        || !IsValid(Coordinator) || !Coordinator->IsConfigured()
        || Owners.Num() != 1 || Owners[0] != this)
    {
        LastDetail = TEXT("Swimming unavailable: use one configured component on the fixed local character and initialized campaign bridge.");
        return false;
    }

    LoadedPresentationMesh = PresentationMesh.LoadSynchronous();
    LoadedIdleAnimation = IdleAnimation.LoadSynchronous();
    LoadedForwardAnimation = ForwardAnimation.LoadSynchronous();
    const bool bAssetsMatch = IsValid(LoadedPresentationMesh) && IsValid(LoadedIdleAnimation)
        && IsValid(LoadedForwardAnimation) && IsValid(LoadedPresentationMesh->GetSkeleton())
        && LoadedIdleAnimation->GetSkeleton() == LoadedPresentationMesh->GetSkeleton()
        && LoadedForwardAnimation->GetSkeleton() == LoadedPresentationMesh->GetSkeleton()
        && LoadedIdleAnimation->GetPlayLength() > 0.0f && LoadedForwardAnimation->GetPlayLength() > 0.0f;
    if (!bAssetsMatch)
    {
        LoadedPresentationMesh = nullptr;
        LoadedIdleAnimation = nullptr;
        LoadedForwardAnimation = nullptr;
        LastDetail = TEXT("Swimming unavailable: the supplied presentation mesh and two clips are missing or do not share one skeleton.");
        return false;
    }

    Character = Pawn;
    Controller = PC;
    Movement = Move;
    PlayerMesh = Mesh;
    Bridge = Interaction;
    Saves = Coordinator;
    AddTickPrerequisiteComponent(Movement);
    bInitialized = true;
    LastDetail = TEXT("Bounded surface swimming initialized; no campaign state was changed.");
    return true;
}

bool UCoastalSwimmingComponent::BindingValid() const
{
    return IsInitialized() && IsRegistered() && IsComponentTickEnabled() && GetWorld()
        && GetWorld()->GetNetMode() == NM_Standalone && IsValid(Character)
        && GetOwner() == Character && IsValid(Controller) && Controller->IsLocalController()
        && Controller->GetPawn() == Character && Character->GetController() == Controller
        && IsValid(Movement) && Character->GetCharacterMovement() == Movement
        && IsValid(PlayerMesh) && Character->GetMesh() == PlayerMesh
        && IsValid(Bridge) && Bridge->GetOwner() == Character
        && IsValid(Saves) && Bridge->GetCoordinator() == Saves && Saves->IsConfigured();
}

ACoastalSwimmingZone* UCoastalSwimmingComponent::CurrentZone() const
{
    return IsValid(Movement) ? Cast<ACoastalSwimmingZone>(Movement->GetPhysicsVolume()) : nullptr;
}
coastal::SwimmingSample UCoastalSwimmingComponent::BuildSample(ACoastalSwimmingZone* Zone) const
{
    coastal::SwimmingSample Sample;
    Sample.initialized = IsInitialized(); Sample.binding_valid = BindingValid();
    Sample.zone_valid = IsValid(Zone) && Zone == CurrentZone() && Zone->IsAuthoredCorrectly();
    Sample.within_recovery_depth = Sample.zone_valid && Zone->IsWithinRecoveryDepth(Character);
    Sample.movement_is_swimming = IsValid(Movement) && Movement->IsSwimming();
    Sample.campaign_ready = IsValid(Saves) && Saves->HasActiveCampaign() && !Saves->IsBusy()
        && !Saves->IsRecoveryRequired() && !Saves->IsPlayerReturnActive();
    Sample.world_input_allowed = IsValid(Bridge) && Bridge->AllowsWorldInput()
        && IsValid(Controller) && !Controller->IsMoveInputIgnored() && !Controller->IsLookInputIgnored();
    Sample.world_paused = UGameplayStatics::IsGamePaused(this);
    Sample.same_session = !bSwimming || (IsValid(Saves) && ActiveEpoch == Saves->GetSessionEpoch());
    return Sample;
}

bool UCoastalSwimmingComponent::BeginSwimming(ACoastalSwimmingZone* Zone)
{
    if (!BindingValid() || bSwimming || !IsValid(Zone) || !Zone->IsWithinRecoveryDepth(Character)
        || !Movement->IsSwimming() || PlayerMesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint
        || !IsValid(PlayerMesh->GetAnimInstance()) || !IsValid(PlayerMesh->GetAnimClass())
        || PlayerMesh->GetAnimInstance()->IsAnyMontagePlaying()
        || !PlayerMesh->bEnableAnimation || PlayerMesh->bPauseAnims || !PlayerMesh->IsComponentTickEnabled()
        || !Saves->HasActiveCampaign() || Saves->IsBusy() || Saves->IsRecoveryRequired()
        || Saves->IsPlayerReturnActive() || !Bridge->AllowsWorldInput()
        || Controller->IsMoveInputIgnored() || Controller->IsLookInputIgnored()
        || UGameplayStatics::IsGamePaused(this))
        return false;

    OriginalMesh = PlayerMesh->GetSkeletalMeshAsset(); OriginalAnimClass = PlayerMesh->GetAnimClass();
    OriginalMaxSwimSpeed = Movement->MaxSwimSpeed;
    OriginalBuoyancy = Movement->Buoyancy;
    Movement->MaxSwimSpeed = SwimSpeedCmPerSecond;
    Movement->Buoyancy = SurfaceBuoyancy;
    Movement->Velocity.Z = 0.0;
    PlayerMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode, true);
    PlayerMesh->SetSkeletalMesh(LoadedPresentationMesh, true);
    PlayerMesh->PlayAnimation(LoadedIdleAnimation, true);
    UAnimSingleNodeInstance* SingleNode = PlayerMesh->GetSingleNodeInstance();
    if (!IsValid(SingleNode) || SingleNode->GetAnimationAsset() != LoadedIdleAnimation
        || !SingleNode->IsPlaying())
    {
        PlayerMesh->SetAnimation(nullptr);
        PlayerMesh->SetSkeletalMesh(OriginalMesh, true);
        PlayerMesh->SetAnimInstanceClass(OriginalAnimClass);
        Movement->MaxSwimSpeed = OriginalMaxSwimSpeed;
        Movement->Buoyancy = OriginalBuoyancy;
        OriginalMesh = nullptr; OriginalAnimClass = nullptr;
        LastDetail = TEXT("Swimming presentation failed to start; the prior mesh, AnimBP, and movement tuning were restored.");
        return false;
    }

    ActiveZone = Zone;
    ActiveEpoch = Saves->GetSessionEpoch();
    bForwardAnimation = false;
    bSwimming = true;
    LastDetail = TEXT("Surface swimming active. Move normally and use the shallow ramp to walk out.");
    return true;
}

bool UCoastalSwimmingComponent::OwnsPresentation() const
{
    const UAnimSingleNodeInstance* SingleNode = IsValid(PlayerMesh) ? PlayerMesh->GetSingleNodeInstance() : nullptr;
    const UAnimationAsset* Expected = bForwardAnimation
        ? static_cast<UAnimationAsset*>(LoadedForwardAnimation.Get())
        : static_cast<UAnimationAsset*>(LoadedIdleAnimation.Get());
    return IsValid(SingleNode) && PlayerMesh->GetAnimationMode() == EAnimationMode::AnimationSingleNode
        && PlayerMesh->GetSkeletalMeshAsset() == LoadedPresentationMesh
        && SingleNode->GetAnimationAsset() == Expected;
}

void UCoastalSwimmingComponent::UpdateAnimation()
{
    if (!OwnsPresentation())
    {
        FinishSwimming(TEXT("Swimming ended because another system changed player presentation."));
        return;
    }
    const bool bShouldUseForward = Movement->Velocity.SizeSquared2D()
        >= FMath::Square(ForwardAnimationSpeedThreshold);
    if (bShouldUseForward == bForwardAnimation) return;
    UAnimSequence* Desired = bShouldUseForward ? LoadedForwardAnimation.Get() : LoadedIdleAnimation.Get();
    PlayerMesh->PlayAnimation(Desired, true);
    UAnimSingleNodeInstance* SingleNode = PlayerMesh->GetSingleNodeInstance();
    if (!IsValid(SingleNode) || SingleNode->GetAnimationAsset() != Desired || !SingleNode->IsPlaying())
    {
        // The requested node is ours even if playback failed; make cleanup recognize it.
        bForwardAnimation = bShouldUseForward;
        FinishSwimming(TEXT("Swimming ended because its loop animation could not be changed safely."));
        return;
    }
    bForwardAnimation = bShouldUseForward;
}

void UCoastalSwimmingComponent::FinishSwimming(const FString& Detail)
{
    if (!bSwimming) return;
    const bool bOwnedPresentation = OwnsPresentation();
    if (bOwnedPresentation)
    {
        PlayerMesh->SetAnimation(nullptr);
        PlayerMesh->SetSkeletalMesh(OriginalMesh, true);
        PlayerMesh->SetAnimInstanceClass(OriginalAnimClass);
    }
    if (IsValid(Movement))
    {
        if (FMath::IsNearlyEqual(Movement->MaxSwimSpeed, SwimSpeedCmPerSecond))
            Movement->MaxSwimSpeed = OriginalMaxSwimSpeed;
        if (FMath::IsNearlyEqual(Movement->Buoyancy, SurfaceBuoyancy))
            Movement->Buoyancy = OriginalBuoyancy;
    }
    bSwimming = false; bForwardAnimation = false;
    ActiveZone.Reset();
    ActiveEpoch = 0;
    OriginalMesh = nullptr; OriginalAnimClass = nullptr;
    LastDetail = bOwnedPresentation ? Detail
        : TEXT("Swimming ended after another system changed player presentation; that newer state was left untouched.");
}

bool UCoastalSwimmingComponent::IsRecoveryProtected() const
{
    if (!bSwimming || !BindingValid()) return false;
    ACoastalSwimmingZone* Zone = ActiveZone.Get();
    return coastal::AllowsDeepWaterRecoveryExemption(bSwimming, BuildSample(Zone));
}

void UCoastalSwimmingComponent::CancelSwimmingForRecovery()
{
    if (IsInGameThread()) FinishSwimming(TEXT("Swimming ended for safe return."));
}

bool UCoastalSwimmingComponent::HandleJumpPressed()
{
    if (!bSwimming) return false;
    if (IsValid(Movement)) Movement->Velocity.Z = 0.0;
    LastDetail = TEXT("Use a shallow shore or the dock ramp to leave the water.");
    return true;
}

void UCoastalSwimmingComponent::NotifyZoneEntered(ACoastalSwimmingZone* Zone)
{
    if (!bSwimming && IsValid(Zone)) BeginSwimming(Zone);
}

void UCoastalSwimmingComponent::NotifyZoneLeft(ACoastalSwimmingZone* Zone)
{
    if (bSwimming && ActiveZone.Get() == Zone)
        LastDetail = TEXT("Swimming volume changed; movement state will resolve the adjacent water or shore.");
}

void UCoastalSwimmingComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    ACoastalSwimmingZone* PhysicsZone = ResolveCurrentZone();
    if (bSwimming && PhysicsZone != ActiveZone.Get()) {
        const coastal::SwimmingSample Candidate = BuildSample(PhysicsZone);
        if (coastal::AllowsZoneHandoff(true, true, Candidate)) {
            ActiveZone = PhysicsZone;
            LastDetail = TEXT("Surface swimming continued into adjacent authored water.");
        }
    }
    ACoastalSwimmingZone* Zone = bSwimming ? ActiveZone.Get() : PhysicsZone;
    const coastal::SwimmingSample Sample = BuildSample(Zone);
    switch (coastal::EvaluateSwimming(bSwimming, Sample))
    {
    case coastal::SwimmingDecision::Enter: BeginSwimming(Zone); break;
    case coastal::SwimmingDecision::Maintain: UpdateAnimation(); break;
    case coastal::SwimmingDecision::ExitWater:
        FinishSwimming(TEXT("Walked out of the bounded swimming area; normal locomotion restored.")); break;
    case coastal::SwimmingDecision::CancelInterrupted:
        FinishSwimming(TEXT("Swimming presentation paused while gameplay input or campaign access is unavailable.")); break;
    case coastal::SwimmingDecision::CancelUnsafe:
        FinishSwimming(TEXT("Swimming safety conditions ended; deep-water recovery remains active.")); break;
    default: break;
    }
}

void UCoastalSwimmingComponent::ReleaseSwimming()
{
    if (bStopped) return;
    FinishSwimming(TEXT("Swimming ended."));
    if (IsValid(Movement)) RemoveTickPrerequisiteComponent(Movement);
    bStopped = true; bInitialized = false;
    Bridge = nullptr;
    Saves = nullptr;
    Character = nullptr;
    Controller = nullptr;
    Movement = nullptr;
    PlayerMesh = nullptr;
    LoadedPresentationMesh = nullptr;
    LoadedIdleAnimation = nullptr;
    LoadedForwardAnimation = nullptr;
}

void UCoastalSwimmingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseSwimming();
    Super::EndPlay(EndPlayReason);
}
