#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalCampaignTypes.h"
#include "Core/CampaignRules.h"
#include "CoastalSaveCoordinator.generated.h"

class ACharacter;
class UCoastalPlayerRecoveryComponent;
class ACoastalWorldObject;
class UCoastalInventoryAdapter;
class UFirstSignalComponent;
class UCoastalInteractionBridge;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoastalSaveNotice,
    ECoastalSaveResult, Result, FString, Detail);

// Game-thread-only; small M1 saves use synchronous IO on a deferred tick.
// This is not a claim of async performance or crash-safe disk atomicity.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalSaveCoordinator : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalSaveCoordinator();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Save")
    FCoastalSaveNotice OnSaveNotice;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Save")
    FString LastDetail;

    UFUNCTION(BlueprintCallable, Category="Coastal|Save")
    bool Configure(UCoastalInventoryAdapter* Provider, ACharacter* Character, FName LogicalMapId);
    // Use a NEW namespace; existing A/B files are never deleted to start a game.
    UFUNCTION(BlueprintCallable, Category="Coastal|Save")
    ECoastalSaveResult StartNewCampaign(FName SaveSet, FTransform InitialDryCheckpoint);
    UFUNCTION(BlueprintCallable, Category="Coastal|Save")
    ECoastalSaveResult LoadCampaign(FName SaveSet);
    UFUNCTION(BlueprintCallable, Category="Coastal|Save")
    ECoastalSaveResult SaveNow();
    UFUNCTION(BlueprintCallable, Category="Coastal|Save")
    void RequestSave();
    UFUNCTION(BlueprintPure, Category="Coastal|Save")
    bool IsBusy() const { return Gate.Busy(); }
    UFUNCTION(BlueprintPure, Category="Coastal|Save")
    bool HasActiveCampaign() const { return !ActiveSaveSet.IsNone(); }
    UFUNCTION(BlueprintPure, Category="Coastal|Save")
    FGuid GetCampaignId() const { return CampaignId; }
    UFUNCTION(BlueprintPure, Category="Coastal|Save")
    TArray<FName> GetJournal() const { return Journal; }
    UFUNCTION(BlueprintPure, Category="Coastal|Save")
    int64 GetGeneration() const { return Generation; }
    UFUNCTION(BlueprintCallable, Category="Coastal|Save")
    bool UpdateDryCheckpoint(FTransform Transform);
    UFUNCTION(BlueprintPure, Category="Coastal|Save") FTransform GetDryCheckpoint() const { return DryCheckpoint; }
    UFUNCTION(BlueprintPure, Category="Coastal|Save") bool IsPlayerReturnActive() const { return Gate.IsRecovery(); }
    ACharacter* GetPlayerCharacter() const { return Player; }

    // Mutation ownership is native, not a BP Begin/End pair that can leak a lock.
    UFUNCTION(BlueprintPure, Category="Coastal|Save")
    bool IsConfigured() const { return Ready(); }
    UFUNCTION(BlueprintPure, Category="Coastal|Save")
    bool HasConfiguration() const { return bConfigured; }
    UFUNCTION(BlueprintPure, Category="Coastal|Save")
    bool IsRecoveryRequired() const { return Gate.Poisoned(); }
    UFUNCTION(BlueprintPure, Category="Coastal|Save")
    FName GetActiveSaveSet() const { return ActiveSaveSet; }
    bool BeginMutation();
    void EndMutation();
    UCoastalInventoryAdapter* GetInventory() const { return Inventory; }
    UFirstSignalComponent* GetMission() const { return Mission; }
    bool OwnsObject(const ACoastalWorldObject* Object) const;
    uint64 GetSessionEpoch() const { return SessionEpoch; }
private:
    UPROPERTY() TObjectPtr<UCoastalInventoryAdapter> Inventory;
    UPROPERTY() TObjectPtr<UFirstSignalComponent> Mission;
    UPROPERTY() TObjectPtr<ACharacter> Player;
    UPROPERTY() TArray<TObjectPtr<ACoastalWorldObject>> Objects;
    UPROPERTY() TArray<FName> Journal;
    FGuid CampaignId;
    FName MapId, ActiveSaveSet;
    FTransform DryCheckpoint;
    int64 Generation = 0;
    int32 LastVerifiedSlot = -1;
    uint64 SessionEpoch = 0;
    bool bConfigured = false;
    coastal::OperationGate Gate;

    bool Ready() const;
    ECoastalSaveResult Notice(ECoastalSaveResult Result, const FString& Detail);
    bool Capture(FCoastalCampaignSnapshot& Out, FString& Error) const;
    ECoastalProviderResult Validate(const FCoastalCampaignSnapshot& Saved,
        bool bPersisted, FString& Error) const;
    bool SafeDestination(const FTransform& Transform) const;
    bool ApplyNative(const FCoastalCampaignSnapshot& Saved, const FTransform& Destination, bool bIsRollback = false);
    ECoastalSaveResult RestoreCoherently(const FCoastalCampaignSnapshot& Saved);
    static bool ValidSaveSet(FName SaveSet);
    static FString SlotName(FName SaveSet, int32 Index);
    coastal::SlotInfo ReadSlot(FName SaveSet, int32 Index, FCoastalCampaignSnapshot& Out) const;
    bool WriteVerified(FName SaveSet, int32 Index, const FCoastalCampaignSnapshot& Saved);
    void StopForIntegrationFailure(const FString& Detail);
    friend class UCoastalSessionBootstrapComponent;
    friend class UCoastalInteractionBridge;
    friend class UCoastalPlayerRecoveryComponent;
    bool BeginPlayerReturn(ACharacter* Character, uint64 ExpectedEpoch);
    bool RelocatePlayerDuringReturn(ACharacter* Character, uint64 ExpectedEpoch, const FTransform& Destination);
    bool FinishPlayerReturn(ACharacter* Character, uint64 ExpectedEpoch);
    void FailPlayerReturn(const FString& Detail);
};
