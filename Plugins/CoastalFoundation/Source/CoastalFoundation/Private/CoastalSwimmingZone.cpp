#include "CoastalSwimmingZone.h"

#include "CoastalSwimmingComponent.h"
#include "Core/SwimmingRules.h"
#include "Components/BrushComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/BodySetup.h"

ACoastalSwimmingZone::ACoastalSwimmingZone(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bWaterVolume = true;
    bPhysicsOnContact = true;
    FluidFriction = 0.5f;
    Priority = 20;
    UBrushComponent* VolumeBrush = GetBrushComponent();
    VolumeBrush->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    VolumeBrush->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    VolumeBrush->SetGenerateOverlapEvents(true);
    VolumeBrush->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

bool ACoastalSwimmingZone::IsAuthoredCorrectly() const
{
    UBrushComponent* VolumeBrush = GetBrushComponent();
    const UBodySetup* BodySetup = IsValid(VolumeBrush) ? VolumeBrush->GetBodySetup() : nullptr;
    const bool bRequiredCollision = IsValid(VolumeBrush)
        && coastal::HasRequiredVolumeCollision(
            VolumeBrush->GetCollisionProfileName() == FName(TEXT("OverlapAllDynamic")),
            VolumeBrush->GetCollisionEnabled() == ECollisionEnabled::QueryOnly,
            VolumeBrush->GetGenerateOverlapEvents(),
            VolumeBrush->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap);
    if (!IsValid(VolumeBrush) || !IsValid(VolumeBrush->Brush) || !IsValid(BodySetup)
        || BodySetup->AggGeom.GetElementCount() <= 0 || !bWaterVolume || !bPhysicsOnContact
        || !bRequiredCollision
        || !FMath::IsFinite(WaterSurfaceZ) || !FMath::IsFinite(MaxRecoveryExemptionDepthCm)
        || MaxRecoveryExemptionDepthCm < 50.0f || MaxRecoveryExemptionDepthCm > 500.0f)
        return false;

    const FBox Bounds = VolumeBrush->Bounds.GetBox();
    const FVector Size = Bounds.GetSize();
    return Bounds.IsValid && Size.X >= 100.0 && Size.Y >= 100.0
        && Size.Z >= MaxRecoveryExemptionDepthCm
        && FMath::Abs(Bounds.Max.Z - WaterSurfaceZ) <= 5.0f;
}

bool ACoastalSwimmingZone::IsCurrentWaterVolumeFor(const ACharacter* Character) const
{
    const UCharacterMovementComponent* Movement = IsValid(Character)
        ? Character->GetCharacterMovement() : nullptr;
    return IsAuthoredCorrectly() && IsValid(Movement)
        && Movement->GetPhysicsVolume() == this;
}

bool ACoastalSwimmingZone::OverlapsCharacter(const ACharacter* Character) const
{
    const UCapsuleComponent* Capsule = IsValid(Character) ? Character->GetCapsuleComponent() : nullptr;
    const UBrushComponent* VolumeBrush = GetBrushComponent();
    if (!IsAuthoredCorrectly() || !IsValid(Capsule) || !IsValid(VolumeBrush)
        || !IsOverlapInVolume(*Capsule)) return false;
    return VolumeBrush->OverlapComponent(Capsule->GetComponentLocation(), Capsule->GetComponentQuat(),
        FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),
            Capsule->GetScaledCapsuleHalfHeight()));
}

bool ACoastalSwimmingZone::IsOverlapInVolume(const USceneComponent& TestComponent) const
{
    if (!Super::IsOverlapInVolume(TestComponent)) return false;
    const ACharacter* Character = Cast<ACharacter>(TestComponent.GetOwner());
    if (!Character || Character->GetCapsuleComponent() != &TestComponent) return true;
    UBrushComponent* VolumeBrush = GetBrushComponent();
    if (!IsValid(VolumeBrush) || !FMath::IsFinite(WaterSurfaceZ)) return false;
    // Physics-on-contact keeps a surface swimmer afloat with its center above
    // the water. At a lateral seam, however, a capsule edge can touch the next
    // volume while its vertical centerline still misses that brush. Unreal's
    // ImmersionDepth treats such a missed trace as full immersion, injecting
    // an upward impulse. Keep the previous overlapping volume until the
    // centerline actually enters this authored water surface.
    const FVector Position = TestComponent.GetComponentLocation();
    FHitResult Hit;
    const FCollisionQueryParams Params(SCENE_QUERY_STAT(CoastalWaterSurfaceFootprint), true);
    return VolumeBrush->LineTraceComponent(Hit,
        FVector(Position.X, Position.Y, WaterSurfaceZ + 2.0),
        FVector(Position.X, Position.Y, WaterSurfaceZ - 2.0), Params);
}

bool ACoastalSwimmingZone::IsWithinRecoveryDepth(const ACharacter* Character) const
{
    const UCapsuleComponent* Capsule = IsValid(Character) ? Character->GetCapsuleComponent() : nullptr;
    if (!IsCurrentWaterVolumeFor(Character) || !IsValid(Capsule)) return false;
    const float CenterZ = Character->GetActorLocation().Z;
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    return FMath::IsFinite(CenterZ) && FMath::IsFinite(HalfHeight) && HalfHeight > 0.0f
        && CenterZ <= WaterSurfaceZ + HalfHeight + 5.0f
        && CenterZ >= WaterSurfaceZ - MaxRecoveryExemptionDepthCm;
}

void ACoastalSwimmingZone::ActorEnteredVolume(AActor* Other)
{
    Super::ActorEnteredVolume(Other);
    if (ACharacter* Character = Cast<ACharacter>(Other))
        if (UCoastalSwimmingComponent* Swimming = Character->FindComponentByClass<UCoastalSwimmingComponent>())
            Swimming->NotifyZoneEntered(this);
}

void ACoastalSwimmingZone::ActorLeavingVolume(AActor* Other)
{
    if (ACharacter* Character = Cast<ACharacter>(Other))
        if (UCoastalSwimmingComponent* Swimming = Character->FindComponentByClass<UCoastalSwimmingComponent>())
            Swimming->NotifyZoneLeft(this);
    Super::ActorLeavingVolume(Other);
}
