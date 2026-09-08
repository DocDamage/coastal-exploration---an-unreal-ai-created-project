#pragma once
#include "Components/ActorComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoastalInteractionOffer.h"
#include "CoastalHostSession.generated.h"
class UCoastalAGISAdapter;
class UCoastalInteractionBridge;
class UCoastalInteractionRelayComponent;
class UCoastalUISessionComponent;
class ACoastalWorldObject;
class AController;
class APlayerController;
class APawn;
class UCameraComponent;
class USpringArmComponent;
struct FCoastalCampaignProbe;

UCLASS(ClassGroup=(Coastal),meta=(BlueprintSpawnableComponent))
class COASTALVENDORINTEGRATION_API UCoastalHostSession : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalHostSession();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function) override;
    UFUNCTION(BlueprintCallable,Category="Coastal|Host") void InitializeHost();
    UFUNCTION() void PawnChanged(APawn* OldPawn,APawn* NewPawn);
    UFUNCTION() void OfferChanged(FCoastalInteractionOffer Offer);
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Coastal|Host") FString LastStatus;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Coastal|Host") bool Started=false;
private:
    bool Attempted=false;
    void FocusedProxyLabel(ACoastalWorldObject* Target);
    TWeakObjectPtr<ACoastalWorldObject> HiddenProxyLabel;
    bool ProxyLabelWasVisible=false;
    double HyperFrontOffset=0;
    UPROPERTY(Transient) TObjectPtr<UCameraComponent> HostCamera;
    UPROPERTY(Transient) TObjectPtr<USpringArmComponent> HostCameraBoom;
    TSharedPtr<FCoastalCampaignProbe> Probe;
    UPROPERTY(Transient) TObjectPtr<APlayerController> Controller;
    UPROPERTY(Transient) TObjectPtr<UCoastalAGISAdapter> Provider;
    UPROPERTY(Transient) TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY(Transient) TObjectPtr<UCoastalInteractionRelayComponent> Relay;
    UPROPERTY(Transient) TObjectPtr<UCoastalUISessionComponent> UI;
    UPROPERTY(Transient) TObjectPtr<UActorComponent> Hyper;
};

UCLASS()
class COASTALVENDORINTEGRATION_API UCoastalHyperFunctions : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure,Category="Coastal|Hyper")
    static bool CanPresent(ACoastalWorldObject* Target,AController* Controller);
    UFUNCTION(BlueprintPure,Category="Coastal|Hyper")
    static void PromptText(ACoastalWorldObject* Target,AController* Controller,FText& Primary,FText& Secondary);
    UFUNCTION(BlueprintPure,Category="Coastal|Hyper")
    static void PromptColor(ACoastalWorldObject* Target,AController* Controller,bool& Custom,FLinearColor& Color);
};
