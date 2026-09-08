#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalFoundationTypes.h"
#include "FirstSignalComponent.generated.h"

class UCoastalInventoryAdapter;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFirstSignalChanged, FFirstSignalSnapshot, Snapshot);

// Attach one instance to a persistent mission-director actor in the small opening map.
// This owns objective facts only, not items, UI, interactions, or disk saving.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UFirstSignalComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFirstSignalComponent();
    UPROPERTY(BlueprintAssignable, Category="Coastal|FirstSignal")
    FFirstSignalChanged OnStateChanged;

    UFUNCTION(BlueprintCallable, Category="Coastal|FirstSignal")
    void MarkCabinVisited();
    UFUNCTION(BlueprintCallable, Category="Coastal|FirstSignal")
    void InspectRadio();
    UFUNCTION(BlueprintCallable, Category="Coastal|FirstSignal")
    void DiscoverDock();
    UFUNCTION(BlueprintCallable, Category="Coastal|FirstSignal")
    void ReadMaintenanceNote();

    // The interaction bridge must validate range, line of sight, and UI state first.
    UFUNCTION(BlueprintCallable, Category="Coastal|FirstSignal")
    ECoastalInventoryCommit RequestRadioRepair(UCoastalInventoryAdapter* Inventory);
    UFUNCTION(BlueprintCallable, Category="Coastal|FirstSignal")
    ECoastalListenResult FinishRadioTransmission();

    UFUNCTION(BlueprintCallable, Category="Coastal|FirstSignal")
    ECoastalAvailability CheckRepairAvailability(UCoastalInventoryAdapter* Inventory) const;
    UFUNCTION(BlueprintCallable, Category="Coastal|FirstSignal")
    EFirstSignalPhase GetObjective(UCoastalInventoryAdapter* Inventory) const;
    UFUNCTION(BlueprintPure, Category="Coastal|FirstSignal")
    TArray<FCoastalItemRequirement> GetRepairRequirements() const;
    UFUNCTION(BlueprintPure, Category="Coastal|FirstSignal")
    FName GetRepairTransactionId() const;

    UFUNCTION(BlueprintPure, Category="Coastal|FirstSignal")
    FFirstSignalSnapshot ExportSnapshot() const;
    // Only the save coordinator should restore: inventory/world restoration is external.
    UFUNCTION(BlueprintCallable, Category="Coastal|FirstSignal")
    bool RestoreSnapshot(const FFirstSignalSnapshot& SavedSnapshot);
    UFUNCTION(BlueprintCallable, Category="Coastal|FirstSignal")
    void ResetForNewGame();

private:
    UPROPERTY(VisibleAnywhere, Category="Coastal|FirstSignal")
    FFirstSignalSnapshot State;
    bool bRepairInProgress = false;
    void NotifyChanged();
};
