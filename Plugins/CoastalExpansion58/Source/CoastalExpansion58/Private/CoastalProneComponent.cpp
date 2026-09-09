#include "CoastalProneComponent.h"
#include "CoastalMutableCharacterComponent.h"
#include "CoastalGrappleComponent.h"
#include "CoastalZiplineRiderComponent.h"
#include "CoastalCombatComponent.h"
#include "CoastalCampingActionComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalInteractionBridge.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

namespace
{
const FName ProneTag(TEXT("Coastal.Prone"));
const FVector BodyExtent(110.f, 42.f, 27.f);
FVector BodyCenter(const ACharacter* Character, FVector Location)
{
    return Location + FVector(0,0,30.f - Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}
}

UCoastalProneComponent::UCoastalProneComponent()
{ PrimaryComponentTick.bCanEverTick = true; PrimaryComponentTick.bTickEvenWhenPaused = true; }

void UCoastalProneComponent::BeginPlay()
{
    Super::BeginPlay();
    Character = Cast<ACharacter>(GetOwner());
    Movement = Character ? Character->GetCharacterMovement() : nullptr;
    Bridge = Character ? Character->FindComponentByClass<UCoastalInteractionBridge>() : nullptr;
    Saves = Bridge ? Bridge->GetCoordinator() : nullptr;
    Epoch = Saves ? Saves->GetSessionEpoch() : 0;
    InputRevision = Bridge ? Bridge->GetInputRevision() : 0;
    if (Movement) AddTickPrerequisiteComponent(Movement);
}

bool UCoastalProneComponent::Available() const
{
    auto* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
    auto* Appearance = Character ? Character->FindComponentByClass<UCoastalMutableCharacterComponent>() : nullptr;
    auto* Combat = Character ? Character->FindComponentByClass<UCoastalCombatComponent>() : nullptr;
    auto* Camp = Character ? Character->FindComponentByClass<UCoastalCampingActionComponent>() : nullptr;
    auto* Grapple = Character ? Character->FindComponentByClass<UCoastalGrappleComponent>() : nullptr;
    auto* Zipline = Character ? Character->FindComponentByClass<UCoastalZiplineRiderComponent>() : nullptr;
    return IsRegistered() && GetWorld() && GetWorld()->GetNetMode() == NM_Standalone && !GetWorld()->IsPaused()
        && PC && PC->IsLocalController() && PC->GetPawn() == Character && Movement && Movement->IsMovingOnGround()
        && Saves && Saves->HasActiveCampaign() && !Saves->IsBusy() && !Saves->IsRecoveryRequired()
        && !Saves->IsPlayerReturnActive() && Saves->GetSessionEpoch() == Epoch && Bridge && Bridge->AllowsWorldInput()
        && Appearance && Appearance->IsAppearanceReady() && !Appearance->IsGenerating()
        && Appearance->ActiveAction.IsNone()
        && (!Combat || (!Combat->IsEncounterActive() && !Combat->GetTransientWeapon()))
        && (!Camp || !Camp->IsActionActive()) && (!Grapple || !Grapple->IsGrappleDeployed())
        && (!Zipline || !Zipline->IsRiding())
        && Character->GetMesh()->GetAnimInstance() && !Character->GetMesh()->GetAnimInstance()->IsAnyMontagePlaying();
}

UAnimSequence* UCoastalProneComponent::GetPose() const
{
    const auto* Appearance = Character ? Character->FindComponentByClass<UCoastalMutableCharacterComponent>() : nullptr;
    const auto* Action = Appearance && Appearance->Definition ? Appearance->Definition->Actions.Find(PoseCue) : nullptr;
    return Action ? Action->Clip.Get() : nullptr;
}

bool UCoastalProneComponent::BodyClear(FVector Location, FQuat Rotation) const
{
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CoastalProneBody), false, Character);
    return !GetWorld()->OverlapBlockingTestByChannel(BodyCenter(Character, Location), Rotation, ECC_Pawn,
        FCollisionShape::MakeBox(BodyExtent), Params);
}

void UCoastalProneComponent::SetPose(FName Cue)
{ PoseCue = Cue; PoseTime = 0.f; }

