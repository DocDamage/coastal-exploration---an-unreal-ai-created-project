#pragma once
#include "CoreMinimal.h"
#include "CoastalInventoryView.generated.h"

USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalInventoryViewItem
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") FGuid InstanceId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") FName ItemId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") int32 Quantity = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") FIntPoint Position = FIntPoint::ZeroValue;
    // Effective occupied size AFTER rotation, in AGIS grid cells.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") FIntPoint Size = FIntPoint(1, 1);
};
USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalContainerView
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") FName ContainerId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") FIntPoint Grid = FIntPoint::ZeroValue;
    // Nonnegative, changes on ANY inventory mutation/restore; not a save generation.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") int64 Revision = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Coastal|Inventory") TArray<FCoastalInventoryViewItem> Items;
    bool IsValidFor(FName Expected) const;
};
