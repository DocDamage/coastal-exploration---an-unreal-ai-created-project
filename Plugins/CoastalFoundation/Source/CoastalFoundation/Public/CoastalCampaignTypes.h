#pragma once
#include "CoreMinimal.h"
#include "CoastalFoundationTypes.h"
#include "CoastalCampaignTypes.generated.h"

UENUM(BlueprintType)
enum class ECoastalProviderResult : uint8 { Ready, NotConfigured, Incompatible, Failed };
UENUM(BlueprintType)
enum class ECoastalSaveResult : uint8
{
    Saved, Loaded, RecoveredPrevious, Busy, NoSave, NotConfigured, InvalidSnapshot,
    Incompatible, DiskFailure, RestoreFailed, RecoveryRequired, StartedNew
};
UENUM(BlueprintType)
enum class ECoastalObjectKind : uint8 { Door, Pickup, Storage, Radio, Discovery, MaintenanceNote };

USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalReceipt
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FName TransactionId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FString Fingerprint;
};
USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalInventorySnapshot
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    int32 SchemaVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FGuid CampaignId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FName ProviderId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FString ProviderVersion;
    // AGIS items, container grids, instance IDs, definitions. Never shadow item counts.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    TArray<uint8> Payload;
    // Adapter-owned commit ledger, restored atomically with Payload.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    TArray<FCoastalReceipt> Receipts;
};
USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalWorldRecord
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FName WorldId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    ECoastalObjectKind Kind = ECoastalObjectKind::Door;
    // Door=open; Pickup=collected; Discovery/Note=read. Radio owned by mission.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    bool bActive = false;
};
USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalCampaignSnapshot
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    int32 SchemaVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FGuid CampaignId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    int64 Generation = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FName MapId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FTransform PlayerTransform;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FTransform DryCheckpoint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FCoastalInventorySnapshot Inventory;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    TArray<FCoastalWorldRecord> World;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    TArray<FName> Journal;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    FFirstSignalSnapshot FirstSignal;
};
USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalTransferRequest
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal")
    FGuid OperationId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal")
    FGuid ItemInstanceId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal")
    FName SourceContainer;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal")
    FName DestinationContainer;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal")
    int32 Quantity = 1;
};
