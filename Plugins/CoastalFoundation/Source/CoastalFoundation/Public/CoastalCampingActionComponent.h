#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalCampingActionComponent.generated.h"

class AActor;
class ACharacter;
class APlayerController;
class UAnimInstance;
class UAnimSequence;
class UCharacterMovementComponent;
class UCoastalInteractionBridge;
class USkeletalMesh;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class ECoastalCampingAction : uint8
{
    WarmHands,
    RestByFire,
    RestInShelter
};

UENUM(BlueprintType)
enum class ECoastalCampingStartResult : uint8
{
    Started,
    AlreadyActive,
    Unavailable,
    InputBlocked,
    TooFar,
    UnsafeDestination,
    PathBlocked,
    ForeignAnimation,
    Failed
};

// Optional cosmetic action owner for the fixed local character. It never mutates campaign state.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalCampingActionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCoastalCampingActionComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Camping")
    FName CampsiteActorTag = TEXT("Coastal.Campsite");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Camping", meta=(ClampMin="50", ClampMax="500"))
    float CampsiteRadiusCm = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Camping")
    TSoftObjectPtr<USkeletalMesh> PresentationMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Camping")
    TSoftObjectPtr<UAnimSequence> WarmHandsAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Camping")
    TSoftObjectPtr<UAnimSequence> RestByFireAnimation;

    // Optional shelter action; editor bindings may use a different supplied skeleton.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Shelter")
    FName ShelterActorTag = TEXT("Coastal.ShelterRest");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Shelter")
    TSoftObjectPtr<USkeletalMesh> ShelterPresentationMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Shelter")
    TSoftObjectPtr<UAnimSequence> ShelterRestAnimation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Camping")
    FString LastDetail;

    // Called once by the existing local UI owner after the interaction bridge is configured.
    bool InitializeCamping(UCoastalInteractionBridge* Interaction);
    void ReleaseCamping();

    UFUNCTION(BlueprintPure, Category="Coastal|Camping")
    bool IsNearCampsite() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Shelter")
    bool IsNearShelter() const;

    // Menu-offer check: it intentionally ignores the pause menu's own blocker.
    UFUNCTION(BlueprintPure, Category="Coastal|Camping")
    bool CanOfferAction(ECoastalCampingAction Action) const;

    // Call only after the pause menu has released input and resumed the world.
    UFUNCTION(BlueprintCallable, Category="Coastal|Camping")
    ECoastalCampingStartResult StartAction(ECoastalCampingAction Action);

    UFUNCTION(BlueprintCallable, Category="Coastal|Camping")
    void CancelAction();

    UFUNCTION(BlueprintPure, Category="Coastal|Camping")
    bool IsActionActive() const { return bActionActive; }

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY() TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY() TObjectPtr<ACharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Controller;
    UPROPERTY() TObjectPtr<UCharacterMovementComponent> Movement;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> PlayerMesh;
    UPROPERTY() TObjectPtr<USkeletalMesh> LoadedPresentationMesh;
    UPROPERTY() TObjectPtr<UAnimSequence> LoadedWarmHands;
    UPROPERTY() TObjectPtr<UAnimSequence> LoadedRestByFire;
    UPROPERTY() TObjectPtr<USkeletalMesh> LoadedShelterMesh;
    UPROPERTY() TObjectPtr<UAnimSequence> LoadedShelterRest;
    UPROPERTY() TObjectPtr<USkeletalMesh> OriginalMesh;
    UPROPERTY() TSubclassOf<UAnimInstance> OriginalAnimClass;
    TWeakObjectPtr<AActor> ActiveMarker;
    FVector ActionStartLocation = FVector::ZeroVector;
    ECoastalCampingAction ActiveAction = ECoastalCampingAction::WarmHands;
    bool bInitialized = false;
    bool bStopped = false;
    bool bActionActive = false;

    bool BindingValid() const;
    AActor* FindNearbyCampsite(ECoastalCampingAction Action = ECoastalCampingAction::WarmHands) const;
    UAnimSequence* AnimationFor(ECoastalCampingAction Action) const;
    USkeletalMesh* MeshFor(ECoastalCampingAction Action) const;
    void LoadShelterAssets();
    bool HasForeignMontage() const;
    bool MarkerDestination(AActor* Marker, FTransform& OutTransform) const;
    bool SweepToMarker(const FTransform& Destination, FHitResult& OutHit) const;
    bool OwnsPresentation() const;
    void FinishAction(const FString& Detail, bool bCompleted);
};
