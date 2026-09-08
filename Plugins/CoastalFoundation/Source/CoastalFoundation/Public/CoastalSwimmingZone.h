#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PhysicsVolume.h"
#include "CoastalSwimmingZone.generated.h"

class ACharacter;

// A deliberately authored, bounded water volume for surface swimming.
UCLASS(Blueprintable)
class COASTALFOUNDATION_API ACoastalSwimmingZone : public APhysicsVolume
{
    GENERATED_BODY()

public:
    ACoastalSwimmingZone(const FObjectInitializer& ObjectInitializer);

    // Must match the top face of the authored volume brush.
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Coastal|Swimming")
    float WaterSurfaceZ = 0.0f;

    // A deeper capsule center returns through the existing safety owner.
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Coastal|Swimming",
        meta=(ClampMin="50", ClampMax="500"))
    float MaxRecoveryExemptionDepthCm = 140.0f;

    UFUNCTION(BlueprintPure, Category="Coastal|Swimming")
    bool IsAuthoredCorrectly() const;

    bool IsCurrentWaterVolumeFor(const ACharacter* Character) const;
    bool OverlapsCharacter(const ACharacter* Character) const;
    bool IsWithinRecoveryDepth(const ACharacter* Character) const;

    virtual bool IsOverlapInVolume(const USceneComponent& TestComponent) const override;
    virtual void ActorEnteredVolume(AActor* Other) override;
    virtual void ActorLeavingVolume(AActor* Other) override;
};
