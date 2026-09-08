#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalSwimmingComponent.generated.h"

class ACharacter;
class ACoastalSwimmingZone;
class APlayerController;
class UAnimInstance;
class UAnimSequence;
class UCharacterMovementComponent;
class UCoastalInteractionBridge;
class UCoastalSaveCoordinator;
class USkeletalMesh;
class USkeletalMeshComponent;
namespace coastal { struct SwimmingSample; }

// Optional surface-swimming owner for the fixed standalone character. It adds no save state.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalSwimmingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCoastalSwimmingComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Swimming")
    TSoftObjectPtr<USkeletalMesh> PresentationMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Swimming")
    TSoftObjectPtr<UAnimSequence> IdleAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Swimming")
    TSoftObjectPtr<UAnimSequence> ForwardAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Swimming",
        meta=(ClampMin="100", ClampMax="600"))
    float SwimSpeedCmPerSecond = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Swimming",
        meta=(ClampMin="1.05", ClampMax="4"))
    float SurfaceBuoyancy = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Swimming",
        meta=(ClampMin="1", ClampMax="200"))
    float ForwardAnimationSpeedThreshold = 20.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Swimming")
    FString LastDetail;

    bool SettingsValid() const;

    UFUNCTION(BlueprintCallable, Category="Coastal|Swimming")
    bool InitializeSwimming(UCoastalInteractionBridge* Interaction);

    UFUNCTION(BlueprintCallable, Category="Coastal|Swimming")
    void ReleaseSwimming();

    UFUNCTION(BlueprintPure, Category="Coastal|Swimming")
    bool IsInitialized() const { return bInitialized && !bStopped; }

    UFUNCTION(BlueprintPure, Category="Coastal|Swimming")
    bool IsSurfaceSwimming() const { return bSwimming; }

    UFUNCTION(BlueprintPure, Category="Coastal|Swimming")
    bool IsRecoveryProtected() const;

    UFUNCTION(BlueprintCallable, Category="Coastal|Swimming")
    void CancelSwimmingForRecovery();

    // Returns true while active so the host can consume jump without an artificial water exit.
    bool HandleJumpPressed();
    void NotifyZoneEntered(ACoastalSwimmingZone* Zone);
    void NotifyZoneLeft(ACoastalSwimmingZone* Zone);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY() TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY() TObjectPtr<UCoastalSaveCoordinator> Saves;
    UPROPERTY() TObjectPtr<ACharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Controller;
    UPROPERTY() TObjectPtr<UCharacterMovementComponent> Movement;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> PlayerMesh;
    UPROPERTY() TObjectPtr<USkeletalMesh> LoadedPresentationMesh;
    UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdleAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> LoadedForwardAnimation;
    UPROPERTY() TObjectPtr<USkeletalMesh> OriginalMesh;
    UPROPERTY() TSubclassOf<UAnimInstance> OriginalAnimClass;
    TWeakObjectPtr<ACoastalSwimmingZone> ActiveZone;
    uint64 ActiveEpoch = 0;
    float OriginalMaxSwimSpeed = 0.0f;
    float OriginalBuoyancy = 0.0f;
    bool bInitialized = false;
    bool bStopped = false;
    bool bSwimming = false;
    bool bForwardAnimation = false;

    bool BindingValid() const;
    coastal::SwimmingSample BuildSample(ACoastalSwimmingZone* Zone) const;
    ACoastalSwimmingZone* CurrentZone() const;
    ACoastalSwimmingZone* ResolveCurrentZone();
    bool BeginSwimming(ACoastalSwimmingZone* Zone);
    bool OwnsPresentation() const;
    void UpdateAnimation();
    void FinishSwimming(const FString& Detail);
};
