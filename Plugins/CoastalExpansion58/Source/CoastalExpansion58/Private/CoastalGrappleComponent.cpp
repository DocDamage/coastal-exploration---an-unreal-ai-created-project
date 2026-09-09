#include "CoastalGrappleComponent.h"
#include "CoastalRopeWorldCollision.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalMutableCharacterComponent.h"
#include "CoastalCampingActionComponent.h"
#include "CoastalSwimmingComponent.h"
#include "CoastalCombatComponent.h"
#include "WeaponBase.h"
#include "Collision/RopeWrapTargetComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

UCoastalGrappleRope::UCoastalGrappleRope()
{
    ResolveMode = ERopeWrapResolveMode::GuaranteedWrap;
    RopeLength = 2500.f; MinRopeLength = 160.f; ReelSpeed = 400.f;
    NumParticles = 96;
    ThrowParams.FrameMode = ERopeThrowFrameMode::OwnerCamera;
    HoldConfig.bEnforceWielderLengthConstraint = true;
}

bool UCoastalGrappleRope::CanWrapTarget(const USceneComponent* Mesh, FName Bone) const
{
    return IsValid(Mesh) && IsValid(Mesh->GetOwner()) && Mesh->GetOwner() != GetOwner()
        && Mesh->GetOwner()->ActorHasTag(TEXT("Coastal.GrappleAnchor"))
        && Mesh->GetOwner()->FindComponentByClass<URopeWrapTargetComponent>();
}

UCoastalGrappleComponent::UCoastalGrappleComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    HandSocketName = TEXT("hand_r");
    // This installed hang clip has its right hand below the left. Curled finger
    // tips sit inside the grip; wrist origins leave the rope outside both palms.
    HangHandSocketName = TEXT("middle_03_r");
    HangGripSocketName = TEXT("middle_03_l");
    AimRayOriginMode = ERopeAimRayOriginMode::ViewLocation;
    bShowThrowPreview = false;
    bShowAimHudWidget = true;
    bShowPullGaugeWidget = false;
    bAutoBindInput = true;
    MappingPriority = 3;
}

void UCoastalGrappleComponent::BeginPlay()
{
    Character = Cast<ACharacter>(GetOwner());
    Bridge = Character ? Character->FindComponentByClass<UCoastalInteractionBridge>() : nullptr;
    Saves = Bridge ? Bridge->GetCoordinator() : nullptr;
    if (Character && Saves && GetWorld()->GetNetMode() == NM_Standalone)
    {
        Epoch = Saves->GetSessionEpoch();
        AttachMesh = Character->GetMesh();
        if (!Rope)
        {
            Rope = NewObject<UCoastalGrappleRope>(Character, TEXT("CoastalGrappleRope"));
            Rope->TipMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Coastal/Activities/Rope/SM_Grapnel.SM_Grapnel"));
            Rope->bUseTipMesh = Rope->TipMesh != nullptr;
            Rope->bUseTipMeshSockets = true;
            Rope->TipSocketName = TEXT("HookPoint");
            Rope->TipRopeSocketName = TEXT("RopeEye");
            Character->AddInstanceComponent(Rope);
            Rope->SetupAttachment(AttachMesh, HandSocketName);
            Rope->RegisterComponent();
            bOwnsRope = true;
        }
        MappingContext = NewObject<UInputMappingContext>(this);
        auto Action = [this](FKey Keyboard, FKey Gamepad)
        {
            auto* Input = NewObject<UInputAction>(this);
            Input->ValueType = EInputActionValueType::Boolean;
            Input->bConsumeInput = true;
            Input->bTriggerWhenPaused = false;
            MappingContext->MapKey(Input, Keyboard); MappingContext->MapKey(Input, Gamepad);
            return Input;
        };
        ThrowAction = Action(EKeys::Q, EKeys::Gamepad_DPad_Up);
        ReelInAction = Action(EKeys::Z, EKeys::Gamepad_DPad_Right);
        ReelOutAction = Action(EKeys::X, EKeys::Gamepad_DPad_Left);
    }
    Super::BeginPlay();
    if (IsValid(Rope) && Rope->GetTipMeshComponent())
    {
        // The solver places the hook in world space. Later skeletal updates must
        // not drag an embedded hook along with its owner's animated hand.
        auto* Tip = Rope->GetTipMeshComponent();
        const FTransform World = Tip->GetComponentTransform();
        Tip->SetAbsolute(true, true, true);
        Tip->SetWorldTransform(World);
    }
    SetRopeInputSuppressed(!IsGrappleAvailable());
}

