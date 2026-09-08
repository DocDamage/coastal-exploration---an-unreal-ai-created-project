#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AudioDeviceHandle.h"
#include "Core/AudioOptionsRules.h"
#include "CoastalAudioOptionsComponent.generated.h"
class UCoastalLocalOptions;
class USoundClass;
class USoundMix;
class APlayerController;
DECLARE_MULTICAST_DELEGATE(FCoastalAudioRoutingReleased);

// Optional LOCAL CONTROLLER component. This controls routed sounds; it does not play or discover them.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalAudioOptionsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalAudioOptionsComponent();
    // Opt in only after routing actual host sounds to three dedicated independent classes.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Audio") bool bUseDedicatedSoundClasses = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Audio") TObjectPtr<USoundClass> AmbienceClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Audio") TObjectPtr<USoundClass> EffectsClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Audio") TObjectPtr<USoundClass> RadioClass;
    UPROPERTY(BlueprintReadOnly, Category="Coastal|Audio") FString LastDetail;
    // Called by the existing native UI preference owner; not a second manual startup call.
    bool InitializeOptions(UCoastalLocalOptions* Options);
    // Readiness means command prerequisites, NOT proof a sound is routed or audible.
    UFUNCTION(BlueprintPure, Category="Coastal|Audio") bool IsAudioReady() const;
    bool ApplyAudioOptions();
    // Captured, validated routes, not the editable properties after initialization.
    USoundClass* GetAmbienceClassForPlayback() const;
    USoundClass* GetEffectsClassForPlayback() const;
    USoundClass* GetRadioClassForPlayback() const;
    // Presentation owners stop sources BEFORE this owner removes its volume contribution.
    FCoastalAudioRoutingReleased OnRoutingReleased;
    void ReleaseOptions();
    virtual void TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnUnregister() override;
private:
    UPROPERTY(Transient) TObjectPtr<UCoastalLocalOptions> Profile;
    UPROPERTY(Transient) TObjectPtr<APlayerController> Controller;
    UPROPERTY(Transient) TObjectPtr<USoundMix> OwnedMix;
    UPROPERTY(Transient) TArray<TObjectPtr<USoundClass>> BoundClasses;
    FAudioDeviceHandle Device; // Strong reference permits cleanup on the SAME device after a world-device change.
    coastal::AudioMixSession MixState;
    bool bInitialized = false, bStopped = false;
    bool ClassesValid() const;
    void StopWithDiagnostic(const TCHAR* Detail);
};
