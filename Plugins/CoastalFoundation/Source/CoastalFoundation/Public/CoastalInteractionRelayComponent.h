#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalInteractionBridge.h"
#include "Core/InteractionRules.h"
#include "CoastalInteractionRelayComponent.generated.h"
class APlayerController;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;

UENUM(BlueprintType)
enum class ECoastalInteractInputOwner : uint8 { VendorEvents, NativeEnhancedInput };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoastalOfferChanged, FCoastalInteractionOffer, Offer);

// One relay on the local controller. Hyper supplies focus and owns prompt rendering.
// No traces for discovery, no guessed vendor API and no second inventory.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalInteractionRelayComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalInteractionRelayComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Interaction")
    ECoastalInteractInputOwner InputOwner = ECoastalInteractInputOwner::VendorEvents;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Interaction") FCoastalOfferChanged OnOfferChanged;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Interaction") FString LastDetail;
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction")
    bool InitializeRelay(UCoastalInteractionBridge* ActualBridge);
    UFUNCTION(BlueprintPure, Category="Coastal|Interaction") bool IsInitialized() const { return bInitialized; }
    // Forward the actual selected world object every local focus evaluation/frame. Null clears.
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction") bool UpdateFocusedTarget(ACoastalWorldObject* Target);
    // A delayed loss callback for A must not clear newer focus B.
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction") void ClearFocusedTarget(ACoastalWorldObject* ExpectedTarget);
    // VendorEvents mode: submit actual aggregate action-down state every input evaluation,
    // including false after release. A held true does not repeat or queue an interaction.
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction") ECoastalActionResult SubmitExternalInput(bool bIsDown);
    UFUNCTION(BlueprintPure, Category="Coastal|Interaction") FCoastalInteractionOffer GetCurrentOffer() const;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY() TObjectPtr<APlayerController> Controller;
    UPROPERTY() TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY() TObjectPtr<UInputAction> InteractAction;
    UPROPERTY() TObjectPtr<UInputMappingContext> InteractContext;
    UPROPERTY() TObjectPtr<UEnhancedInputComponent> InteractInput;
    TWeakObjectPtr<ACoastalWorldObject> FocusedTarget;
    coastal::InteractionIntentGate Intent;
    coastal::InteractionFocusLease Lease;
    FCoastalInteractionOffer PublishedOffer;
    ECoastalInteractInputOwner BoundInputOwner = ECoastalInteractInputOwner::VendorEvents;
    uint64 Epoch = 0, Revision = 0;
    bool bInitialized = false, bStopped = false, bPublishing = false, bDispatching = false;
    bool Synchronize();
    bool LiveBinding() const;
    ECoastalActionResult Press();
    bool InstallNativeInput();
    void RemoveNativeInput();
    void NativePressed();
    void NativeReleased();
    void NativeCanceled();
    bool NativeKeysDown() const;
    void PublishOffer();
};
