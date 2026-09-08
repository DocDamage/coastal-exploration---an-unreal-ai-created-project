#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoastalSafetyVolume.generated.h"
class UBoxComponent;
class USceneComponent;
UENUM(BlueprintType)
enum class ECoastalSafetyKind : uint8 { DeepWater, OutOfBounds, DryCheckpoint };
// Authored query-only safety geometry, not a persistent quest object or a focus target.
UCLASS(Blueprintable)
class COASTALFOUNDATION_API ACoastalSafetyVolume : public AActor
{
    GENERATED_BODY()
public:
    ACoastalSafetyVolume();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Safety") TObjectPtr<UBoxComponent> Bounds;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Safety") TObjectPtr<USceneComponent> ReturnPoint;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Safety") ECoastalSafetyKind Kind = ECoastalSafetyKind::DeepWater;
    bool IsAuthoredCorrectly() const;
    bool TouchesCapsule(const FVector& Center, float Radius, float HalfHeight) const;
    bool IsHazard() const { return Kind != ECoastalSafetyKind::DryCheckpoint; }
    FTransform Destination() const;
};
