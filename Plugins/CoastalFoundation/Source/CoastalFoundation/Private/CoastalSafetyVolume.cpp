#include "CoastalSafetyVolume.h"
#include "Core/RecoveryRules.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

ACoastalSafetyVolume::ACoastalSafetyVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SafetyBounds")); SetRootComponent(Bounds);
    Bounds->SetBoxExtent(FVector(150, 150, 100));
    // Explicit geometry queries do not depend on overlap-event ordering or collision presets.
    Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision); Bounds->SetGenerateOverlapEvents(false);
    Bounds->SetHiddenInGame(true);
    ReturnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ReturnPoint")); ReturnPoint->SetupAttachment(Bounds);
}
bool ACoastalSafetyVolume::IsAuthoredCorrectly() const
{
    if (!IsValid(Bounds) || !IsValid(ReturnPoint) || GetActorTransform().ContainsNaN()
        || (Kind != ECoastalSafetyKind::DeepWater && Kind != ECoastalSafetyKind::OutOfBounds
            && Kind != ECoastalSafetyKind::DryCheckpoint)) return false;
    const FTransform T = Bounds->GetComponentTransform();
    const FVector S = T.GetScale3D(), E = Bounds->GetScaledBoxExtent();
    return !T.ContainsNaN() && T.GetRotation().IsNormalized() && S.X > 0 && S.Y > 0 && S.Z > 0
        && T.GetRotation().GetUpVector().Equals(FVector::UpVector, 0.001)
        && coastal::ValidSafetyBox(E.X, E.Y, E.Z);
}
bool ACoastalSafetyVolume::TouchesCapsule(const FVector& Center, float Radius, float HalfHeight) const
{
    if (!IsAuthoredCorrectly()) return false;
    const FVector P = Bounds->GetComponentTransform().InverseTransformPositionNoScale(Center);
    const FVector E = Bounds->GetScaledBoxExtent();
    return coastal::SafetyCapsuleOverlap(P.X, P.Y, P.Z, Radius, HalfHeight, E.X, E.Y, E.Z);
}
FTransform ACoastalSafetyVolume::Destination() const
{
    // ReturnPoint position/rotation are world-space; volume scale is not character scale.
    return FTransform(ReturnPoint->GetComponentQuat(), ReturnPoint->GetComponentLocation(), FVector::OneVector);
}