bool UCoastalProneComponent::TryEnter()
{
    if (IsProne() || !Available() || Character->bIsCrouched
        || !Character->GetActorScale3D().Equals(FVector::OneVector,.01f)
        || !BodyClear(Character->GetActorLocation(),Character->GetActorQuat()))
    { LastDetail = TEXT("Find clear ground and free your hands to crawl."); return false; }
    SetPose(TEXT("prone_enter"));
    if (!GetPose()) { PoseCue = NAME_None; LastDetail = TEXT("Crawl animation unavailable."); return false; }
    SavedCrouchHeight = Movement->GetCrouchedHalfHeight();
    SavedCrouchSpeed = Movement->MaxWalkSpeedCrouched; SavedStepHeight = Movement->MaxStepHeight;
    SavedCanCrouch = Movement->GetNavAgentPropertiesRef().bCanCrouch;
    bOwnsTuning = true;
    Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
    Movement->SetCrouchedHalfHeight(FMath::Max(42.f,Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius()));
    Movement->MaxWalkSpeedCrouched = 0.f; Movement->MaxStepHeight = 8.f;
    Character->Crouch(); Movement->Crouch(false);
    if (!Character->bIsCrouched) { Finish(); LastDetail = TEXT("Native crouch could not enter."); return false; }
    Character->Tags.AddUnique(ProneTag);
    Movement->StopMovementImmediately(); Character->ConsumeMovementInputVector();
    Previous = Character->GetActorTransform();
    LastDetail = TEXT("Going prone. C / B stands when clear.");
    return true;
}

bool UCoastalProneComponent::TryStand()
{
    if (!IsProne() || !Movement || PoseCue == TEXT("prone_exit")) return false;
    // UnCrouch performs the engine's standing-capsule test and preserves the floor.
    Character->UnCrouch(); Movement->UnCrouch(false);
    if (Character->bIsCrouched)
    {
        Character->Crouch();
        LastDetail = TEXT("Not enough room to stand. Crawl into clear space.");
        return false;
    }
    Movement->StopMovementImmediately(); Character->ConsumeMovementInputVector();
    SetPose(TEXT("prone_exit")); Previous = Character->GetActorTransform();
    LastDetail = TEXT("Standing up."); return true;
}

void UCoastalProneComponent::RestoreTuning()
{
    if (!bOwnsTuning || !Movement) return;
    Movement->SetCrouchedHalfHeight(SavedCrouchHeight); Movement->MaxWalkSpeedCrouched = SavedCrouchSpeed;
    Movement->MaxStepHeight = SavedStepHeight; Movement->GetNavAgentPropertiesRef().bCanCrouch = SavedCanCrouch;
    bOwnsTuning = false;
}

void UCoastalProneComponent::Finish()
{
    RestoreTuning(); PoseCue = NAME_None; PoseTime = 0.f;
    if (Character) Character->Tags.Remove(ProneTag);
}

FTransform UCoastalProneComponent::StandingSaveTransform() const
{
    FTransform Result = Character->GetActorTransform();
    if (Character->bIsCrouched)
    {
        const float Standing = Character->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
        Result.AddToTranslation(FVector(0,0,Standing-Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
    }
    return Result;
}

void UCoastalProneComponent::PrepareStandingPlacement()
{
    // Called only by save/recovery after a full standing destination passed its
    // collision check. Reset native geometry without moving the placement origin.
    Character->UnCrouch();
    if (Character->bIsCrouched) { Movement->UnCrouch(true); Character->SetIsCrouched(false); }
    Finish();
}

void UCoastalProneComponent::ConstrainBody()
{
    const FVector Start = Previous.GetLocation(), End = Character->GetActorLocation();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CoastalProneSweep), false, Character);
    FHitResult Hit;
    const bool Blocked = GetWorld()->SweepSingleByChannel(Hit,BodyCenter(Character,Start),BodyCenter(Character,End),
        Previous.GetRotation(),ECC_Pawn,FCollisionShape::MakeBox(BodyExtent),Params);
    const bool TurnBlocked = !BodyClear(End,Character->GetActorQuat());
    if (Blocked || TurnBlocked)
    {
        const FVector Safe = Blocked && !Hit.bStartPenetrating
            ? FMath::Lerp(Start,End,FMath::Max(0.f,Hit.Time-.01f)) : Start;
        FHitResult Correction;
        Movement->SafeMoveUpdatedComponent(Safe-End,Previous.GetRotation(),true,Correction);
        Movement->StopMovementImmediately();
        LastDetail = TEXT("Crawl path blocked.");
    }
}

