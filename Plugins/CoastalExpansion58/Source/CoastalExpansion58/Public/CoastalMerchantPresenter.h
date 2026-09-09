#pragma once

#include "CoreMinimal.h"
#include "Core/MerchantPresentationRules.h"
#include "GameFramework/Actor.h"
#include "CoastalMerchantPresenter.generated.h"

class UAnimSequence;
class ACharacter;
class UCoastalInteractionBridge;
class UCoastalSaveCoordinator;
class UCoastalUISessionComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class ECoastalMerchantPresentation : uint8 { Dormant, Idle, AwaitingJournalClose, Pitching };

// A map-owned, presentation-only merchant. The village register remains the authority.
UCLASS(Blueprintable)
class COASTALEXPANSION58_API ACoastalMerchantPresenter : public AActor
{
    GENERATED_BODY()
public:
    ACoastalMerchantPresenter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Merchant")
    TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Merchant")
    TObjectPtr<USkeletalMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Merchant|Presentation")
    TSoftObjectPtr<USkeletalMesh> MerchantMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Merchant|Presentation")
    TSoftObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Merchant|Presentation")
    TSoftObjectPtr<UAnimSequence> PitchAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Merchant|Presentation")
    FTransform MeshRelativeTransform;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Merchant", meta=(ClampMin="5", ClampMax="120"))
    float JournalCloseTimeoutSeconds = 45.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Merchant")
    ECoastalMerchantPresentation Presentation = ECoastalMerchantPresentation::Dormant;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Merchant")
    FString LastDetail;

    UFUNCTION(BlueprintPure, Category="Coastal|Merchant") bool IsInitialized() const;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() TObjectPtr<UAnimSequence> LoadedIdle;
    UPROPERTY() TObjectPtr<UAnimSequence> LoadedPitch;
    UPROPERTY() TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY() TObjectPtr<UCoastalSaveCoordinator> Saves;
    UPROPERTY() TObjectPtr<UCoastalUISessionComponent> UI;
    TWeakObjectPtr<ACharacter> Player;
    coastal::MerchantPhase Phase = coastal::MerchantPhase::Dormant;
    uint64 Epoch = 0;
    double QueueDeadline = 0;
    double NextInitializeTime = 0;
    bool bInitialized = false;
    bool bSawJournalModal = false;
    bool bPresentationLoadFailed = false;

    bool TryInitialize();
    bool BindingsValid() const;
    void SetPhase(coastal::MerchantPhase NewPhase, const FString& Detail);
    void PlayIdle();
    void ResetBindings();
    UFUNCTION() void HandleJournalRequested(FName JournalEntryId);
};
