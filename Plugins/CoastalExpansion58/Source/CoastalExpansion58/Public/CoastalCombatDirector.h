#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoastalCombatDirector.generated.h"

class AWeaponBase;
class UCoastalCombatComponent;
class UCoastalCombatAudioComponent;
class UCoastalCombatOverlay;
class UMaterialInterface;
class UStaticMesh;

// Map-owned bootstrap for the optional UE5.8 combat layer; no 5.7 host hook is required.
UCLASS(Blueprintable)
class COASTALEXPANSION58_API ACoastalCombatDirector : public AActor
{
    GENERATED_BODY()
public:
    ACoastalCombatDirector();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") TSubclassOf<AWeaponBase> WeaponClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FName WeaponAttachSocket = TEXT("hand_r");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") TSoftObjectPtr<UStaticMesh> WeaponVisualAsset;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FTransform WeaponVisualTransform;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FVector MuzzleOffset = FVector(25, 0, 0);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") TSoftObjectPtr<UStaticMesh> TracerAsset;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") TSoftObjectPtr<UMaterialInterface> TracerMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat", meta=(ClampMin="1")) float MaxHealth = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat", meta=(ClampMin="0")) float MaxShield = 50.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") bool bShowCombatHUD = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FString LastDetail;
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") bool IsInitialized() const;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY() TObjectPtr<UCoastalCombatComponent> Combat;
    UPROPERTY() TObjectPtr<UCoastalCombatAudioComponent> CombatAudio;
    UPROPERTY() TObjectPtr<UCoastalCombatOverlay> Overlay;
    bool bCreatedCombat = false;
    bool bCreatedCombatAudio = false;
    bool bCreatedShooter = false;
    bool TryInitialize();
};