void UCoastalProneComponent::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    if (!Character || !Movement || !Saves) return;
    auto* PC = Cast<APlayerController>(Character->GetController());
    const bool Held = PC && (PC->IsInputKeyDown(EKeys::C) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Right));
    if (Bridge && InputRevision != Bridge->GetInputRevision())
    { InputRevision = Bridge->GetInputRevision(); bNeedsRelease = Held; }
    if (!Held) bNeedsRelease = false;
    const bool Allowed = Available();
    if (Saves->GetSessionEpoch() != Epoch || Saves->IsPlayerReturnActive())
    {
        if (IsProne()) { Character->UnCrouch(); Movement->UnCrouch(false); if (!Character->bIsCrouched) Finish(); }
        Epoch = Saves->GetSessionEpoch(); bNeedsRelease = Held;
    }
    if (Held && !bNeedsRelease)
    {
        bNeedsRelease = true;
        if (Allowed) { if (IsProne()) TryStand(); else TryEnter(); }
    }
    if (!IsProne()) return;
    if (!Allowed)
    {
        bNeedsRelease |= Held;
        // Menus retain the safe low capsule when standing is obstructed.
        Movement->StopMovementImmediately(); Character->ConsumeMovementInputVector();
        if (!GetWorld()->IsPaused() && !Saves->IsBusy())
        { Character->UnCrouch(); Movement->UnCrouch(false); if (!Character->bIsCrouched) Finish(); }
        Previous = Character->GetActorTransform(); return;
    }
    PoseTime += Delta;
    auto* Clip = GetPose();
    const bool Done = !Clip || PoseTime >= Clip->GetPlayLength();
    const bool Transition = PoseCue == TEXT("prone_enter") || PoseCue == TEXT("prone_exit");
    if (Transition)
    {
        FHitResult Hit;
        Movement->SafeMoveUpdatedComponent(Previous.GetLocation()-Character->GetActorLocation(),Previous.GetRotation(),true,Hit);
        Movement->StopMovementImmediately(); Character->ConsumeMovementInputVector();
        if (Done) { if (PoseCue == TEXT("prone_exit")) Finish(); else SetPose(TEXT("prone_idle")); }
    }
    else
    {
        ConstrainBody();
        const bool Moving = Movement->Velocity.SizeSquared2D() > 16.f;
        const FString Cue = PoseCue.ToString();
        const TCHAR* Side = bRight ? TEXT("r") : TEXT("l");
        if (Moving && (PoseCue == TEXT("prone_idle") || Cue.StartsWith(TEXT("prone_stop"))))
            SetPose(FName(*(FString(TEXT("prone_start_"))+Side)));
        else if (!Moving && (Cue.StartsWith(TEXT("prone_start")) || Cue.StartsWith(TEXT("prone_loop"))))
            SetPose(FName(*(FString(TEXT("prone_stop_"))+Side)));
        else if (Done && Cue.StartsWith(TEXT("prone_start"))) SetPose(FName(*(FString(TEXT("prone_loop_"))+Side)));
        else if (Done && Cue.StartsWith(TEXT("prone_loop"))) { bRight=!bRight; SetPose(bRight?TEXT("prone_loop_r"):TEXT("prone_loop_l")); }
        else if (Done && Cue.StartsWith(TEXT("prone_stop"))) SetPose(TEXT("prone_idle"));
    }
    if (IsProne()) Movement->MaxWalkSpeedCrouched = PoseCue == TEXT("prone_enter") ? 0.f : 65.f;
    Previous = Character->GetActorTransform();
}

void UCoastalProneComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsProne() && Character && Movement) { Character->UnCrouch(); Movement->UnCrouch(false); }
    Finish(); Super::EndPlay(Reason);
}