bool UCoastalGrappleComponent::IsGrappleAvailable() const
{
    auto* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
    auto* Move = Character ? Character->GetCharacterMovement() : nullptr;
    auto* Appearance = Character ? Character->FindComponentByClass<UCoastalMutableCharacterComponent>() : nullptr;
    auto* Combat = Character ? Character->FindComponentByClass<UCoastalCombatComponent>() : nullptr;
    auto* Camp = Character ? Character->FindComponentByClass<UCoastalCampingActionComponent>() : nullptr;
    auto* Swim = Character ? Character->FindComponentByClass<UCoastalSwimmingComponent>() : nullptr;
    return IsRegistered() && IsValid(Rope) && GetWorld() && GetWorld()->GetNetMode() == NM_Standalone && !GetWorld()->IsPaused()
        && IsValid(PC) && PC->IsLocalController() && PC->GetPawn() == Character
        && IsValid(Saves) && Saves->HasActiveCampaign() && Epoch == Saves->GetSessionEpoch()
        && !Saves->IsBusy() && !Saves->IsRecoveryRequired() && !Saves->IsPlayerReturnActive()
        && IsValid(Bridge) && Bridge->AllowsWorldInput()
        && Move && (Move->IsMovingOnGround() || Move->IsFalling())
        && !Character->ActorHasTag(TEXT("Coastal.Prone"))
        && Appearance && Appearance->IsAppearanceReady() && !Appearance->IsGenerating()
        && (!Camp || !Camp->IsActionActive()) && (!Swim || !Swim->IsSurfaceSwimming())
        && (!Combat || (!Combat->IsEncounterActive() && !IsValid(Combat->GetTransientWeapon())))
        && Character->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationBlueprint
        && Character->GetMesh()->GetAnimInstance() && !Character->GetMesh()->GetAnimInstance()->IsAnyMontagePlaying();
}

bool UCoastalGrappleComponent::CanThrow() const
{
    // A grapple needs a resolved anchor. Bare scenery and open-space shots do not deploy.
    return IsGrappleAvailable() && GetAimHudSample().bHasTarget;
}

bool UCoastalGrappleComponent::IsGrappleDeployed() const
{
    if (!IsValid(Rope)) return false;
    const ERopePhase Phase = Rope->GetPhase();
    // Releasing is a visual return phase; its gameplay length constraint is gone.
    return Phase == ERopePhase::Flight || Phase == ERopePhase::Contacting
        || Phase == ERopePhase::Wrapping || Phase == ERopePhase::Wrapped || Phase == ERopePhase::GuidedThrow;
}

void UCoastalGrappleComponent::CancelGrapple()
{
    StopPull(); StopReel();
    if (IsValid(Rope)) { Rope->CancelQueuedGuaranteedAimThrow(); Rope->ReleaseWrap(); }
}

void UCoastalGrappleComponent::RefreshHandAttachment()
{
    if (Character && IsValid(Rope))
    {
        auto* Appearance = Character->FindComponentByClass<UCoastalMutableCharacterComponent>();
        auto* Mesh = Appearance && Appearance->IsAppearanceReady() ? Appearance->GetGeneratedBody() : Character->GetMesh();
        if (IsValid(Mesh) && Rope->GetAttachParent() != Mesh)
        {
            CancelGrapple(); AttachMesh = Mesh;
            Rope->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocketName);
        }
    }
}

void UCoastalGrappleComponent::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    if (!bWorldCollisionConfigured) bWorldCollisionConfigured = ConfigureCoastalRopeWorldCollision(GetWorld());
    if (Saves && Epoch != Saves->GetSessionEpoch()) { CancelGrapple(); Epoch = Saves->GetSessionEpoch(); }
    const bool Allowed = IsGrappleAvailable();
    if (!Allowed && IsGrappleDeployed()) { CancelGrapple(); LastDetail = TEXT("Grapple released by the current activity."); }
    SetRopeInputSuppressed(!Allowed);
    RefreshHandAttachment();
    if (IsValid(Rope) && Rope->GetPhase() == ERopePhase::Free) Rope->EnterLoaded();
    Super::TickComponent(Delta, Type, Function);
    // The hook is stowed between throws and during menus or incompatible activities.
    if (IsValid(Rope) && Rope->GetTipMeshComponent())
        Rope->GetTipMeshComponent()->SetHiddenInGame(!Allowed || !IsGrappleDeployed());
}

void UCoastalGrappleComponent::NotifyThrown() { LastDetail = TEXT("Grapple launched. Z / right D-pad reels in; X / left D-pad reels out; Q / up D-pad releases."); }
void UCoastalGrappleComponent::NotifyThrowRejected(ERopeThrowRejectReason) { LastDetail = TEXT("Aim at a reachable grapple anchor while free to move."); }

void UCoastalGrappleComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    CancelGrapple();
    Super::EndPlay(Reason);
    if (bOwnsRope && Rope) Rope->DestroyComponent();
    Rope = nullptr;
}
