#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalFoundationTypes.h"
#include "CoastalCampaignTypes.h"
#include "CoastalInventoryView.h"
#include "CoastalInventoryAdapter.generated.h"

// Subclass this component in Blueprint and wire the actual installed AGIS API.
// The default implementation deliberately returns NotConfigured, never success.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalInventoryAdapter : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalInventoryAdapter();

    // Read-only. Inspect carried inventory only; do not mutate or search remote storage.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Coastal|Inventory")
    ECoastalAvailability CheckRequirements(const TArray<FCoastalItemRequirement>& Requirements) const;
    virtual ECoastalAvailability CheckRequirements_Implementation(
        const TArray<FCoastalItemRequirement>& Requirements) const;

    // Synchronous contract. Atomically consume all or none. Deduplicate TransactionId
    // within the active campaign; repeated committed IDs must return AlreadyCommitted.
    // Persist the transaction ledger with the inventory in the unified save snapshot.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Coastal|Inventory")
    ECoastalInventoryCommit TryCommitRequirements(FName TransactionId,
        const TArray<FCoastalItemRequirement>& Requirements);
    virtual ECoastalInventoryCommit TryCommitRequirements_Implementation(
        FName TransactionId, const TArray<FCoastalItemRequirement>& Requirements);

    // Read-only AGIS projection for the native list UI. Never parse opaque save bytes here.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Coastal|Inventory")
    ECoastalProviderResult ReadContainerView(FName ContainerId, FCoastalContainerView& View) const;
    virtual ECoastalProviderResult ReadContainerView_Implementation(
        FName ContainerId, FCoastalContainerView& View) const;

    // All new methods fail closed until a project-owned subclass connects real AGIS.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Coastal|Inventory")
    ECoastalProviderResult GetProviderStatus() const;
    virtual ECoastalProviderResult GetProviderStatus_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Coastal|Inventory")
    ECoastalInventoryCommit TryCollectWorldItem(FName WorldId, FCoastalItemRequirement Item);
    virtual ECoastalInventoryCommit TryCollectWorldItem_Implementation(
        FName WorldId, FCoastalItemRequirement Item);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Coastal|Inventory")
    ECoastalInventoryCommit TryTransfer(const FCoastalTransferRequest& Request);
    virtual ECoastalInventoryCommit TryTransfer_Implementation(const FCoastalTransferRequest& Request);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Coastal|Inventory")
    ECoastalProviderResult ExportInventory(FCoastalInventorySnapshot& Snapshot) const;
    virtual ECoastalProviderResult ExportInventory_Implementation(FCoastalInventorySnapshot& Snapshot) const;

    // Read-only full structural, definition, capacity, receipt/payload validation.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Coastal|Inventory")
    ECoastalProviderResult ValidateInventory(const FCoastalInventorySnapshot& Snapshot) const;
    virtual ECoastalProviderResult ValidateInventory_Implementation(const FCoastalInventorySnapshot& Snapshot) const;

    // Synchronous atomic replacement of BOTH items/containers and receipt ledger.
    // A failure MUST leave all live inventory unchanged.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Coastal|Inventory")
    ECoastalProviderResult RestoreInventory(const FCoastalInventorySnapshot& Snapshot);
    virtual ECoastalProviderResult RestoreInventory_Implementation(const FCoastalInventorySnapshot& Snapshot);

    // Read-only: construct an initial snapshot, including guaranteed container contents.
    // Does not reset the live inventory until RestoreInventory is called by coordinator.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Coastal|Inventory")
    ECoastalProviderResult BuildNewInventory(FGuid CampaignId, FCoastalInventorySnapshot& Snapshot) const;
    virtual ECoastalProviderResult BuildNewInventory_Implementation(
        FGuid CampaignId, FCoastalInventorySnapshot& Snapshot) const;
};
