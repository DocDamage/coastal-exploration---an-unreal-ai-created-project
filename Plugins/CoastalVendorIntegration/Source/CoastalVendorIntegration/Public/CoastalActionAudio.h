#pragma once
#include "Components/ActorComponent.h"
#include "CoastalInteractionBridge.h"
#include "CoastalActionAudio.generated.h"
class UCoastalAudioOptionsComponent;
class UCoastalUISessionComponent;
class USoundBase;
class UAudioComponent;

// Sole bounded interaction/menu feedback voice; no inventory or mission authority.
UCLASS(ClassGroup=(Coastal),meta=(BlueprintSpawnableComponent))
class COASTALVENDORINTEGRATION_API UCoastalActionAudio : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalActionAudio();
    bool Initialize(UCoastalInteractionBridge* Bridge,UCoastalAudioOptionsComponent* Routing,
        UCoastalUISessionComponent* UI,const TMap<FName,USoundBase*>& Sounds);
    static FName ResultCue(ECoastalActionResult Result,FName SuccessCue);
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintPure,Category="Coastal|Audio") UAudioComponent* GetFeedbackVoice() const { return Voice; }
    // Submissions are diagnostics, not proof of audible output.
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Coastal|Audio") int32 SubmissionCount = 0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Coastal|Audio") FName LastCue;
private:
    void Action(ECoastalActionResult Result,FName Cue,FVector Location);
    void Menu(FName Cue);
    void Play(FName Cue,FVector Location,bool bSpatial);
    void Stop();
    UPROPERTY(Transient) TObjectPtr<UCoastalInteractionBridge> Source;
    UPROPERTY(Transient) TObjectPtr<UCoastalAudioOptionsComponent> Options;
    UPROPERTY(Transient) TObjectPtr<UCoastalUISessionComponent> Menus;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<USoundBase>> Clips;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Voice;
    FDelegateHandle ReleaseHandle,ActionHandle,MenuHandle;
    uint64 VoiceEpoch = 0,LastFrame = MAX_uint64;
    bool bWorldVoice = false;
};
