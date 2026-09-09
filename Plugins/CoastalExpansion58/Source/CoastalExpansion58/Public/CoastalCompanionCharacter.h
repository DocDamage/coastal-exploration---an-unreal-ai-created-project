#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "Core/CompanionRules.h"
#include "CoastalCompanionCharacter.generated.h"

class UAnimSequence;
class UCoastalSaveCoordinator;
class USkeletalMesh;

UENUM(BlueprintType)
enum class ECoastalCompanionMode : uint8
{
    Dormant,
    Following,
    Waiting,
    Regrouping
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoastalCompanionModeChanged,
    ECoastalCompanionMode, Mode, FString, Detail);

// Transient standalone companion owned by the already-configured local player.
// Uses CharacterMovement plus swept local steering; it owns no input, inventory, or save data.
UCLASS(Blueprintable)
class COASTALEXPANSION58_API ACoastalCompanionCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ACoastalCompanionCharacter();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Presentation")
    TSoftObjectPtr<USkeletalMesh> CompanionMeshAsset;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Presentation")
    TSoftObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Presentation")
    TSoftObjectPtr<UAnimSequence> WalkAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Presentation")
    TSoftObjectPtr<UAnimSequence> RunAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Presentation")
    FTransform MeshRelativeTransform = FTransform(FRotator(0, -90, 0), FVector(0, 0, -48), FVector::OneVector);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Follow", meta=(ClampMin="75", ClampMax="600"))
    float StopDistanceCm = 155.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Follow", meta=(ClampMin="100", ClampMax="1200"))
    float RunDistanceCm = 650.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Follow", meta=(ClampMin="500", ClampMax="6000"))
    float CatchupDistanceCm = 2400.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Follow", meta=(ClampMin="75", ClampMax="600"))
    float FollowOffsetCm = 210.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Follow", meta=(ClampMin="-300", ClampMax="300"))
    float SideOffsetCm = 90.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Follow", meta=(ClampMin="50", ClampMax="500"))
    float WalkSpeedCmPerSecond = 220.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Follow", meta=(ClampMin="100", ClampMax="900"))
    float RunSpeedCmPerSecond = 440.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Companion")
    ECoastalCompanionMode Mode = ECoastalCompanionMode::Dormant;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Companion")
    FString LastDetail;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Companion")
    FCoastalCompanionModeChanged OnCompanionModeChanged;

    bool InitializeCompanion(ACharacter* ExistingPlayer, UCoastalSaveCoordinator* Coordinator,
        UCoastalPlayerRecoveryComponent* RecoveryOwner);
    UFUNCTION(BlueprintCallable, Category="Coastal|Companion") bool SetFollowing(bool bShouldFollow);
    UFUNCTION(BlueprintCallable, Category="Coastal|Companion") bool ToggleFollowing();
    UFUNCTION(BlueprintPure, Category="Coastal|Companion") bool IsFollowingRequested() const { return bFollowRequested; }
    UFUNCTION(BlueprintPure, Category="Coastal|Companion") bool IsInitialized() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Companion") bool CanAcceptCommand() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Companion") ACharacter* GetPlayerOwner() const { return Player.Get(); }

    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    UPROPERTY() TObjectPtr<UCoastalSaveCoordinator> Saves;
    UPROPERTY() TObjectPtr<UCoastalPlayerRecoveryComponent> Recovery;
    TWeakObjectPtr<ACharacter> Player;
    UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdle;
    UPROPERTY() TObjectPtr<UAnimSequence> LoadedWalk;
    UPROPERTY() TObjectPtr<UAnimSequence> LoadedRun;
    uint64 Epoch = 0;
    double NextRegroupTime = 0.0;
    FVector LastProgressLocation = FVector::ZeroVector;
    float StuckSeconds = 0.0f;
    bool bInitialized = false;
    bool bStopped = false;
    bool bFollowRequested = true;
    bool bNeedsRegroup = true;
    float AvoidanceSign = 1.0f;
    coastal::CompanionPace PresentedPace = coastal::CompanionPace::Idle;

    bool BindingValid() const;
    bool SettingsValid() const;
    void SetMode(ECoastalCompanionMode NewMode, const FString& Detail);
    void SetActive(bool bActive);
    bool LoadPresentation();
    bool FindDryLocationNearPlayer(FTransform& OutTransform) const;
    bool TryRegroup();
    bool ChooseSteeringDirection(const FVector& Target, FVector& OutDirection);
    bool CanAdvance(const FVector& Direction, float StepCm) const;
    FVector FollowTarget() const;
    void StopLocomotion();
    void PresentPace(coastal::CompanionPace Pace);
    UFUNCTION() void HandlePlayerReturn(ECoastalReturnNotice Result, FString Detail);
};
