#include "CoastalZiplineRiderComponent.h"
#include "CoastalZipline.h"
#include "CoastalMutableCharacterComponent.h"
#include "CoastalGrappleComponent.h"
#include "CoastalCombatComponent.h"
#include "CoastalCampingActionComponent.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "WeaponBase.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/ScopedMovementUpdate.h"

namespace { constexpr uint8 ZiplineMode = 42; constexpr float HangDrop = 127.2f; constexpr float HandForward = 22.1f; }

UCoastalZiplineRiderComponent::UCoastalZiplineRiderComponent()
{ PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.bTickEvenWhenPaused = true; }

void UCoastalZiplineRiderComponent::BeginPlay()
{
    Super::BeginPlay();
    Character = Cast<ACharacter>(GetOwner());
    Movement = Character ? Character->GetCharacterMovement() : nullptr;
    Bridge = Character ? Character->FindComponentByClass<UCoastalInteractionBridge>() : nullptr;
    Saves = Bridge ? Bridge->GetCoordinator() : nullptr;
    Epoch = Saves ? Saves->GetSessionEpoch() : 0;
    if (Movement) AddTickPrerequisiteComponent(Movement);
    if (Character)
    {
        Trolley = NewObject<UStaticMeshComponent>(Character, TEXT("CoastalZiplineTrolley"));
        Character->AddInstanceComponent(Trolley);
        Trolley->SetupAttachment(Character->GetRootComponent());
        Trolley->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Coastal/Activities/Rope/SM_ZiplineTrolley.SM_ZiplineTrolley")));
        Trolley->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Trolley->bAffectDistanceFieldLighting = false;
        Trolley->SetAbsolute(true,true,true); Trolley->SetHiddenInGame(true);
        Trolley->RegisterComponent();
    }
}

bool UCoastalZiplineRiderComponent::Available() const
{
    auto* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
    auto* Appearance = Character ? Character->FindComponentByClass<UCoastalMutableCharacterComponent>() : nullptr;
    auto* Combat = Character ? Character->FindComponentByClass<UCoastalCombatComponent>() : nullptr;
    auto* Camp = Character ? Character->FindComponentByClass<UCoastalCampingActionComponent>() : nullptr;
    auto* Grapple = Character ? Character->FindComponentByClass<UCoastalGrappleComponent>() : nullptr;
    return IsRegistered() && GetWorld() && !GetWorld()->IsPaused() && GetWorld()->GetNetMode() == NM_Standalone
        && IsValid(PC) && PC->IsLocalController() && PC->GetPawn() == Character && Movement
        && !Character->ActorHasTag(TEXT("Coastal.Prone"))
        && Saves && Saves->HasActiveCampaign() && Epoch == Saves->GetSessionEpoch() && !Saves->IsBusy()
        && !Saves->IsRecoveryRequired() && !Saves->IsPlayerReturnActive() && Bridge && Bridge->AllowsWorldInput()
        && Appearance && Appearance->IsAppearanceReady() && !Appearance->IsGenerating()
        && Trolley && Trolley->GetStaticMesh()
        && (!Combat || (!Combat->IsEncounterActive() && !IsValid(Combat->GetTransientWeapon())))
        && (!Camp || !Camp->IsActionActive()) && (!Grapple || !Grapple->IsGrappleDeployed())
        && Character->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationBlueprint
        && Character->GetMesh()->GetAnimInstance() && !Character->GetMesh()->GetAnimInstance()->IsAnyMontagePlaying()
        && (IsRiding() || Movement->IsMovingOnGround() || Movement->IsFalling());
}

bool UCoastalZiplineRiderComponent::TryBoard(ACoastalZipline* Line)
{
    FVector Point, Tangent; float Length;
    if (IsRiding() || !Available() || !IsValid(Line) || !Line->Sample(.02f, Point, Tangent, Length)
        || FVector::Distance(Character->GetActorLocation() + FVector(0,0,HangDrop), Point) > 220.f)
    { LastDetail = TEXT("Stand near the start of a ready zipline with free hands."); return false; }
    const FTransform Before = Character->GetActorTransform();
    FScopedMovementUpdate Boarding(Movement->UpdatedComponent, EScopedUpdate::DeferredUpdates);
    const FVector Destination = Point - FVector(0,0,HangDrop) - Tangent.GetSafeNormal2D() * HandForward;
    FHitResult Hit;
    Movement->SafeMoveUpdatedComponent(Destination - Before.GetLocation(), FRotator(0,Tangent.Rotation().Yaw,0).Quaternion(), true, Hit);
    if (Hit.bBlockingHit || FVector::Distance(Character->GetActorLocation(), Destination) > 2.f)
    {
        Boarding.RevertMove();
        LastDetail = TEXT("The zipline boarding path is blocked."); return false;
    }
    ActiveLine = Line; Progress = .02f; Speed = 360.f;
    Movement->StopMovementImmediately(); Character->ConsumeMovementInputVector();
    Movement->SetMovementMode(MOVE_Custom, ZiplineMode);
    Trolley->SetWorldLocationAndRotation(Point, Tangent.Rotation()); Trolley->SetHiddenInGame(false);
    LastDetail = TEXT("Riding zipline. V / right stick click releases.");
    return true;
}

