#pragma once
#include "CoreMinimal.h"
#include "WeaponBase.h"
#include "GameFramework/Actor.h"
#include "CoastalCombatActors.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class COASTALEXPANSION58_API ACoastalSidearmWeapon : public AWeaponBase
{
    GENERATED_BODY()
public:
    ACoastalSidearmWeapon();
    virtual FVector GetMuzzleSocketLocation() const override;
    void ConfigurePresentation(UStaticMesh* Mesh, FTransform VisualTransform, FVector MuzzleOffset);
    bool IsPresentationReady() const;
private:
    UPROPERTY(VisibleAnywhere, Category="Coastal|Combat") TObjectPtr<UStaticMeshComponent> VisualMesh;
    UPROPERTY(VisibleAnywhere, Category="Coastal|Combat") TObjectPtr<USceneComponent> MuzzleAnchor;
};

UCLASS(NotBlueprintable)
class COASTALEXPANSION58_API ACoastalCombatTracer : public AActor
{
    GENERATED_BODY()
public:
    ACoastalCombatTracer();
    bool Configure(UStaticMesh* Mesh, UMaterialInterface* Material,
        const FVector& Start, const FVector& End, float RadiusCm, float LifetimeSeconds);
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> TracerMesh;
};

UCLASS(Blueprintable)
class COASTALEXPANSION58_API ACoastalCombatTarget : public AActor
{
    GENERATED_BODY()
public:
    ACoastalCombatTarget();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat", meta=(ClampMin="1"))
    float MaxHealth = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat")
    TSoftObjectPtr<UStaticMesh> PresentationAsset;
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") bool IsDefeated() const { return bDefeated; }
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") float GetHealth() const { return Health; }
    virtual void BeginPlay() override;
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Combat")
    TObjectPtr<UStaticMeshComponent> TargetMesh;
    UFUNCTION() void HandleDamage(AActor* DamagedActor, float Damage,
        const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
    float Health = 0.f;
    bool bDefeated = false;
};

UCLASS(Blueprintable)
class COASTALEXPANSION58_API ACoastalCombatSentry : public ACoastalCombatTarget
{
    GENERATED_BODY()
public:
    ACoastalCombatSentry();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat", meta=(ClampMin="0.25"))
    float AttackIntervalSeconds = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat", meta=(ClampMin="1"))
    float AttackDamage = 12.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat", meta=(ClampMin="100", ClampMax="5000"))
    float EngagementRangeCm = 1400.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat", meta=(ClampMin="100", ClampMax="10000"))
    float EncounterRadiusCm = 1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat")
    bool bRequireRecentlyRendered = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat")
    TSoftObjectPtr<UStaticMesh> TracerAsset;
    virtual void BeginPlay() override;
private:
    FVector Home = FVector::ZeroVector;
    FTimerHandle AttackTimer;
    void AttemptAttack();
};

UCLASS(Blueprintable)
class COASTALEXPANSION58_API ACoastalCombatEncounterVolume : public AActor
{
    GENERATED_BODY()
public:
    ACoastalCombatEncounterVolume();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Combat") TObjectPtr<UBoxComponent> Bounds;
    UFUNCTION(BlueprintPure, Category="Coastal|Combat") bool IsAuthoredCorrectly() const;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UFUNCTION() void Enter(UPrimitiveComponent* Overlapped, AActor* Other,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);
    UFUNCTION() void Leave(UPrimitiveComponent* Overlapped, AActor* Other,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);
};
