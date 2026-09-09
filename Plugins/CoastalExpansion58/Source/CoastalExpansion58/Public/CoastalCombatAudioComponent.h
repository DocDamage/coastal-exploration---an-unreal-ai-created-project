#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalCombatAudioComponent.generated.h"

class UAudioComponent;
class UCoastalAudioOptionsComponent;
class UCoastalCombatComponent;
class UCoastalUISessionComponent;
class USoundBase;

// One transient Effects-routed voice for semantically supported combat feedback.
UCLASS(ClassGroup=(Coastal))
class COASTALEXPANSION58_API UCoastalCombatAudioComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalCombatAudioComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat|Audio")
    TSoftObjectPtr<USoundBase> EncounterStartSound;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Combat|Audio")
    TSoftObjectPtr<USoundBase> PlayerHitSound;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Combat|Audio") int32 SubmissionCount = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Combat|Audio") FName LastCue;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal|Combat|Audio") FString LastDetail;

    bool InitializeAudio(UCoastalCombatComponent* Combat, UCoastalUISessionComponent* UI,
        UCoastalAudioOptionsComponent* Routing);
    void ReleaseAudio();
    UFUNCTION(BlueprintPure, Category="Coastal|Combat|Audio") bool IsAudioReady() const;
    UFUNCTION(BlueprintPure, Category="Coastal|Combat|Audio") UAudioComponent* GetVoice() const { return Voice; }
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    UPROPERTY(Transient) TObjectPtr<UCoastalCombatComponent> Source;
    UPROPERTY(Transient) TObjectPtr<UCoastalUISessionComponent> Menus;
    UPROPERTY(Transient) TObjectPtr<UCoastalAudioOptionsComponent> Options;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<USoundBase>> Clips;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Voice;
    FDelegateHandle CueHandle;
    FDelegateHandle ReleaseHandle;
    uint64 VoiceEpoch = 0;
    bool bStopped = false;

    void Cue(FName Name, FVector Location);
    void StopVoice();
};
