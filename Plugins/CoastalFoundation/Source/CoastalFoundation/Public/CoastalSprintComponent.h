#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/SprintRules.h"
#include "CoastalSprintComponent.generated.h"
class UCoastalLocalOptions;
class UCoastalInteractionBridge;
class UCharacterMovementComponent;
class ACharacter;
class APlayerController;

// Optional single-player speed owner. Existing host movement/jump/look mappings remain authoritative.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalSprintComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalSprintComponent();
    // Enable only after routing actual aggregate Sprint state and removing other MaxWalkSpeed writers.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Sprint") bool bUseHostSprintEvents = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Sprint", meta=(ClampMin="100", ClampMax="1000"))
    float WalkSpeed = 330.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Sprint", meta=(ClampMin="100", ClampMax="1500"))
    float SprintSpeed = 520.0f;
    UPROPERTY(BlueprintReadOnly, Category="Coastal|Sprint") FString LastDetail;
    // Return means a state sample was accepted, not that the character moved or accelerated.
    UFUNCTION(BlueprintCallable, Category="Coastal|Sprint") bool SubmitSprintInput(bool bActionDown);
    UFUNCTION(BlueprintPure, Category="Coastal|Sprint") bool IsSprintReady() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Sprint") bool IsSprintRequested() const;
    bool InitializeOptions(UCoastalLocalOptions* Options, UCoastalInteractionBridge* Interaction);
    void ApplySprintOptions();
    void ReleaseOptions();
    virtual void TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UPROPERTY() TObjectPtr<UCoastalLocalOptions> Profile;
    UPROPERTY() TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY() TObjectPtr<ACharacter> Character;
    UPROPERTY() TObjectPtr<APlayerController> Controller;
    UPROPERTY() TObjectPtr<UCharacterMovementComponent> Movement;
    coastal::SprintGate Gate;
    coastal::WalkSpeedLease SpeedLease;
    bool bInitialized = false, bStopped = false, bFailed = false;
    bool BindingValid() const;
    bool SprintPermitted() const;
    void Synchronize();
    void ApplySpeed();
    void StopWithDiagnostic(const TCHAR* Detail);
};
