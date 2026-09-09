#pragma once
#include "CoreMinimal.h"
#include "RopeComponent.h"
#include "Gameplay/RopeWielderComponent.h"
#include "CoastalGrappleComponent.generated.h"

class ACharacter;
class UCoastalInteractionBridge;
class UCoastalSaveCoordinator;

// Keep the plugin's solver, render and movement constraint authoritative.
UCLASS()
class COASTALEXPANSION58_API UCoastalGrappleRope : public URopeComponent
{
    GENERATED_BODY()
public:
    UCoastalGrappleRope();
    virtual bool CanWrapTarget(const USceneComponent* Mesh, FName Bone) const override;
};

UCLASS(ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALEXPANSION58_API UCoastalGrappleComponent : public URopeWielderComponent
{
    GENERATED_BODY()
public:
    UCoastalGrappleComponent();
    UFUNCTION(BlueprintPure) bool IsGrappleAvailable() const;
    UFUNCTION(BlueprintPure) bool IsGrappleDeployed() const;
    UFUNCTION(BlueprintCallable) void CancelGrapple();
    void RefreshHandAttachment();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString LastDetail;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function) override;
protected:
    virtual bool CanThrow() const override;
    virtual void NotifyThrown() override;
    virtual void NotifyThrowRejected(ERopeThrowRejectReason Reason) override;
private:
    UPROPERTY(Transient) TObjectPtr<ACharacter> Character;
    UPROPERTY(Transient) TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY(Transient) TObjectPtr<UCoastalSaveCoordinator> Saves;
    uint64 Epoch = 0;
    bool bWorldCollisionConfigured = false;
    bool bOwnsRope = false;
};
