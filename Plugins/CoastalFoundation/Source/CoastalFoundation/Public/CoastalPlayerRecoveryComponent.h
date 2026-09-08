#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/RecoveryRules.h"
#include "CoastalPlayerRecoveryComponent.generated.h"
class UEnhancedInputComponent;
class ACharacter;
class APlayerController;
class APlayerCameraManager;
class ACoastalSafetyVolume;
class UCoastalSaveCoordinator;
class UCoastalInteractionBridge;
class UCoastalSwimmingComponent;
UENUM(BlueprintType)
enum class ECoastalReturnNotice : uint8 { Returning, Returned, CheckpointRecorded, RestartRequired };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoastalReturnNotice, ECoastalReturnNotice, Result, FString, Detail);
// Optional on the possessed Character. No inventory, input mapping, world-focus or swimming system.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalPlayerRecoveryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalPlayerRecoveryComponent();
    UFUNCTION(BlueprintCallable, Category="Coastal|Safety")
    bool InitializeRecovery(UCoastalSaveCoordinator* Coordinator, UCoastalInteractionBridge* Interaction,
        FTransform InitialDryFallback);
    UFUNCTION(BlueprintPure, Category="Coastal|Safety") bool IsInitialized() const { return bInitialized && !bStopped; }
    UFUNCTION(BlueprintPure, Category="Coastal|Safety") bool IsReturning() const { return Flow.Active(); }
    // Uses the existing exclusive return flow; combat never teleports the player directly.
    UFUNCTION(BlueprintCallable, Category="Coastal|Safety")
    bool RequestDefeatReturn();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Safety") bool bEnableFallBoundary = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Safety") float FallBoundaryZ = -1500.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Safety") bool bFadeCamera = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Safety") FString LastDetail;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Safety") FCoastalReturnNotice OnReturnNotice;
    bool SettingsValid() const;
    bool BelowBoundary(const FVector& Position) const;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY() TObjectPtr<UCoastalSaveCoordinator> Saves;
    UPROPERTY() TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY() TObjectPtr<ACharacter> Player;
    UPROPERTY() TObjectPtr<APlayerController> Controller;
    UPROPERTY() TObjectPtr<UEnhancedInputComponent> ReturnInput;
    UPROPERTY() TObjectPtr<UCoastalSwimmingComponent> Swimming;
    TWeakObjectPtr<APlayerCameraManager> FadeManager;
    TArray<TWeakObjectPtr<ACoastalSafetyVolume>> Volumes;
    coastal::ReturnFlow Flow;
    coastal::CheckpointVisit Visit;
    FTransform Fallback, Primary;
    uint64 Epoch = 0;
    bool bInitialized = false, bStopped = false, bControlsOwned = false, bBlockerOwned = false;
    bool bFadeOwned = false, bUsedFallback = false, bFailurePublished = false;
    bool bBoundFallBoundary = true, bBoundFadeCamera = true;
    float BoundFallBoundaryZ = -1500.0f;
    bool InspectVolumes(ACoastalSafetyVolume*& Hazard, ACoastalSafetyVolume*& Checkpoint) const;
    void BeginReturn(const FString& Reason);
    void AdvanceReturn(float DeltaTime);
    bool PlacePlayer();
    void FailReturn(const FString& Detail);
    bool InstallReturnInput();
    void RemoveReturnInput();
    void ReleaseControls();
    void ClearFade();
    void Publish(ECoastalReturnNotice Result, const FString& Detail);
};
