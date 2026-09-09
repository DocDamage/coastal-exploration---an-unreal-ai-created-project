#include "CoastalMutableCharacterComponent.h"
#include "CoastalCombatComponent.h"
#include "CoastalGrappleComponent.h"
#include "CoastalZiplineRiderComponent.h"
#include "CoastalProneComponent.h"
#include "CoastalSaveCoordinator.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "CoastalRetargetAnimInstance.h"
#include "Core/CharacterActionRules.h"
#include "WeaponBase.h"

namespace
{
FName DirectionalCue(FName Cue, const FVector& Local)
{
    using D = coastal::ActionDirection;
    const D Direction = coastal::CharacterActionDirection(Local.X, Local.Y);
    const TCHAR* Suffix = Direction == D::Right ? TEXT("right") : Direction == D::Back ? TEXT("back")
        : Direction == D::Left ? TEXT("left") : TEXT("front");
    if (Cue == TEXT("player_hit") && Direction == D::Front) return Cue;
    return FName(*(Cue.ToString() + TEXT("_") + Suffix));
}
}

void UCoastalMutableCharacterComponent::InteractionFeedback(ECoastalActionResult Result, FName Cue, FVector)
{
    if (Result != ECoastalActionResult::Applied || !bReady || bEditing || !Definition || !Definition->Actions.Contains(Cue)) return;
    QueuedAction = Cue; QueuedUntil = GetWorld()->GetTimeSeconds() + 0.3;
}

void UCoastalMutableCharacterComponent::CombatFeedback(FName Cue, FVector Location)
{
    // Weapon lifecycle is observed when presentation is permitted. A checkpoint
    // save or initial generation must not discard a short-lived equip event.
    if (Cue == TEXT("pistol_equip") || Cue == TEXT("pistol_unequip")) return;
    if (Character && (Cue == TEXT("player_hit") || Cue == TEXT("pistol_shoot")))
        Cue = DirectionalCue(Cue, Character->GetActorTransform().InverseTransformVectorNoScale(Location - Character->GetActorLocation()));
    if (!bReady || bEditing || !Definition || !Definition->Actions.Contains(Cue)) return;
    if (QueuedAction.ToString().StartsWith(TEXT("player_hit")) && !Cue.ToString().StartsWith(TEXT("player_hit"))) return;
    // Deliver an accepted combat receipt on the next animation tick even if
    // asset streaming stalls that frame. Lifecycle interruption still clears it.
    QueuedAction = Cue; QueuedUntil = TNumericLimits<double>::Max();
}

void UCoastalMutableCharacterComponent::StopAction(bool ClearQueued)
{
    for (int32 Index = 0; Index < Parts.Num() && Index < ActionMontages.Num(); ++Index)
        if (auto* Anim = Parts[Index]->GetAnimInstance()) Anim->Montage_Stop(0.15f, ActionMontages[Index]);
    ActionMontages.Reset(); bActionUpperBody = false;
    ActiveAction = NAME_None; ActionUntil = 0.0;
    if (ClearQueued) QueuedAction = NAME_None;
}

void UCoastalMutableCharacterComponent::UpdateWeaponPose(bool Allowed)
{
    FName Cue = TEXT("pistol_hold");
    const bool Armed = Combat && IsValid(Combat->GetTransientWeapon());
    if (Armed && Combat->IsAiming())
        Cue = DirectionalCue(TEXT("pistol_aim"), Character->GetActorTransform().InverseTransformVectorNoScale(Character->GetControlRotation().Vector()));
    const auto* Pose = Definition ? Definition->Actions.Find(Cue) : nullptr;
    // Unequip retains a base pose for its upper-body slot after the weapon retires.
    const bool Enabled = Allowed && (Armed || bActionUpperBody) && Pose && Pose->Clip;
    auto* Grapple = Character ? Character->FindComponentByClass<UCoastalGrappleComponent>() : nullptr;
    auto* Zipline = Character ? Character->FindComponentByClass<UCoastalZiplineRiderComponent>() : nullptr;
    const bool Riding = Zipline && Zipline->IsRiding();
    const auto* Prone = Character ? Character->FindComponentByClass<UCoastalProneComponent>() : nullptr;
    auto* PronePose = Prone && Prone->IsProne() ? Prone->GetPose() : nullptr;
    const auto* Hang = Definition ? Definition->Actions.Find(Riding ? TEXT("zipline_ride") : TEXT("grapple_hang")) : nullptr;
    const bool Hanging = Allowed && !Armed && Hang && Hang->Clip && (Riding
        || (Grapple && Grapple->IsGrappleAvailable() && Grapple->IsGrappleDeployed() && Grapple->IsHangingOnRope()));
    for (const auto& Part : Parts)
        if (auto* Anim = Cast<UCoastalRetargetAnimInstance>(Part->GetAnimInstance()))
        {
            const bool Compatible = Enabled && Part->GetSkeletalMeshAsset()
                && Part->GetSkeletalMeshAsset()->GetSkeleton() == Pose->Clip->GetSkeleton();
            Anim->WeaponPose = Compatible ? Pose->Clip.Get() : nullptr;
            Anim->WeaponWeight = Compatible ? 1.f : 0.f;
            Anim->bWeaponPoseLooping = Cue == TEXT("pistol_hold");
            Anim->ActivityPose = Hanging && Part->GetSkeletalMeshAsset()
                && Part->GetSkeletalMeshAsset()->GetSkeleton() == Hang->Clip->GetSkeleton()
                ? Hang->Clip.Get() : nullptr;
            Anim->bActivityTimeDriven = PronePose != nullptr;
            if (PronePose)
            {
                Anim->ActivityPose = PronePose;
                Anim->ActivityTime = Prone->PoseCue == TEXT("prone_idle")
                    ? FMath::Fmod(Prone->PoseTime, PronePose->GetPlayLength()) : Prone->PoseTime;
            }
        }
}

