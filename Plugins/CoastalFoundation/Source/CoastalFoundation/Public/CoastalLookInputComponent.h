#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/PlayerOptionsRules.h"
#include "CoastalLookInputComponent.generated.h"
class UCoastalLocalOptions;
class UCoastalInteractionBridge;
class UCameraComponent;
class ACharacter;
class APlayerController;

// Optional fixed-character camera/look consumer. Does not install a second input mapping.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalLookInputComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalLookInputComponent();
    // Opt in only after routing the real mouse/stick actions below; not proof of vendor wiring.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Options") bool bUseHostLookEvents = false;
    UFUNCTION(BlueprintCallable, Category="Coastal|Options") bool SubmitMouseLook(FVector2D MappedDelta);
    UFUNCTION(BlueprintCallable, Category="Coastal|Options") bool SubmitStickLook(FVector2D MappedAxis);
    UFUNCTION(BlueprintPure, Category="Coastal|Options") bool IsCameraReady() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Options") bool IsLookReady() const;
    bool InitializeOptions(UCoastalLocalOptions* Options, UCoastalInteractionBridge* Interaction);
    bool ApplyCameraOptions();
    void ReleaseOptions();
    virtual void TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UPROPERTY() TObjectPtr<UCoastalLocalOptions> Profile;
    UPROPERTY() TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY() TObjectPtr<ACharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Controller;
    TWeakObjectPtr<UCameraComponent> Camera;
    coastal::LookInputGate Gate;
    float OriginalFov = 0, LastAppliedFov = 0;
    bool bInitialized = false, bStopped = false, bHostLookBound = false;
    void Synchronize();
};
