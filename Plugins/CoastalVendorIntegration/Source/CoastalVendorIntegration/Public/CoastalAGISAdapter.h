#pragma once
#include "CoastalInventoryAdapter.h"
#include "CoastalAGISAdapter.generated.h"

class UCompositeDataTable;
class UDataTable;
struct FCoastalAGISItem
{
    int32 Uid = 0;
    int32 Container = 0;
    int32 Slot = 0;
    bool Rotated = false;
    bool operator==(const FCoastalAGISItem&) const = default;
};

// Bounded M1 codec: two radio parts and up to 72 non-stackable postcard instances.
// UID identifies the instance; all postcards share one catalogue definition. The runtime
// quantities, positions and occupied cells are held only by real AGIS components.
UCLASS(ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALVENDORINTEGRATION_API UCoastalAGISAdapter : public UCoastalInventoryAdapter
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Coastal|Inventory") bool InitializeRealProvider();
    virtual ECoastalProviderResult GetProviderStatus_Implementation() const override;
    virtual ECoastalAvailability CheckRequirements_Implementation(const TArray<FCoastalItemRequirement>& Requirements) const override;
    virtual ECoastalInventoryCommit TryCollectWorldItem_Implementation(FName WorldId, FCoastalItemRequirement Item) override;
    virtual ECoastalInventoryCommit TryCommitRequirements_Implementation(FName Id, const TArray<FCoastalItemRequirement>& Requirements) override;
    virtual ECoastalInventoryCommit TryTransfer_Implementation(const FCoastalTransferRequest& Request) override;
    virtual ECoastalProviderResult ReadContainerView_Implementation(FName Id, FCoastalContainerView& View) const override;
    virtual ECoastalProviderResult ExportInventory_Implementation(FCoastalInventorySnapshot& Snapshot) const override;
    virtual ECoastalProviderResult ValidateInventory_Implementation(const FCoastalInventorySnapshot& Snapshot) const override;
    virtual ECoastalProviderResult RestoreInventory_Implementation(const FCoastalInventorySnapshot& Snapshot) override;
    virtual ECoastalProviderResult BuildNewInventory_Implementation(FGuid Id, FCoastalInventorySnapshot& Snapshot) const override;
    static FName ItemId(int32 Uid);
    static constexpr int32 LastUid=74;
    static FName WorldIdFor(int32 Uid);
    static int32 UidForWorld(FName World);
    static FName ContainerId(int32 Index);
    static FIntPoint Grid(int32 Index);
    static FIntPoint ItemSize(int32 Uid, bool Rotated);
    FGuid InstanceId(int32 Uid) const;
    static FGuid InstanceFor(FGuid CampaignId,int32 Uid);
    bool DefinitionsValid() const;
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TObjectPtr<UClass> InventoryClass;
    UPROPERTY(Transient) TObjectPtr<UObject> Library;
    UPROPERTY(Transient) TObjectPtr<UCompositeDataTable> VendorDefinitions;
    UPROPERTY(Transient) TObjectPtr<UDataTable> CoastalDefinitions;
    UPROPERTY(Transient) TObjectPtr<UActorComponent> Backpack;
    UPROPERTY(Transient) TObjectPtr<UActorComponent> Storage;
    UPROPERTY(Transient) TObjectPtr<UScriptStruct> ContainerType;
    UPROPERTY(Transient) TObjectPtr<UScriptStruct> ItemType;
    UPROPERTY(Transient) TArray<FCoastalReceipt> Receipts;
    FGuid Campaign;
    int64 Revision = 0;
    bool Ready = false;
    bool Busy = false;
    bool RegisterDefinitions();
    UActorComponent* MakeInventory(int32 Index) const;
    bool ReadItems(UActorComponent* Inventory, int32 Index, TArray<FCoastalAGISItem>& Items) const;
    bool ReadLive(TArray<FCoastalAGISItem>& Items) const;
    bool Insert(UActorComponent* Inventory, const FCoastalAGISItem& Item) const;
    ECoastalInventoryCommit FindSpace(UActorComponent* Inventory, int32 Uid, int32& Slot, bool& Rotated) const;
    bool Replace(const TArray<FCoastalAGISItem>& Items, const TArray<FCoastalReceipt>& Ledger, FGuid Id);
    bool Encode(FGuid Id, const TArray<FCoastalAGISItem>& Items, const TArray<FCoastalReceipt>& Ledger, FCoastalInventorySnapshot& Snapshot) const;
    bool Decode(const FCoastalInventorySnapshot& Snapshot, TArray<FCoastalAGISItem>& Items) const;
    ECoastalInventoryCommit ReceiptResult(FName Id, const FString& Fingerprint) const;
};
