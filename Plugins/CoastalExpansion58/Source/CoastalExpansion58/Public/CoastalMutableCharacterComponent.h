#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CoastalCharacterCreator.h"
#include "CoastalAppearanceTypes.h"
#include "CoastalInteractionBridge.h"
#include "CoastalMutableCharacterComponent.generated.h"

class ACharacter;
class UCoastalInteractionBridge;
class UCoastalSaveCoordinator;
class UCoastalCombatComponent;
class UCustomizableObjectInstanceUsage;
class USceneCaptureComponent2D;
class UDirectionalLightComponent;
class USkeletalMeshComponent;
class UAnimMontage;
class UEnhancedInputComponent;
class UInputMappingContext;
class UInputAction;
class APlayerController;
struct FUpdateContext;

UCLASS(ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALEXPANSION58_API UCoastalMutableCharacterComponent : public UActorComponent, public ICoastalCharacterCreator
{
    GENERATED_BODY()
public:
    UCoastalMutableCharacterComponent();
    UPROPERTY(EditAnywhere) TObjectPtr<UCoastalCharacterDefinition> Definition;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString LastDetail;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName ActiveAction;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ActionPlayCount = 0;
    UFUNCTION(BlueprintPure) bool IsAppearanceReady() const { return bReady; }
    UFUNCTION(BlueprintPure) bool IsGenerating() const { return bGenerating; }
    UFUNCTION(BlueprintPure) USkeletalMeshComponent* GetGeneratedBody() const;
    UFUNCTION(BlueprintCallable) bool PlayNextEmote();
    virtual bool CanEditCharacter() const override;
    virtual bool BeginCharacterEdit() override;
    virtual void EndCharacterEdit() override;
    virtual void PresentCharacter(FText& Body, TArray<FCoastalUIChoice>& Choices) override;
    virtual void CharacterCommand(FName Command) override;
    virtual UTextureRenderTarget2D* CharacterPreview() const override;
    virtual FText CharacterStatus() const override { return FText::FromString(LastDetail); }
    virtual FText CharacterActionHint(bool Gamepad) const override;
    virtual void TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function) override;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    bool BindingValid() const;
    bool ValidateSelections(const TArray<int32>& Values) const;
    int32 OptionCount(const FCoastalAppearanceControl& Control) const;
    bool LoadAppearance();
    bool SaveAppearance();
    FString Slot(int32 Index) const;
    void RequestGeneration(const TArray<int32>& Values);
    void RefreshPresentation();
    void UpdatePreview();
    void ClearGenerated();
    void ClearPendingParts();
    void UpdateActions();
    void UpdateGestures();
    bool CanPlayGesture() const;
    void RemoveGestureInput();
    void InputEmote();
    void UpdateWeaponPose(bool Allowed);
    void StopAction(bool ClearQueued = true);
    void InteractionFeedback(ECoastalActionResult Result, FName Cue, FVector Location);
    void CombatFeedback(FName Cue, FVector Location);
    UFUNCTION() void Generated(const FUpdateContext& Result);
    UPROPERTY(Transient) TObjectPtr<ACharacter> Character;
    UPROPERTY(Transient) TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY(Transient) TObjectPtr<UCoastalSaveCoordinator> Saves;
    UPROPERTY(Transient) TObjectPtr<UCoastalCombatComponent> Combat;
    UPROPERTY(Transient) TObjectPtr<UCustomizableObjectInstance> CurrentInstance;
    UPROPERTY(Transient) TObjectPtr<UCustomizableObjectInstance> PendingInstance;
    UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMeshComponent>> Parts;
    UPROPERTY(Transient) TArray<TObjectPtr<UCustomizableObjectInstanceUsage>> Usages;
    UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMeshComponent>> PendingParts;
    UPROPERTY(Transient) TArray<TObjectPtr<UCustomizableObjectInstanceUsage>> PendingUsages;
    UPROPERTY(Transient) TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> PreviewTexture;
    UPROPERTY(Transient) TArray<TObjectPtr<UDirectionalLightComponent>> PreviewLights;
    TArray<int32> Applied, Draft, GeneratedSelections, PendingSelections;
    FGuid Campaign, PendingCampaign;
    uint64 Epoch = 0, PendingEpoch = 0;
    int64 Generation = 0;
    int32 SelectedControl = 0;
    bool bReady = false, bGenerating = false, bEditing = false, bCanWrite = true;
    bool bGenerationRequested = false;
    bool bCompileRequested = false;
    bool bOriginalHidden = false;
    EVisibilityBasedAnimTickOption OriginalTickOption;
    float PreviewYaw = 0.f;
    FDelegateHandle InteractionHandle, CombatHandle;
    FName QueuedAction;
    UPROPERTY(Transient) TArray<TObjectPtr<UAnimMontage>> ActionMontages;
    bool bActionUpperBody = false;
    TWeakObjectPtr<AActor> PresentedWeapon;
    bool bPresentedWeapon = false;
    double QueuedUntil = 0.0, ActionUntil = 0.0, NextPreviewCapture = 0.0;
    UPROPERTY(Transient) TObjectPtr<UEnhancedInputComponent> GestureInput;
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> GestureContext;
    UPROPERTY(Transient) TObjectPtr<UInputAction> EmoteInputAction;
    TWeakObjectPtr<APlayerController> GestureController;
    int32 NextEmote = 0, NextIdleVariation = 0;
    double NextIdleTime = 0.0;
    bool bEmoteNeedsRelease = false;
};
