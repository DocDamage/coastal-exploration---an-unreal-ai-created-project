#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalUISessionComponent.h"
#include "CoastalLocalOptions.h"
#include "CoastalAGISAdapter.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalInteractionBridge.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
bool FCoastalCampaignProbe::ConfigureLiveAudioSteps()
{
    if(Mode!=TEXT("live_audio"))return false;
    auto Click=[&](const TCHAR* Command,int32 Count=1){for(int32 I=0;I<Count;++I)Steps.Add({TEXT("click"),Command});};
    Click(TEXT("options"));Steps.Add({TEXT("live_audio"),TEXT("capture")});Click(TEXT("option_defaults"));
    Click(TEXT("option_next"),7);Click(TEXT("option_decrease"),10); // Ambience 50.
    Click(TEXT("option_next"));Click(TEXT("option_decrease"),5); // Effects 75.
    Click(TEXT("option_next"));Click(TEXT("option_decrease"),15); // Radio 25.
    Click(TEXT("option_session"));Click(TEXT("back"));Click(TEXT("start"));Steps.Add({TEXT("check"),TEXT("new")});
    Steps.Add({TEXT("live_audio"),TEXT("baseline")});
    for(const TCHAR* Label:{TEXT("muted"),TEXT("restored")})
    {
        Steps.Add({TEXT("pause"),NAME_None});Click(TEXT("options"));Click(TEXT("option_next"),6);
        Click(FCString::Strcmp(Label,TEXT("muted"))==0?TEXT("option_decrease"):TEXT("option_increase"),20);
        Click(TEXT("option_session"));Click(TEXT("back"));Click(TEXT("back"));
        Steps.Add({TEXT("live_audio"),Label});
    }
    Steps.Add({TEXT("pause"),NAME_None});Click(TEXT("quit"));Click(TEXT("exit_without_save"));return true;
}
bool FCoastalCampaignProbe::RunLiveAudioStep(UCoastalHostSession* Host,FName Kind,FString& Error)
{
    auto* PC=Cast<APlayerController>(Host->GetOwner());auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();
    UCoastalLocalOptions* Profile=nullptr;
    for(TObjectIterator<UCoastalLocalOptions> It;It;++It)if(It->GetOuter()==UI)Profile=*It;
    if(!Profile){Error=TEXT("Missing live options owner");return false;}
    TArray<TArray<uint8>> Disk;
    for(const TCHAR* Name:{TEXT("CoastalLocalOptions_v1_A"),TEXT("CoastalLocalOptions_v1_B")})
    {
        TArray<uint8> Bytes;
        if(UGameplayStatics::DoesSaveGameExist(Name,0) && !UGameplayStatics::LoadDataFromSlot(Bytes,Name,0))
        {Error=TEXT("Preference read failed during live audio test");return false;}
        Disk.Add(MoveTemp(Bytes));
    }
    if(Kind==TEXT("capture")){FaultDisk=Disk;return true;}
    const auto& Values=Profile->Get();
    if(Disk!=FaultDisk || Values.masterPercent!=(Kind==TEXT("muted")?0:100) || Values.ambiencePercent!=50
        || Values.effectsPercent!=75 || Values.radioPercent!=25)
    {Error=TEXT("Live mute/unmute changed disk or discarded category settings");return false;}
    APawn* Pawn=PC->GetPawn();auto* Provider=Pawn->FindComponentByClass<UCoastalAGISAdapter>();
    auto* Saves=Pawn->FindComponentByClass<UCoastalInteractionBridge>()->GetCoordinator();
    FCoastalInventorySnapshot Snapshot;
    if(Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready){Error=TEXT("Cannot read inventory");return false;}
    UAudioComponent* Voice=nullptr;int32 Count=0;
    for(TObjectIterator<UAudioComponent> It;It;++It)
        if(It->GetWorld()==Host->GetWorld() && It->Sound && It->Sound->GetName()==TEXT("SW_M1Ambience"))
        {Voice=*It;++Count;}
    if(Count!=1 || !Voice->IsPlaying() || (LiveAudioVoiceId && LiveAudioVoiceId!=Voice->GetUniqueID()))
    {Error=TEXT("Live mute/unmute lost, duplicated or replaced the actual ambience voice");return false;}
    LiveAudioVoiceId=Voice->GetUniqueID();
    if(!Phase)
    {
        BeforeReturn=Snapshot.Payload;BeforeSave=Saves->GetGeneration();
        UAudioMixerBlueprintLibrary::StartRecordingOutput(Host,15);
        Phase=1;const double Now=FPlatformTime::Seconds();Next=Now+10;Deadline=Now+20;return false;
    }
    if(Snapshot.Payload!=BeforeReturn || Saves->GetGeneration()!=BeforeSave)
    {Error=TEXT("Live audio sample changed campaign state");return false;}
    UAudioMixerBlueprintLibrary::StopRecordingOutput(Host,EAudioRecordingExportType::WavFile,Slot+TEXT("-")+Kind.ToString(),
        FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"))));
    UE_LOG(LogTemp,Display,TEXT("COASTAL_LIVE_AUDIO_PASS %s Master=%d Ambience=50 Effects=75 Radio=25 same_voice=%u disk_and_inventory_unchanged"),
        *Kind.ToString(),Values.masterPercent,LiveAudioVoiceId);return true;
}
#endif