void UCoastalMutableCharacterComponent::UpdateActions()
{
    if (!Combat && Character)
    {
        Combat = Character->FindComponentByClass<UCoastalCombatComponent>();
        if (Combat) CombatHandle = Combat->OnCombatPresentation.AddUObject(this, &UCoastalMutableCharacterComponent::CombatFeedback);
    }
    auto* Move = Character ? Character->GetCharacterMovement() : nullptr;
    auto* Zipline = Character ? Character->FindComponentByClass<UCoastalZiplineRiderComponent>() : nullptr;
    const bool Allowed = BindingValid() && bReady && !bEditing && !bGenerating && Bridge->AllowsWorldInput()
        && Move && (Move->IsMovingOnGround() || Move->IsFalling() || (Zipline && Zipline->IsRiding()))
        && Character->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationBlueprint
        && !GetWorld()->IsPaused();
    if (!Allowed)
    {
        StopAction(); UpdateWeaponPose(false);
        if (Combat) Combat->InterruptAim();
        return;
    }
    AActor* CurrentWeapon = Combat ? Combat->GetTransientWeapon() : nullptr;
    if ((CurrentWeapon && (!bPresentedWeapon || PresentedWeapon.Get() != CurrentWeapon))
        || (!CurrentWeapon && bPresentedWeapon))
    {
        if (QueuedAction.IsNone())
        {
            QueuedAction = CurrentWeapon ? TEXT("pistol_equip") : TEXT("pistol_unequip");
            QueuedUntil = GetWorld()->GetTimeSeconds() + 0.3;
        }
        PresentedWeapon = CurrentWeapon; bPresentedWeapon = CurrentWeapon != nullptr;
    }
    UpdateWeaponPose(true);
    if (Character->ActorHasTag(TEXT("Coastal.Prone"))) { StopAction(); return; }
    const bool Stationary = Move->IsMovingOnGround() && Move->Velocity.SizeSquared2D() < 100.f
        && Move->GetCurrentAcceleration().IsNearlyZero();
    if (!ActiveAction.IsNone() && !bActionUpperBody && !Stationary) StopAction(false);
    const double Now = GetWorld()->GetTimeSeconds();
    if (!ActiveAction.IsNone() && Now >= ActionUntil) StopAction(false);
    if (QueuedAction.IsNone()) return;
    const FName Requested = QueuedAction; QueuedAction = NAME_None;
    const auto* Action = Definition->Actions.Find(Requested);
    const bool UseUpperBody = Action && (Action->bUpperBody
        || (!Stationary && Requested.ToString().StartsWith(TEXT("player_hit"))));
    if (Now > QueuedUntil || !Action || !Action->Clip || Action->Clip->bEnableRootMotion
        || (!UseUpperBody && !Stationary)
        || !FMath::IsFinite(Action->DurationSeconds) || Action->DurationSeconds <= 0.f || Action->DurationSeconds > 5.f
        || !FMath::IsFinite(Action->StartSeconds) || Action->StartSeconds < 0.f || Action->StartSeconds >= Action->Clip->GetPlayLength()) return;
    for (const auto& Part : Parts)
        if (!Part->GetAnimInstance() || !Part->GetSkeletalMeshAsset()
            || Part->GetSkeletalMeshAsset()->GetSkeleton() != Action->Clip->GetSkeleton()) return;
    StopAction();
    bool Played = true;
    for (const auto& Part : Parts)
    {
        auto* Montage = Part->GetAnimInstance()->PlaySlotAnimationAsDynamicMontage(Action->Clip,
            UseUpperBody ? TEXT("CoastalUpperBody") : TEXT("CoastalAction"),
            0.08f, 0.15f, 1.f, 1, -1.f, Action->StartSeconds);
        ActionMontages.Add(Montage); Played &= Montage != nullptr;
    }
    if (!Played)
    {
        StopAction();
        return;
    }
    ActiveAction = Requested; ++ActionPlayCount;
    bActionUpperBody = UseUpperBody;
    UpdateWeaponPose(true);
    ActionUntil = Now + FMath::Min(Action->DurationSeconds, Action->Clip->GetPlayLength() - Action->StartSeconds);
}
