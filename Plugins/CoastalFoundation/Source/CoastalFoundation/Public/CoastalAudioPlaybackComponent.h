#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AudioDeviceHandle.h"
#include "Core/AudioPlaybackRules.h"
#include "CoastalAudioPlaybackComponent.generated.h"
class UAudioComponent;
class USoundBase;
class USoundClass;
class UCoastalAudioOptionsComponent;
class UCoastalUISessionComponent;

// Optional LOCAL CONTROLLER presentation owner; never mission or inventory authority.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalAudioPlaybackComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalAudioPlaybackComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Playback") bool bEnablePlayback = false;
    // Assign actual owned project sounds. Either slot may be absent. No paths are invented.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Playback") TObjectPtr<USoundBase> AmbienceLoop;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Playback") TObjectPtr<USoundBase> RadioTransmission;
    UPROPERTY(BlueprintReadOnly, Category="Coastal|Playback") FString LastDetail;
    // Called by the native UI after its routed options are initialized; not a host BeginPlay call.
    bool InitializePlayback(UCoastalUISessionComponent* UI, UCoastalAudioOptionsComponent* Routing);
    UFUNCTION(BlueprintPure, Category="Coastal|Playback") bool IsPlaybackReady() const;
    void RefreshPlayback();
    void SuspendPlayback();
    void ReleasePlayback();
    virtual void TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void OnUnregister() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UPROPERTY(Transient) TObjectPtr<UCoastalUISessionComponent> Session;
    UPROPERTY(Transient) TObjectPtr<UCoastalAudioOptionsComponent> AudioOptions;
    UPROPERTY(Transient) TObjectPtr<USoundBase> BoundAmbience;
    UPROPERTY(Transient) TObjectPtr<USoundBase> BoundRadio;
    UPROPERTY(Transient) TObjectPtr<USoundClass> AmbienceBus;
    UPROPERTY(Transient) TObjectPtr<USoundClass> RadioBus;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> AmbienceVoice;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> RadioVoice;
    FAudioDeviceHandle Device;
    FDelegateHandle RoutingReleasedHandle;
    coastal::AudioPlaybackSession Playback;
    bool bInitialized = false, bStopped = false, bRefreshing = false;
    bool SourcesValid() const;
    UAudioComponent* CreateVoice(USoundBase* Sound, USoundClass* Bus, bool bRadio);
    void DestroyVoice(TObjectPtr<UAudioComponent>& Voice);
    void Execute(const coastal::AudioPlaybackCommands& Commands);
    void RoutingReleased();
    void Fail(const TCHAR* Detail);
};
