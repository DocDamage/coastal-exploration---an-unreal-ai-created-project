#include "CoastalLookInputComponent.h"
#include "CoastalLocalOptions.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "CoreGlobals.h"

UCoastalLookInputComponent::UCoastalLookInputComponent()
{
    PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}
bool UCoastalLookInputComponent::InitializeOptions(UCoastalLocalOptions* Options, UCoastalInteractionBridge* Interaction)
{
    auto* Pawn = Cast<ACharacter>(GetOwner());
    auto* PC = IsValid(Pawn) ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (!IsInGameThread() || bInitialized || bStopped || !IsRegistered() || !IsValid(Options) || !Options->IsInitialized()
        || !IsValid(Pawn) || !IsValid(PC) || !PC->IsLocalController() || PC->GetPawn() != Pawn
        || !IsValid(Interaction) || Interaction->GetOwner() != Pawn || !Interaction->GetCoordinator()) return false;
    TArray<UCoastalLookInputComponent*> Owners; Pawn->GetComponents<UCoastalLookInputComponent>(Owners);
    if (Owners.Num() != 1 || Owners[0] != this) return false;
    TArray<UCameraComponent*> Cameras; Pawn->GetComponents<UCameraComponent>(Cameras);
    Cameras.RemoveAll([](const auto* C) { return !IsValid(C) || !C->IsActive() || !C->IsRegistered(); });
    if (Cameras.Num() != 1 || Cameras[0]->ProjectionMode != ECameraProjectionMode::Perspective
        || PC->GetViewTarget() != Pawn || !Pawn->bFindCameraComponentWhenViewTarget
        || !FMath::IsFinite(Cameras[0]->FieldOfView)) return false;
    Profile = Options; Bridge = Interaction; Character = Pawn; Controller = PC; Camera = Cameras[0];
    OriginalFov = Camera->FieldOfView; LastAppliedFov = OriginalFov;
    bHostLookBound = bUseHostLookEvents; bInitialized = true;
    ApplyCameraOptions(); Synchronize(); return true;
}
bool UCoastalLookInputComponent::IsCameraReady() const
{
    if (!bInitialized || bStopped || !IsValid(Character)) return false;
    TArray<UCameraComponent*> Cameras; Character->GetComponents<UCameraComponent>(Cameras);
    int32 ActiveCount = 0;
    for (const auto* Candidate : Cameras) if (IsValid(Candidate) && Candidate->IsRegistered() && Candidate->IsActive()) ++ActiveCount;
    return ActiveCount == 1 && IsValid(Profile) && IsValid(Character) && IsValid(Controller)
        && Controller->IsLocalController() && Controller->GetPawn() == Character && Controller->GetViewTarget() == Character
        && Character->bFindCameraComponentWhenViewTarget && Camera.IsValid() && Camera->IsRegistered()
        && Camera->IsActive() && Camera->GetOwner() == Character && Camera->ProjectionMode == ECameraProjectionMode::Perspective;
}
bool UCoastalLookInputComponent::IsLookReady() const
{ return IsCameraReady() && bHostLookBound && IsValid(Bridge) && IsValid(Bridge->GetCoordinator()); }
bool UCoastalLookInputComponent::ApplyCameraOptions()
{
    if (!IsInGameThread() || !IsCameraReady()) return false;
    LastAppliedFov = static_cast<float>(Profile->Get().fieldOfView); Camera->SetFieldOfView(LastAppliedFov); return true;
}
void UCoastalLookInputComponent::Synchronize()
{
    const auto* Saves = IsValid(Bridge) ? Bridge->GetCoordinator() : nullptr;
    const bool Allowed = IsLookReady() && Bridge->AllowsWorldInput() && !Controller->IsLookInputIgnored();
    Gate.Synchronize(Allowed, IsValid(Saves) ? Saves->GetSessionEpoch() : 0,
        IsValid(Bridge) ? Bridge->GetInputRevision() : 0, GFrameCounter);
}
bool UCoastalLookInputComponent::SubmitMouseLook(FVector2D MappedDelta)
{
    if (!IsInGameThread() || !IsLookReady() || !coastal::ValidLook(MappedDelta.X, MappedDelta.Y)) return false;
    Synchronize(); if (!Gate.Mouse(GFrameCounter)) return false;
    const auto Delta = coastal::MouseLook(Profile->Get(), MappedDelta.X, MappedDelta.Y);
    Character->AddControllerYawInput(static_cast<float>(Delta.yaw)); Character->AddControllerPitchInput(static_cast<float>(Delta.pitch));
    return true;
}
bool UCoastalLookInputComponent::SubmitStickLook(FVector2D MappedAxis)
{
    if (!IsInGameThread() || !IsLookReady() || !coastal::ValidLook(MappedAxis.X, MappedAxis.Y)) return false;
    Synchronize(); if (!Gate.Stick(MappedAxis.X, MappedAxis.Y, GFrameCounter)) return false;
    const auto Delta = coastal::StickLook(Profile->Get(), MappedAxis.X, MappedAxis.Y, GetWorld()->GetDeltaSeconds());
    Character->AddControllerYawInput(static_cast<float>(Delta.yaw)); Character->AddControllerPitchInput(static_cast<float>(Delta.pitch));
    return true;
}
void UCoastalLookInputComponent::TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{ Super::TickComponent(Delta, TickType, TickFunction); if (bInitialized && !bStopped) Synchronize(); }
void UCoastalLookInputComponent::ReleaseOptions()
{
    // Never reset an FOV another camera owner changed after our last explicit apply.
    if (bInitialized && Camera.IsValid() && FMath::IsNearlyEqual(Camera->FieldOfView, LastAppliedFov))
        Camera->SetFieldOfView(OriginalFov);
    Gate.Synchronize(false, 0, 0, GFrameCounter); bStopped = true; bInitialized = false;
    Profile = nullptr; Bridge = nullptr; Character = nullptr; Controller = nullptr; Camera.Reset();
}
void UCoastalLookInputComponent::EndPlay(const EEndPlayReason::Type Reason)
{ ReleaseOptions(); Super::EndPlay(Reason); }
