#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalZiplineRiderComponent.generated.h"

class ACharacter;
class APlayerController;
class ACoastalZipline;
class UCharacterMovementComponent;
class UCoastalInteractionBridge;
class UCoastalSaveCoordinator;
class UEnhancedInputComponent;
class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;

UCLASS(ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALEXPANSION58_API UCoastalZiplineRiderComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalZiplineRiderComponent();
    UFUNCTION(BlueprintPure) bool IsRiding() const { return ActiveLine.IsValid(); }
    UFUNCTION(BlueprintCallable) bool TryBoard(ACoastalZipline* Line);
    UFUNCTION(BlueprintCallable) void Release();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString LastDetail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Progress = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Speed = 0.f;
    UPROPERTY(Transient, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Trolley;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function) override;
private:
    bool Available() const;
    void InputRide();
    void RemoveInput();
    void Stop(bool Momentum);
    UPROPERTY(Transient) TObjectPtr<ACharacter> Character;
    UPROPERTY(Transient) TObjectPtr<UCharacterMovementComponent> Movement;
    UPROPERTY(Transient) TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY(Transient) TObjectPtr<UCoastalSaveCoordinator> Saves;
    UPROPERTY(Transient) TObjectPtr<UEnhancedInputComponent> RideInput;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> RideContext;
    UPROPERTY(Transient) TObjectPtr<UInputAction> RideAction;
    TWeakObjectPtr<APlayerController> InputController;
    TWeakObjectPtr<ACoastalZipline> ActiveLine;
    FVector TravelVelocity = FVector::ZeroVector;
    uint64 Epoch = 0;
    bool bNeedsRelease = false;
};