void UCoastalZiplineRiderComponent::Stop(bool Momentum)
{
    const bool Owned = IsRiding() || (Movement && Movement->MovementMode == MOVE_Custom && Movement->CustomMovementMode == ZiplineMode);
    ActiveLine.Reset();
    if (Trolley) Trolley->SetHiddenInGame(true);
    if (Owned && IsValid(Movement))
    {
        Movement->SetMovementMode(MOVE_Falling);
        Movement->Velocity = Momentum ? TravelVelocity : FVector::ZeroVector;
    }
    TravelVelocity = FVector::ZeroVector; Speed = 0.f;
}

void UCoastalZiplineRiderComponent::Release() { Stop(true); LastDetail = TEXT("Zipline released."); }

void UCoastalZiplineRiderComponent::InputRide()
{
    if (bNeedsRelease) return;
    bNeedsRelease = true;
    if (IsRiding()) { Release(); return; }
    if (!Available()) return;
    ACoastalZipline* Closest = nullptr; float Best = 220.f;
    for (TActorIterator<ACoastalZipline> It(GetWorld()); It; ++It)
    {
        FVector Point, Tangent; float Length;
        if (!It->Sample(.02f, Point, Tangent, Length)) continue;
        const float Distance = FVector::Distance(Character->GetActorLocation() + FVector(0,0,HangDrop), Point);
        if (Distance < Best) { Closest = *It; Best = Distance; }
    }
    TryBoard(Closest);
}

void UCoastalZiplineRiderComponent::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Type, Function);
    auto* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
    if (RideInput && InputController.Get() != PC) { Stop(false); RemoveInput(); }
    if (!RideInput && PC && PC->GetLocalPlayer())
        if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            InputController = PC;
            RideContext = NewObject<UInputMappingContext>(this);
            RideAction = NewObject<UInputAction>(this); RideAction->ValueType = EInputActionValueType::Boolean;
            RideContext->MapKey(RideAction, EKeys::V); RideContext->MapKey(RideAction, EKeys::Gamepad_RightThumbstick);
            RideInput = NewObject<UEnhancedInputComponent>(PC); RideInput->Priority = 5; RideInput->RegisterComponent();
            RideInput->BindAction(RideAction, ETriggerEvent::Started, this, &UCoastalZiplineRiderComponent::InputRide);
            PC->PushInputComponent(RideInput);
            FModifyContextOptions Options; Options.bIgnoreAllPressedKeysUntilRelease = true;
            Subsystem->AddMappingContext(RideContext, 5, Options);
        }
    const bool Held = PC && (PC->IsInputKeyDown(EKeys::V) || PC->IsInputKeyDown(EKeys::Gamepad_RightThumbstick));
    if (!Held) bNeedsRelease = false;
    if (Saves && Epoch != Saves->GetSessionEpoch()) { Stop(false); Epoch = Saves->GetSessionEpoch(); }
    if (!Available()) { if (Held) bNeedsRelease = true; Stop(false); return; }
    if (!IsRiding()) return;
    FVector Point, Tangent; float Length;
    if (!ActiveLine->Sample(Progress, Point, Tangent, Length)) { Stop(false); return; }
    Speed = FMath::Clamp(Speed + (-GetWorld()->GetGravityZ() * -Tangent.Z - 5.f) * Delta, 180.f, 850.f);
    Progress = FMath::Min(.98f, Progress + Speed * Delta / Length);
    if (!ActiveLine->Sample(Progress, Point, Tangent, Length)) { Stop(false); return; }
    const FVector Before = Character->GetActorLocation();
    FHitResult Hit;
    Movement->SafeMoveUpdatedComponent(Point - FVector(0,0,HangDrop) - Tangent.GetSafeNormal2D() * HandForward - Before, FRotator(0,Tangent.Rotation().Yaw,0).Quaternion(), true, Hit);
    Trolley->SetWorldLocationAndRotation(Point, Tangent.Rotation());
    TravelVelocity = (Character->GetActorLocation() - Before) / FMath::Max(Delta, .001f);
    Movement->Velocity = TravelVelocity;
    Character->ConsumeMovementInputVector();
    if (Hit.bBlockingHit) { Stop(false); LastDetail = TEXT("Zipline path blocked; released."); }
    else if (Progress >= .98f) { Stop(true); LastDetail = TEXT("Zipline complete."); }
}

void UCoastalZiplineRiderComponent::RemoveInput()
{
    if (auto* PC = InputController.Get())
    {
        if (PC->GetLocalPlayer()) if (auto* S = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
            if (RideContext) S->RemoveMappingContext(RideContext);
        if (RideInput) PC->PopInputComponent(RideInput);
    }
    if (RideInput) RideInput->DestroyComponent();
    RideInput = nullptr; RideContext = nullptr; RideAction = nullptr; InputController.Reset();
}

void UCoastalZiplineRiderComponent::EndPlay(const EEndPlayReason::Type Reason)
{ Stop(false); RemoveInput(); if (Trolley) Trolley->DestroyComponent(); Super::EndPlay(Reason); }
