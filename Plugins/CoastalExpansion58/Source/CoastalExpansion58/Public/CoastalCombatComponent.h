#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "Core/CombatRules.h"
#include "CoastalCombatComponent.generated.h"

class ACharacter;
class ACoastalCombatDirector;
class ACoastalSidearmWeapon;
class APlayerController;
class AWeaponBase;
class UAnimMontage;
class UCoastalCampingActionComponent;
class UCoastalInteractionBridge;
class UCoastalSaveCoordinator;
class UCoastalSwimmingComponent;
class UDamageType;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInterface;
class UStaticMesh;
class UAdvancedShooterComponent;

UENUM(BlueprintType)
enum class ECoastalCombatResult : uint8
{
    Applied,
    NotConfigured,
    NoCampaign,
    OutsideEncounter,
    InputBlocked,
    Busy,
    Recovering,
    ActionActive,
    Swimming,
    Paused,
    ForeignAnimation,
    WrongEpoch,
    Defeated,
    Reloading,
    Cooldown,
    NoAmmo,
    MagazineFull,
    NoReserve,
    Rejected,
    Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FCoastalCombatStateChanged,
    float, Health, float, Shield, int32, Clip, int32, Reserve, bool, Armed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCoastalCombatNotice,
    ECoastalCombatResult, Result, FString, Detail);
// Presentation only, emitted after an authoritative combat state change.
DECLARE_MULTICAST_DELEGATE_TwoParams(FCoastalCombatPresentationFeedback, FName, FVector);

// Standalone encounter coordinator. Weapon and ammunition are transient encounter state.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALEXPANSION58_API UCoastalCombatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalCombatComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") TSubclassOf<AWeaponBase> WeaponClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FName WeaponAttachSocket = TEXT("hand_r");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") TSoftObjectPtr<UStaticMesh> WeaponVisualAsset;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FTransform WeaponVisualTransform;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FVector MuzzleOffset = FVector(25, 0, 0);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") TSoftObjectPtr<UStaticMesh> TracerAsset;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") TSoftObjectPtr<UMaterialInterface> TracerMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat", meta=(ClampMin="1")) float MaxHealth = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat", meta=(ClampMin="0")) float MaxShield = 50.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat", meta=(ClampMin="0.25", ClampMax="5")) float TracerRadiusCm = 1.25f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FKey FireKeyboardKey;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FKey FireGamepadKey;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FKey ReloadKeyboardKey;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FKey ReloadGamepadKey;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Combat") FString LastDetail;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Combat") FCoastalCombatStateChanged OnCombatStateChanged;
    UPROPERTY(BlueprintAssignable, Category="Coastal|Combat") FCoastalCombatNotice OnCombatNotice;
    FCoastalCombatPresentationFeedback OnCombatPresentation;

    bool InitializeCombat(UCoastalInteractionBridge* Interaction, UCoastalSaveCoordinator* Coordinator,
        UCoastalPlayerRecoveryComponent* RecoveryOwner, UCoastalCampingActionComponent* CampingOwner,
        UCoastalSwimmingComponent* SwimmingOwner);
    void ReleaseCombat();
    // Presentation follows a ready generated hand; weapon/ammo authority stays here.
    void RefreshWeaponAttachment();
    void InterruptAim();
    UFUNCTION(BlueprintCallable, Category="Coastal|Combat") ECoastalCombatResult TryFire();
    UFUNCTION(BlueprintCallable, Category="Coastal|Combat") ECoastalCombatResult TryReload();
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") bool IsAiming() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") bool IsInitialized() const { return BindingValid(); }
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") bool CanAttack() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") bool CanReceiveHostileAttack() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") bool IsEncounterActive() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") bool IsDefeated() const { return bDefeated; }
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") float GetHealth() const { return Health; }
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") float GetShield() const { return Shield; }
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") int32 GetCurrentAmmo() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") int32 GetReserveAmmo() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") AWeaponBase* GetTransientWeapon() const { return Weapon; }
    void EnterEncounter(AActor* Source);
    void LeaveEncounter(AActor* Source);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Tick) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    friend class ACoastalCombatDirector;
    UPROPERTY() TObjectPtr<ACharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Controller;
    UPROPERTY() TObjectPtr<UAdvancedShooterComponent> Shooter;
    UPROPERTY() TObjectPtr<AWeaponBase> Weapon;
    UPROPERTY() TObjectPtr<UStaticMesh> LoadedWeaponVisual;
    UPROPERTY() TObjectPtr<UStaticMesh> LoadedTracerMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> LoadedTracerMaterial;
    UPROPERTY() TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY() TObjectPtr<UCoastalSaveCoordinator> Saves;
    UPROPERTY() TObjectPtr<UCoastalPlayerRecoveryComponent> Recovery;
    UPROPERTY() TObjectPtr<UCoastalCampingActionComponent> Camping;
    UPROPERTY() TObjectPtr<UCoastalSwimmingComponent> Swimming;
    UPROPERTY() TObjectPtr<UEnhancedInputComponent> CombatInput;
    UPROPERTY() TObjectPtr<UInputMappingContext> CombatContext;
    UPROPERTY() TArray<TObjectPtr<UInputAction>> CombatActions;
    TSet<TWeakObjectPtr<AActor>> EncounterSources;
    coastal::EncounterEpoch EpochState;
    float Health = 0.f;
    float Shield = 0.f;
    double LastAcceptedShotTime = -1000.0;
    double ReloadUntil = -1.0;
    int32 LastPublishedClip = INDEX_NONE;
    int32 LastPublishedReserve = INDEX_NONE;
    bool bInitialized = false;
    bool bStopped = false;
    bool bDefeated = false;
    bool bInputInstalled = false;
    bool bSubmittingShot = false;
    bool bAimHeld = false;
    bool bAimNeedsRelease = false;
    bool BindingValid() const;
    bool HasForeignMontage() const;
    bool SpawnWeapon();
    void DestroyWeapon();
    bool InstallInput();
    void RemoveInput();
    coastal::CombatGateSample Sample() const;
    ECoastalCombatResult Emit(coastal::CombatDecision Decision, const FString& Detail);
    ECoastalCombatResult Emit(ECoastalCombatResult Result, const FString& Detail);
    void PublishState(bool Force = false);
    void SpawnTracer(const FVector& Start, const FVector& End);
    UFUNCTION() void InputFire();
    UFUNCTION() void InputReload();
    UFUNCTION() void InputAim();
    UFUNCTION() void InputAimReleased();
    UFUNCTION() void HandleDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
        AController* InstigatedBy, AActor* DamageCauser);
    UFUNCTION() void HandleReturn(ECoastalReturnNotice Result, FString Detail);
};
