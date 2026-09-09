#include "CoastalPlacementLibrary.h"
#include "CoastalCharacterStance.h"
#include "CoastalSafetyVolume.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

bool UCoastalPlacementLibrary::IsDryDestination(ACharacter* Player, FTransform Transform)
{
    if (!IsInGameThread() || !IsValid(Player) || !Player->GetWorld() || Transform.ContainsNaN()
        || !Transform.GetRotation().IsNormalized()
        || !Transform.GetRotation().GetUpVector().Equals(FVector::UpVector, 0.001) || !Transform.GetScale3D().Equals(FVector::OneVector, 0.001)
        || Transform.GetLocation().GetAbsMax() > 10000000.0) return false;
    const auto* Capsule = Player->GetCapsuleComponent();
    if (!Capsule || !Player->GetCharacterMovement()) return false;
    bool StanceActive = false;
    for (auto* Component : Player->GetComponents())
        if (auto* Stance = Cast<ICoastalCharacterStance>(Component)) StanceActive |= Stance->IsStanceActive();
    const auto* Standing = StanceActive ? Player->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent() : Capsule;
    const float Radius = Standing->GetUnscaledCapsuleRadius() * Capsule->GetShapeScale();
    const float HalfHeight = Standing->GetUnscaledCapsuleHalfHeight() * Capsule->GetShapeScale();
    if (!FMath::IsFinite(Radius) || !FMath::IsFinite(HalfHeight) || Radius <= 0 || HalfHeight < Radius) return false;
    if (const auto* Recovery = Player->FindComponentByClass<UCoastalPlayerRecoveryComponent>())
        if (!Recovery->SettingsValid() || Recovery->BelowBoundary(Transform.GetLocation())) return false;
    for (TActorIterator<ACoastalSafetyVolume> It(Player->GetWorld()); It; ++It)
        if (!It->IsAuthoredCorrectly() || (It->IsHazard()
            && It->TouchesCapsule(Transform.GetLocation(), Radius, HalfHeight))) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CoastalSafeDestination), false, Player);
    const FVector Position = Transform.GetLocation();
    if (Player->GetWorld()->OverlapBlockingTestByProfile(Position, FQuat::Identity, TEXT("Pawn"),
        FCollisionShape::MakeCapsule(Radius, FMath::Max(Radius, HalfHeight - 1.0f)), Params)) return false;
    FHitResult Floor;
    const FVector Bottom = Position - FVector(0, 0, HalfHeight);
    if (!Player->GetWorld()->LineTraceSingleByChannel(Floor, Bottom + FVector(0, 0, 20),
        Bottom - FVector(0, 0, 100), ECC_Visibility, Params)) return false;
    return Floor.bBlockingHit && Floor.ImpactNormal.Z >= Player->GetCharacterMovement()->GetWalkableFloorZ()
        && (!Floor.GetActor() || !Floor.GetActor()->ActorHasTag(TEXT("Coastal.UnsafeCheckpoint")));
}

FTransform UCoastalPlacementLibrary::StanceSaveTransform(ACharacter* Character, const FTransform& DryFallback)
{
    for (auto* Component : Character->GetComponents())
        if (auto* Stance = Cast<ICoastalCharacterStance>(Component); Stance && Stance->IsStanceActive())
        {
            const auto Standing = Stance->StandingSaveTransform();
            return IsDryDestination(Character,Standing) ? Standing : DryFallback;
        }
    return Character->GetActorTransform();
}

void UCoastalPlacementLibrary::PrepareStandingPlacement(ACharacter* Character)
{
    for (auto* Component : Character->GetComponents())
        if (auto* Stance = Cast<ICoastalCharacterStance>(Component); Stance && Stance->IsStanceActive())
            Stance->PrepareStandingPlacement();
}
