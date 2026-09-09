#pragma once

#include "CoreMinimal.h"
#include "CoastalCompanionCommands.h"
#include "GameFramework/Actor.h"
#include "CoastalCompanionDirector.generated.h"

class ACoastalCompanionCharacter;
class UAnimSequence;
class USkeletalMesh;

// Map-owned bootstrap. The existing pause/input owner may call SetFollowing or ToggleFollowing.
UCLASS(Blueprintable)
class COASTALEXPANSION58_API ACoastalCompanionDirector : public AActor, public ICoastalCompanionCommands
{
    GENERATED_BODY()
public:
    ACoastalCompanionDirector();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion")
    TSubclassOf<ACoastalCompanionCharacter> CompanionClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Presentation")
    TSoftObjectPtr<USkeletalMesh> CompanionMeshAsset;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Presentation")
    TSoftObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Presentation")
    TSoftObjectPtr<UAnimSequence> WalkAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Presentation")
    TSoftObjectPtr<UAnimSequence> RunAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Companion|Presentation")
    FTransform MeshRelativeTransform = FTransform(FRotator(0, -90, 0), FVector(0, 0, -48), FVector::OneVector);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Companion")
    FString LastDetail;

    UFUNCTION(BlueprintCallable, Category="Coastal|Companion") bool SetFollowing(bool bShouldFollow);
    UFUNCTION(BlueprintCallable, Category="Coastal|Companion") bool ToggleFollowing();
    UFUNCTION(BlueprintPure, Category="Coastal|Companion") bool IsInitialized() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Companion") ACoastalCompanionCharacter* GetCompanion() const { return Companion; }

    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual bool CanCommandCompanion() const override;
    virtual bool IsCompanionFollowing() const override;
    virtual bool SetCompanionFollowing(bool bFollowing) override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() TObjectPtr<ACoastalCompanionCharacter> Companion;
    TWeakObjectPtr<ACharacter> BoundPlayer;
    bool TryInitialize();
    void DestroyOwnedCompanion();
};
