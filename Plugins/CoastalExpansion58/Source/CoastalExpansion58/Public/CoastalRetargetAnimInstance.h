#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CoastalRetargetAnimInstance.generated.h"

class UIKRetargeter;
class UAnimSequence;

// Presentation only: retarget the pose produced by the existing player owners.
UCLASS(Transient)
class COASTALEXPANSION58_API UCoastalRetargetAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> SourceMesh;
    UPROPERTY(Transient) TObjectPtr<UIKRetargeter> Retargeter;
    UPROPERTY(Transient, BlueprintReadOnly) TObjectPtr<UAnimSequence> WeaponPose;
    UPROPERTY(Transient, BlueprintReadOnly) float WeaponWeight = 0.f;
    UPROPERTY(Transient, BlueprintReadOnly) bool bWeaponPoseLooping = true;
    UPROPERTY(Transient, BlueprintReadOnly) float WeaponPoseTime = 0.f;
    UPROPERTY(Transient, BlueprintReadOnly) TObjectPtr<UAnimSequence> ActivityPose;
    UPROPERTY(Transient, BlueprintReadOnly) float ActivityWeight = 0.f;
    UPROPERTY(Transient, BlueprintReadOnly) bool bActivityTimeDriven = false;
    UPROPERTY(Transient, BlueprintReadOnly) float ActivityTime = 0.f;
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
