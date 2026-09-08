#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoastalCampaignTypes.h"
#include "CoastalWorldObject.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class USceneComponent;
class UStaticMesh;
class UCoastalSaveCoordinator;
class UCoastalInteractionBridge;

// Native development proxy; subclass with real owned art after the M1 gate.
UCLASS(Blueprintable)
class COASTALFOUNDATION_API ACoastalWorldObject : public AActor
{
    GENERATED_BODY()
public:
    ACoastalWorldObject();
    virtual void OnConstruction(const FTransform& Transform) override;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Coastal|Proxy")
    void RefreshDevelopmentProxy();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal")
    FName WorldId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal")
    ECoastalObjectKind Kind = ECoastalObjectKind::Door;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal")
    FCoastalItemRequirement PickupItem;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal")
    FName JournalEntry;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Proxy")
    FString DisplayLabel = TEXT("DEV PROXY");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Proxy")
    FVector ProxySizeCm = FVector(80, 60, 80);
    // Optional authored presentation; gameplay/save identity stays on this actor.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Art")
    TObjectPtr<UStaticMesh> VisualMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Art")
    FTransform VisualTransform;
    // A guaranteed supply container remains in the world after its contents are taken.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Art")
    bool bPersistentPickupContainer = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Proxy")
    TObjectPtr<UStaticMeshComponent> ProxyMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Proxy")
    TObjectPtr<UTextRenderComponent> Label;
    UFUNCTION(BlueprintPure, Category="Coastal")
    bool IsActive() const { return bActive; }
    UFUNCTION(BlueprintPure, Category="Coastal")
    FVector GetInteractionPoint() const;
private:
    UPROPERTY()
    bool bActive = false;
    void ApplyNativeState(bool bNewActive);
    friend class UCoastalSaveCoordinator;
    friend class UCoastalInteractionBridge;
};
