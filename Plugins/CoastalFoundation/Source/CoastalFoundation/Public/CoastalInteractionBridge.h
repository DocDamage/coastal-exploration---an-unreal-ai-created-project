#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalCampaignTypes.h"
#include "CoastalInteractionOffer.h"
#include "Core/InteractionRules.h"
#include "CoastalInteractionBridge.generated.h"
class UCoastalSaveCoordinator;
class ACoastalWorldObject;
UENUM(BlueprintType)
enum class ECoastalActionResult : uint8
{
    Applied, AlreadyApplied, Busy, BlockedByUI, TooFar, Occluded, InvalidTarget,
    NotConfigured, MissingItems, NoSpace, Failed, SuppressedInput, StaleFocus
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoastalStorageRequested, FName, StorageWorldId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoastalTranscriptRequested, FText, Transcript, FGuid, AcknowledgementToken);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoastalJournalRequested, FName, JournalEntryId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoastalActionNotice, ECoastalActionResult, Result);
// Presentation only, emitted after the authoritative operation finishes.
DECLARE_MULTICAST_DELEGATE_ThreeParams(FCoastalActionFeedback, ECoastalActionResult, FName, FVector);

// Hyper supplies focus/prompt. This bridge owns permission checks and game actions.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalInteractionBridge : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalInteractionBridge();
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction")
    bool Configure(UCoastalSaveCoordinator* Coordinator);
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction")
    ECoastalActionResult TryInteract(ACoastalWorldObject* Target);
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction")
    ECoastalActionResult TransferWithOpenStorage(const FCoastalTransferRequest& Request);
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction")
    ECoastalActionResult AcknowledgeTranscript(FGuid Token);
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction")
    void CancelTranscript();
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction")
    void CloseStorage();
    // Unique widget tokens prevent one closed menu from unblocking another menu.
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction")
    bool AcquireUIBlocker(FName Token);
    UFUNCTION(BlueprintCallable, Category="Coastal|Interaction")
    void ReleaseUIBlocker(FName Token);
    UCoastalSaveCoordinator* GetCoordinator() const { return Saves; }
#if WITH_DEV_AUTOMATION_TESTS
    // Read-only observation of the real issued token for cross-load acceptance.
    FGuid GetTranscriptTokenForAcceptance() const { return TranscriptToken; }
#endif
    ECoastalActionResult ValidateOpenStorage() const;
    // Input permission changes invalidate vendor focus, even when a menu opens/closes between ticks.
    uint64 GetInputRevision() const { return InputRevision; }
    UFUNCTION(BlueprintPure, Category="Coastal|Interaction")
    bool AllowsWorldInput() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Interaction")
    FCoastalInteractionOffer PreviewInteraction(ACoastalWorldObject* Target) const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Interaction", meta=(ClampMin="10", ClampMax="500"))
    float ReachCm = 180.0f;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Interaction")
    FCoastalStorageRequested OnStorageRequested;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Interaction")
    FCoastalTranscriptRequested OnTranscriptRequested;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Interaction")
    FCoastalJournalRequested OnJournalRequested;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Interaction")
    FCoastalActionNotice OnActionNotice;
    FCoastalActionFeedback OnActionFeedback;
private:
    UPROPERTY() TObjectPtr<UCoastalSaveCoordinator> Saves;
    UPROPERTY() TSet<FName> UIBlockers;
    TWeakObjectPtr<ACoastalWorldObject> OpenStorage;
    uint64 StorageEpoch = 0, TranscriptEpoch = 0;
    FGuid TranscriptToken;
    uint64 InputRevision = 0;
    coastal::InteractionDispatchGate DispatchGate;
    ECoastalActionResult CheckTarget(ACoastalWorldObject* Target, bool bCheckUI) const;
    ECoastalActionResult ApplyAction(ACoastalWorldObject* Target);
    ECoastalActionResult Emit(ECoastalActionResult Result, FName Cue = TEXT("confirm"), FVector Location = FVector::ZeroVector);
    static ECoastalActionResult FromInventory(ECoastalInventoryCommit Result);
};
