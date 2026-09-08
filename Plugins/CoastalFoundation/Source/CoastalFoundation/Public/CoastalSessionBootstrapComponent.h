#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalIntegrationTypes.h"
#include "Core/BootstrapRules.h"
#include "CoastalSessionBootstrapComponent.generated.h"
class ACoastalMissionDirector;
class UCoastalInventoryAdapter;
class UCoastalInteractionBridge;
class UCoastalUISessionComponent;
class UCoastalInteractionRelayComponent;
class UCoastalPlayerRecoveryComponent;
class UCoastalSwimmingComponent;
class ACharacter;
class APlayerController;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoastalStartupNotice, ECoastalStartupResult, Result);
// Optional preferred replacement for the three manual Configure/Initialize calls.
// Add once to the local PlayerController; invoke explicitly after REAL vendor readiness.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalSessionBootstrapComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalSessionBootstrapComponent();
    // Explicit strict 79-object acceptance-room profile; never relaxes the base manifest.
    UPROPERTY(EditAnywhere, Category="Coastal|Acceptance") bool bInventoryCapacityFixture = false;
    UFUNCTION(BlueprintCallable, Category="Coastal|Startup")
    bool RunPreflight(ACoastalMissionDirector* Director, UCoastalInventoryAdapter* Provider,
        FTransform InitialDryCheckpoint, FCoastalIntegrationReport& Report) const;
    UFUNCTION(BlueprintCallable, Category="Coastal|Startup")
    ECoastalStartupResult StartTestRoom(ACoastalMissionDirector* Director, UCoastalInventoryAdapter* Provider,
        FTransform InitialDryCheckpoint);
    UFUNCTION(BlueprintPure, Category="Coastal|Startup") ECoastalStartupPhase GetPhase() const;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Startup") FCoastalIntegrationReport LastReport;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Startup") FCoastalStartupNotice OnStartupNotice;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    coastal::StartupGate Gate;
    UPROPERTY() TObjectPtr<ACoastalMissionDirector> BoundDirector;
    UPROPERTY() TObjectPtr<UCoastalInventoryAdapter> BoundProvider;
    UPROPERTY() TObjectPtr<ACharacter> BoundCharacter;
    UPROPERTY() TObjectPtr<UCoastalInteractionBridge> BoundBridge;
    UPROPERTY() TObjectPtr<UCoastalUISessionComponent> BoundUI;
    UPROPERTY() TObjectPtr<UCoastalInteractionRelayComponent> BoundRelay;
    bool bHadRelay = false, bHadRecovery = false, bHadSwimming = false;
    UPROPERTY() TObjectPtr<UCoastalPlayerRecoveryComponent> BoundRecovery;
    UPROPERTY() TObjectPtr<UCoastalSwimmingComponent> BoundSwimming;
    UPROPERTY() TObjectPtr<APlayerController> LockedController;
    FTransform BoundCheckpoint;
    bool bPublishing = false;
    ECoastalStartupResult Publish(ECoastalStartupResult Result);
    ECoastalStartupResult BindingFailed(const TCHAR* Step, const FString& Detail);
};
