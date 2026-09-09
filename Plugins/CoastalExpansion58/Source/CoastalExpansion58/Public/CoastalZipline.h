#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RopeComponent.h"
#include "CoastalZipline.generated.h"

class ACoastalGrappleAnchor;
class UStaticMeshComponent;

UCLASS()
class COASTALEXPANSION58_API UCoastalZiplineRope : public URopeComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient, BlueprintReadOnly)
    TWeakObjectPtr<USceneComponent> Endpoint;
    virtual bool CanWrapTarget(const USceneComponent* Mesh, FName Bone) const override;
};

// The actual plugin owns the cable's simulation and rendering. Riders sample it.
UCLASS(Blueprintable)
class COASTALEXPANSION58_API ACoastalZipline : public AActor
{
    GENERATED_BODY()
public:
    ACoastalZipline();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> StartAnchor;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCoastalZiplineRope> Cable;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly) TObjectPtr<ACoastalGrappleAnchor> EndAnchor;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString LastDetail;
    UFUNCTION(BlueprintPure) bool IsReady() const;
    bool Sample(float Fraction, FVector& Point, FVector& Tangent, float& Length) const;
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
private:
    double RetryAt = 0.0;
    bool bRequested = false;
    bool bTensioned = false;
    bool bPrepared = false;
    bool bPreparedEndpoint = false;
    bool bWorldCollisionConfigured = false;
};
