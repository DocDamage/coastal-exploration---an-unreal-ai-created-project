#include "CoastalSprintComponent.h"
#include "CoastalLocalOptions.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "CoreGlobals.h"

UCoastalSprintComponent::UCoastalSprintComponent()
{
    PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics; bAutoActivate = true;
}
bool UCoastalSprintComponent::InitializeOptions(UCoastalLocalOptions* Options, UCoastalInteractionBridge* Interaction)
{
    auto* Pawn = Cast<ACharacter>(GetOwner());
    auto* PC = IsValid(Pawn) ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    auto* Move = IsValid(Pawn) ? Pawn->GetCharacterMovement() : nullptr;
    if (!IsInGameThread() || bInitialized || bStopped || !bUseHostSprintEvents || !IsRegistered()
        || !IsComponentTickEnabled() || !GetWorld() || GetWorld()->GetNetMode() != NM_Standalone
        || !IsValid(Options) || !Options->IsInitialized() || !IsValid(Pawn) || !IsValid(PC)
        || !PC->IsLocalController() || PC->GetPawn() != Pawn || !IsValid(Move) || !Move->IsRegistered()
        || !IsValid(Interaction) || Interaction->GetOwner() != Pawn || !IsValid(Interaction->GetCoordinator())
        || !coastal::ValidSprintTuning({WalkSpeed, SprintSpeed}) || !FMath::IsFinite(Move->MinAnalogWalkSpeed)
        || Move->MinAnalogWalkSpeed < 0 || Move->MinAnalogWalkSpeed > WalkSpeed)
    { LastDetail = TEXT("Sprint unavailable: opt into real host input on one registered fixed character; check speed tuning, movement and standalone ownership."); return false; }
    TArray<UCoastalSprintComponent*> Owners; Pawn->GetComponents<UCoastalSprintComponent>(Owners);
    if (Owners.Num() != 1 || Owners[0] != this || !SpeedLease.Acquire(Move->MaxWalkSpeed, {WalkSpeed, SprintSpeed}))
    { LastDetail = TEXT("Sprint unavailable: duplicate speed owner or invalid initial movement speed."); return false; }
    Profile = Options; Bridge = Interaction; Character = Pawn; Controller = PC; Movement = Move;
    bInitialized = true; LastDetail.Empty();
    // Controller input evaluates first; the speed decision precedes character movement physics.
    AddTickPrerequisiteActor(Controller);
    Movement->AddTickPrerequisiteComponent(this);
    Synchronize(); ApplySpeed();
    if (bFailed) { ReleaseOptions(); return false; }
    return true;
}
bool UCoastalSprintComponent::BindingValid() const
{
    return bInitialized && !bStopped && !bFailed && IsRegistered() && IsComponentTickEnabled()
        && IsValid(Profile) && Profile->IsInitialized() && IsValid(Bridge) && IsValid(Bridge->GetCoordinator())
        && IsValid(Character) && GetOwner() == Character && Bridge->GetOwner() == Character
        && IsValid(Controller) && Controller->IsLocalController() && Controller->GetPawn() == Character
        && Character->GetController() == Controller && IsValid(Movement) && Movement->IsRegistered()
        && Character->GetCharacterMovement() == Movement && GetWorld() && GetWorld()->GetNetMode() == NM_Standalone;
}
bool UCoastalSprintComponent::IsSprintReady() const
{ return BindingValid() && SpeedLease.Owns(Movement->MaxWalkSpeed); }
bool UCoastalSprintComponent::SprintPermitted() const
{
    return IsSprintReady() && Bridge->AllowsWorldInput() && !Controller->IsMoveInputIgnored()
        && Movement->IsActive() && Movement->IsComponentTickEnabled()
        && Movement->MovementMode == MOVE_Walking && !Character->bIsCrouched;
}
bool UCoastalSprintComponent::IsSprintRequested() const
{ return SprintPermitted() && Gate.Requested(GFrameCounter); }
void UCoastalSprintComponent::Synchronize()
{
    const auto* Saves = IsValid(Bridge) ? Bridge->GetCoordinator() : nullptr;
    Gate.Synchronize(SprintPermitted(), IsValid(Saves) ? Saves->GetSessionEpoch() : 0,
        IsValid(Bridge) ? Bridge->GetInputRevision() : 0,
        IsValid(Profile) && Profile->Get().sprintToggle, GFrameCounter);
}
void UCoastalSprintComponent::ApplySpeed()
{
    if (!bInitialized || bStopped || bFailed) return;
    if (!BindingValid())
    { StopWithDiagnostic(TEXT("Sprint disabled: fixed character/input lifetime changed. Relaunch after correcting the host binding.")); return; }
    double Next = Movement->MaxWalkSpeed;
    if (!SpeedLease.Apply(Next, IsSprintRequested(), Next))
    { StopWithDiagnostic(TEXT("Sprint disabled: another system changed MaxWalkSpeed. Its value was left untouched; remove the competing writer and relaunch.")); return; }
    Movement->MaxWalkSpeed = static_cast<float>(Next);
}
bool UCoastalSprintComponent::SubmitSprintInput(bool bActionDown)
{
    if (!IsInGameThread() || !bInitialized || bStopped || bFailed) return false;
    Synchronize();
    const bool Accepted = Gate.Sample(bActionDown, GFrameCounter);
    ApplySpeed(); return Accepted && !bFailed;
}
void UCoastalSprintComponent::ApplySprintOptions()
{ if (IsInGameThread() && bInitialized && !bStopped) { Synchronize(); ApplySpeed(); } }
void UCoastalSprintComponent::TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Delta, TickType, TickFunction);
    if (bInitialized && !bStopped && !bFailed) { Synchronize(); ApplySpeed(); }
}
void UCoastalSprintComponent::StopWithDiagnostic(const TCHAR* Detail)
{
    if (bFailed) return;
    bFailed = true; LastDetail = Detail; Gate.Synchronize(false, 0, 0, false, GFrameCounter);
    if (IsValid(Movement))
    {
        double Restored = Movement->MaxWalkSpeed;
        if (SpeedLease.Release(Restored, Restored)) Movement->MaxWalkSpeed = static_cast<float>(Restored);
    }
    UE_LOG(LogTemp, Warning, TEXT("Coastal Sprint: %s"), *LastDetail);
}
void UCoastalSprintComponent::ReleaseOptions()
{
    Gate.Synchronize(false, 0, 0, false, GFrameCounter);
    if (IsValid(Movement))
    {
        double Restored = Movement->MaxWalkSpeed;
        if (SpeedLease.Release(Restored, Restored)) Movement->MaxWalkSpeed = static_cast<float>(Restored);
        Movement->RemoveTickPrerequisiteComponent(this);
    }
    if (IsValid(Controller)) RemoveTickPrerequisiteActor(Controller);
    bStopped = true; bInitialized = false;
    Profile = nullptr; Bridge = nullptr; Character = nullptr; Controller = nullptr; Movement = nullptr;
}
void UCoastalSprintComponent::EndPlay(const EEndPlayReason::Type Reason)
{ ReleaseOptions(); Super::EndPlay(Reason); }
