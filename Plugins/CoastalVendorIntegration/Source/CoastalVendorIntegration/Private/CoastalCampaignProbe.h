#pragma once
#include "CoreMinimal.h"
#include "Core/DisplaySettingsRules.h"
#include "CoastalCampaignTypes.h"
class UCoastalHostSession;
// Opt-in development acceptance driver. Uses rendered native buttons and real
// Enhanced Input events; positioning is deterministic, not a locomotion test.
struct FCoastalCampaignProbe
{
    explicit FCoastalCampaignProbe(const FString& Mode);
    void Tick(UCoastalHostSession* Host);
    struct FStep { FString Verb; FName Argument; };
    TArray<FStep> Steps;
    FString Mode,Slot;
    int32 Step=0,Phase=0;
    double Deadline=0,Next=0;
    bool Finished=false;
    int64 BeforeSave=0;
    FGuid ExpectedCampaign;
    TArray<uint8> BeforeReturn;
    bool RunControlsStep(UCoastalHostSession* Host,FName Kind,FString& Error);
    FRotator BeforeRotation;
    FVector BeforePosition;
    double SampleStarted=0, MouseBaseline=0;
    bool RunDisplayStep(UCoastalHostSession* Host,FName Kind,FString& Error);
    bool ConfigureSaveSteps();
    bool ConfigureCapacitySteps();
    bool ConfigureOptionsSteps();
    bool ConfigureLiveAudioSteps();
    void TickMissingProvider(UCoastalHostSession* Host);
    bool RunLiveAudioStep(UCoastalHostSession* Host,FName Kind,FString& Error);
    uint32 LiveAudioVoiceId=0;
    bool RunOptionsStep(UCoastalHostSession* Host,FName Kind,FString& Error);
    bool RunCapacityStep(UCoastalHostSession* Host,FName Kind,FString& Error);
    bool RunStorageCapacityStep(UCoastalHostSession* Host,FName Kind,FString& Error);
    FCoastalInventorySnapshot CapacityBefore;
    int64 CapacityGeneration=0;
    bool RunSaveStep(UCoastalHostSession* Host,FName Kind,FString& Error);
    bool RunSaveFaultStep(UCoastalHostSession* Host,FName Kind,FString& Error);
    bool ClearSaveFault();
    bool VerifyRejectedSave(UCoastalHostSession* Host,FString& Error);
    bool NavigateGamepad(UCoastalHostSession* Host,FName Command);
    void PrependGamepadOptions();
    FName PendingGamepadCommand;
    uint64 PendingGamepadTicket=0;
    TArray<FString> FaultPaths;
    TArray<TArray<uint8>> FaultDisk;
    coastal::DisplayMode BeforeDisplay,SavedDisplay;
};
