#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalCharacterStance.h"
#include "CoastalProneComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UCoastalSaveCoordinator;
class UCoastalInteractionBridge;
class UAnimSequence;

UCLASS(ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALEXPANSION58_API UCoastalProneComponent : public UActorComponent, public ICoastalCharacterStance
{
    GENERATED_BODY()
public:
    UCoastalProneComponent();
    UFUNCTION(BlueprintPure) bool IsProne() const { return !PoseCue.IsNone(); }
    UFUNCTION(BlueprintCallable) bool TryEnter();
    UFUNCTION(BlueprintCallable) bool TryStand();
    UFUNCTION(BlueprintPure) UAnimSequence* GetPose() const;
    virtual bool IsStanceActive() const override { return IsProne(); }
    virtual FTransform StandingSaveTransform() const override;
    virtual void PrepareStandingPlacement() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName PoseCue;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString LastDetail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float PoseTime = 0.f;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function) override;
private:
    bool Available() const;
    bool BodyClear(FVector Location, FQuat Rotation) const;
    void SetPose(FName Cue);
    void RestoreTuning();
    void Finish();
    void ConstrainBody();
    UPROPERTY(Transient) TObjectPtr<ACharacter> Character;
    UPROPERTY(Transient) TObjectPtr<UCharacterMovementComponent> Movement;
    UPROPERTY(Transient) TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY(Transient) TObjectPtr<UCoastalSaveCoordinator> Saves;
    FTransform Previous;
    uint64 Epoch = 0;
    uint64 InputRevision = 0;
    float SavedCrouchHeight = 0, SavedCrouchSpeed = 0, SavedStepHeight = 0;
    bool SavedCanCrouch = false, bOwnsTuning = false, bNeedsRelease = true, bRight = false;
};
