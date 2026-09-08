#pragma once
#include "Components/ActorComponent.h"
#include "CoastalHostControls.generated.h"
class APlayerController;
class UEnhancedPlayerInput;
class UInputAction;
class UInputModifierDeadZone;
class UCoastalLookInputComponent;
class UCoastalSprintComponent;

// Fixed Third Person host input adapter. Existing mappings still own movement,
// jump and mouse orientation; this replaces only the two original look callbacks.
UCLASS(ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALVENDORINTEGRATION_API UCoastalHostControls : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalHostControls();
    bool Install(UCoastalLookInputComponent* Look, UCoastalSprintComponent* Sprint);
    virtual void TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Controls") bool Installed=false;
private:
    UPROPERTY(Transient) TObjectPtr<APlayerController> Controller;
    UPROPERTY(Transient) TObjectPtr<UEnhancedPlayerInput> Input;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MouseAction;
    UPROPERTY(Transient) TObjectPtr<UInputModifierDeadZone> StickDeadZone;
    UPROPERTY(Transient) TObjectPtr<UCoastalLookInputComponent> LookInput;
    UPROPERTY(Transient) TObjectPtr<UCoastalSprintComponent> SprintInput;
};
