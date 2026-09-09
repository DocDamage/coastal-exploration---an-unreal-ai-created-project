#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AudioDeviceHandle.h"
#include "CoastalSoundscape.generated.h"
class UAudioComponent;
class USoundBase;
class USoundClass;
class UCoastalAudioOptionsComponent;
class UCoastalUISessionComponent;

// Presentation only: one destination score and one sparse thunder voice on existing buses.
UCLASS(ClassGroup=(Coastal))
class COASTALVENDORINTEGRATION_API UCoastalSoundscape : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalSoundscape();
    bool Initialize(UCoastalUISessionComponent* UI,UCoastalAudioOptionsComponent* Routing);
    UFUNCTION(BlueprintPure) UAudioComponent* GetMusicVoice() const { return Music; }
    UFUNCTION(BlueprintPure) UAudioComponent* GetThunderVoice() const { return Thunder; }
    UPROPERTY(BlueprintReadOnly) FName CurrentScore;
    UPROPERTY(BlueprintReadOnly) float MusicGain = 0;
    UPROPERTY(BlueprintReadOnly) float ShelterGain = 1;
    UPROPERTY(BlueprintReadOnly) int32 MusicSubmissions = 0;
    UPROPERTY(BlueprintReadOnly) int32 ThunderSubmissions = 0;
    UPROPERTY(BlueprintReadOnly) bool bReady = false;
    static bool WithinZone(FVector Position,FVector Center,float Radius,bool Retained);
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function) override;
    virtual void OnUnregister() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    struct FZone { FName Score; FVector Center; float Radius; };
    TArray<FZone> Zones;
    UPROPERTY(Transient) TObjectPtr<UCoastalUISessionComponent> Menus;
    UPROPERTY(Transient) TObjectPtr<UCoastalAudioOptionsComponent> Options;
    UPROPERTY(Transient) TObjectPtr<APawn> Pawn;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<USoundBase>> Clips;
    UPROPERTY(Transient) TObjectPtr<USoundClass> MusicBus;
    UPROPERTY(Transient) TObjectPtr<USoundClass> ThunderBus;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Music;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Thunder;
    FAudioDeviceHandle Device;
    FDelegateHandle ReleaseHandle;
    uint64 Epoch = 0;
    float ThunderWait = 20, ThunderAge = 0, CoverWait = 0, CoverTarget = 1;
    bool bReleased = false;
    FName SelectScore(FVector Position) const;
    UAudioComponent* CreateVoice(USoundBase* Sound,USoundClass* Bus);
    void DestroyVoice(TObjectPtr<UAudioComponent>& Voice);
    void ResetVoices();
    void Release();
};
