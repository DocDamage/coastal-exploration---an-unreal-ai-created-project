#pragma once
#include "CoreMinimal.h"
#include "CoastalFoundationTypes.generated.h"

UENUM(BlueprintType)
enum class ECoastalInventoryCommit : uint8
{
    Committed, AlreadyCommitted, MissingItems, NotConfigured, Failed, NoSpace
};

UENUM(BlueprintType)
enum class ECoastalAvailability : uint8
{
    Available, MissingItems, NotConfigured, Failed
};

UENUM(BlueprintType)
enum class EFirstSignalPhase : uint8
{
    InspectRadio, FindEquipment, ReturnToRadio, ListenToRadio, Complete
};

UENUM(BlueprintType)
enum class ECoastalListenResult : uint8
{
    Applied, AlreadyApplied, NotRepaired
};

USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalItemRequirement
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal")
    FName ItemId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal", meta=(ClampMin="1"))
    int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FFirstSignalSnapshot
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    int32 SchemaVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    bool bCabinVisited = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    bool bRadioInspected = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    bool bDockDiscovered = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    bool bMaintenanceNoteRead = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    bool bRadioRepaired = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category="Coastal")
    bool bMessageHeard = false;
};
