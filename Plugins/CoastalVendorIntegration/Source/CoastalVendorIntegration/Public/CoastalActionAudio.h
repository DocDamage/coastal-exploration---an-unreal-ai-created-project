#pragma once
#include "Components/ActorComponent.h"
#include "CoastalInteractionBridge.h"
#include "CoastalActionAudio.generated.h"
class UCoastalAudioOptionsComponent;
class USoundBase;
class UAudioComponent;

// One bounded confirmation sound for successful actions; no mission authority.
UCLASS(ClassGroup=(Coastal),meta=(BlueprintSpawnableComponent))
class COASTALVENDORINTEGRATION_API UCoastalActionAudio : public UActorComponent
{
    GENERATED_BODY()
public:
    bool Initialize(UCoastalInteractionBridge* Bridge,UCoastalAudioOptionsComponent* Routing,USoundBase* Sound);
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UFUNCTION() void Action(ECoastalActionResult Result);
    void Stop();
    UPROPERTY(Transient) TObjectPtr<UCoastalInteractionBridge> Source;
    UPROPERTY(Transient) TObjectPtr<UCoastalAudioOptionsComponent> Options;
    UPROPERTY(Transient) TObjectPtr<USoundBase> Clip;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Voice;
    FDelegateHandle ReleaseHandle;
};
