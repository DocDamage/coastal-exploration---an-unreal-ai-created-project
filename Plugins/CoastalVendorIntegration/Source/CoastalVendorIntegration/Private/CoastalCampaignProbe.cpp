#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalAGISAdapter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalInteractionRelayComponent.h"
#include "CoastalUISessionComponent.h"
#include "CoastalPanelWidget.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalWorldObject.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "FirstSignalComponent.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "InputKeyEventArgs.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "AudioMixerBlueprintLibrary.h"
#include "AudioDevice.h"
#include "Engine/World.h"
#include "Components/AudioComponent.h"
#include "Components/EditableTextBox.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundBase.h"
#include "Misc/App.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "HAL/FileManager.h"
namespace
{
bool Click(UWorld* World,FName Command)
{
    for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
    {
        auto* Panel=*It;
        if(Panel->GetWorld()!=World || !Panel->IsInViewport() || !Panel->IsVisible() || !Panel->HasBeenPresented() || !Panel->WidgetTree)continue;
        UCoastalCommandButton* Found=nullptr;
        Panel->WidgetTree->ForEachWidget([&](UWidget* W){if(auto* B=Cast<UCoastalCommandButton>(W))if(B->Command==Command && B->GetIsEnabled())Found=B;});
        if(Found){Found->OnClicked.Broadcast();return true;}
    }
    return false;
}
ACoastalWorldObject* Target(UWorld* World,FName Id)
{ for(TActorIterator<ACoastalWorldObject> It(World);It;++It)if(It->WorldId==Id)return *It;return nullptr; }
void Key(APlayerController* PC,FKey K,bool Down)
{ PC->InputKey(FInputKeyEventArgs(nullptr,IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(),K,Down?IE_Pressed:IE_Released,Down?1.0f:0.0f,false,FPlatformTime::Cycles64())); }
FString EvidencePath(const FString& Slot,const FString& Suffix)
{ return FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot+Suffix); }
void Screenshot(const FString& Slot,const FString& Label)
{ FScreenshotRequest::RequestScreenshot(EvidencePath(Slot,TEXT("-")+Label+TEXT(".png")),true,false); }
}
FCoastalCampaignProbe::FCoastalCampaignProbe(const FString& InMode):Mode(InMode)
{
    FParse::Value(FCommandLine::Get(),TEXT("CoastalAcceptanceSlot="),Slot);
    if(Slot.IsEmpty() || Slot.Len()>48 || Slot.Contains(TEXT("..")) || !Slot.StartsWith(TEXT("coastal_test_"))) {Finished=true;return;}
    for(TCHAR C:Slot)if(!FChar::IsAlnum(C) && C!='_'){Finished=true;return;}
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(EvidencePath(Slot,TEXT(".txt"))),true);
    if(Mode==TEXT("new") || Mode==TEXT("audio") || Mode==TEXT("gamepad_ui") || Mode==TEXT("transcript_stale") || Mode==TEXT("order_new"))
    {
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},
            {TEXT("use"),TEXT("world.test.radio")},{TEXT("check"),TEXT("inspected")},
            {TEXT("use"),TEXT("world.test.battery")},{TEXT("use"),TEXT("world.test.fuse")},{TEXT("check"),TEXT("collected")},
            {TEXT("use"),TEXT("world.test.storage")},{TEXT("click"),TEXT("transfer")},{TEXT("check"),TEXT("stored")},
            {TEXT("click"),TEXT("switch_side")},{TEXT("click"),TEXT("transfer")},{TEXT("check"),TEXT("collected")},{TEXT("click"),TEXT("back")},
            {TEXT("use"),TEXT("world.test.radio")},{TEXT("check"),TEXT("repaired")},
            {TEXT("use"),TEXT("world.test.radio")},{TEXT("check"),TEXT("unacknowledged")},
            {TEXT("click"),TEXT("acknowledge")},{TEXT("check"),TEXT("complete")},
            {TEXT("hazard"),NAME_None},{TEXT("pause"),NAME_None},{TEXT("click"),TEXT("save")},{TEXT("check"),TEXT("saved")},
            {TEXT("finish"),NAME_None},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
        if(Mode==TEXT("order_new"))
        {
            Steps.RemoveAt(2,2); // Acquire both parts before the first radio inspection.
            Steps[4].Argument=TEXT("collected_uninspected");
            Steps.Insert({TEXT("use"),TEXT("world.test.radio")},12);
            Steps.Insert({TEXT("check"),TEXT("inspected")},13);
        }
        if(Mode==TEXT("transcript_stale"))
        {
            Steps.Insert({TEXT("save_probe"),TEXT("transcript_load")},18);
            Steps.Insert({TEXT("save_probe"),TEXT("transcript_old_callback")},19);
            Steps.Insert({TEXT("save_probe"),TEXT("transcript_old_token")},20);
            Steps.Insert({TEXT("use"),TEXT("world.test.radio")},21);
            Steps.Insert({TEXT("check"),TEXT("unacknowledged")},22);
        }
        if(Mode==TEXT("audio"))
        {
            Steps.Insert({TEXT("audio_wait"),NAME_None},18);
            Steps.Insert({TEXT("check"),TEXT("unacknowledged")},19);
        }
    }
    else if(Mode==TEXT("ambience"))
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},
            {TEXT("ambience_wait"),NAME_None},{TEXT("pause"),NAME_None},
            {TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("controls"))
    {
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},
            {TEXT("pause"),NAME_None},{TEXT("click"),TEXT("options")},{TEXT("click"),TEXT("option_defaults")},
            {TEXT("click"),TEXT("option_session")},{TEXT("click"),TEXT("back")},{TEXT("click"),TEXT("back")},
            {TEXT("controls"),TEXT("ready")},{TEXT("controls"),TEXT("mouse_base")},
            {TEXT("controls"),TEXT("stick")},{TEXT("controls"),TEXT("walk")},{TEXT("controls"),TEXT("sprint_hold")},
            {TEXT("pause"),NAME_None},{TEXT("controls"),TEXT("modal_held")},{TEXT("click"),TEXT("back")},
            {TEXT("controls"),TEXT("held_after_modal")},{TEXT("controls"),TEXT("rearm")},
            {TEXT("pause"),NAME_None},{TEXT("click"),TEXT("options")}};
        for(int32 Field=0;Field<6;++Field)
        {
            if(Field)Steps.Add({TEXT("click"),TEXT("option_next")});
            const int32 Count=Field==3?5:1;
            for(int32 I=0;I<Count;++I)Steps.Add({TEXT("click"),TEXT("option_increase")});
        }
        Steps.Append({{TEXT("click"),TEXT("option_session")},{TEXT("controls"),TEXT("preferences")},
            {TEXT("click"),TEXT("back")},{TEXT("click"),TEXT("back")},{TEXT("controls"),TEXT("mouse_scaled")},
            {TEXT("controls"),TEXT("toggle")},{TEXT("controls"),TEXT("gamepad_walk")},
            {TEXT("controls"),TEXT("finish")},{TEXT("pause"),NAME_None},{TEXT("click"),TEXT("quit")},
            {TEXT("click"),TEXT("exit_without_save")}});
    }
    else if(Mode==TEXT("display"))
        Steps={{TEXT("click"),TEXT("display_options")},{TEXT("display"),TEXT("capture")},
            {TEXT("click"),TEXT("display_next")},{TEXT("display"),TEXT("draft")},
            {TEXT("click"),TEXT("display_test")},{TEXT("display"),TEXT("settled")},
            {TEXT("click"),TEXT("display_revert")},{TEXT("display"),TEXT("restored")},
            {TEXT("click"),TEXT("display_test")},{TEXT("display"),TEXT("settled")},
            {TEXT("click"),TEXT("display_keep")},{TEXT("display"),TEXT("session_keep")},
            {TEXT("click"),TEXT("display_next")},{TEXT("click"),TEXT("display_test")},
            {TEXT("display"),TEXT("timeout")},{TEXT("display"),TEXT("restored")},
            {TEXT("click"),TEXT("display_test")},{TEXT("display"),TEXT("settled")},
            {TEXT("click"),TEXT("display_save")},{TEXT("display"),TEXT("saved")},
            {TEXT("click"),TEXT("back")},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("display_reload"))
        Steps={{TEXT("display"),TEXT("reload")},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("continue") || Mode==TEXT("order_continue"))
        Steps={{TEXT("click"),TEXT("continue")},{TEXT("check"),TEXT("loaded")},{TEXT("finish"),NAME_None},
            {TEXT("pause"),NAME_None},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("missing_provider")){}
    else if(!ConfigureSaveSteps() && !ConfigureCapacitySteps() && !ConfigureOptionsSteps() && !ConfigureLiveAudioSteps())Finished=true;
    if(Mode==TEXT("gamepad_ui"))PrependGamepadOptions();
    Deadline=FPlatformTime::Seconds()+30;
}
void FCoastalCampaignProbe::Tick(UCoastalHostSession* Host)
{
    if(Finished)return;
    if(Mode==TEXT("missing_provider")){TickMissingProvider(Host);return;}
    if(!Host->Started)return;
    auto* PC=Cast<APlayerController>(Host->GetOwner()); auto* Character=PC?Cast<ACharacter>(PC->GetPawn()):nullptr;
    auto* UI=PC?PC->FindComponentByClass<UCoastalUISessionComponent>():nullptr;
    auto* Relay=PC?PC->FindComponentByClass<UCoastalInteractionRelayComponent>():nullptr;
    auto* Bridge=Character?Character->FindComponentByClass<UCoastalInteractionBridge>():nullptr;
    auto* Saves=Bridge?Bridge->GetCoordinator():nullptr;
    auto* Provider=Character?Character->FindComponentByClass<UCoastalAGISAdapter>():nullptr;
    if(!UI||!Relay||!Saves||!Provider||!Character)return;
    const double Now=FPlatformTime::Seconds();
    auto Fail=[&](const FString& Message)
    {
        Finished=true; Key(PC,EKeys::E,false); Key(PC,EKeys::Escape,false);
        ClearSaveFault();
        if(FParse::Param(FCommandLine::Get(),TEXT("CoastalRecordAudio")))
            UAudioMixerBlueprintLibrary::StopRecordingOutput(Host,EAudioRecordingExportType::WavFile,Slot+TEXT("-failure-output"),
                FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"))));
        UE_LOG(LogTemp,Error,TEXT("COASTAL_ACCEPTANCE_FAILED step=%d %s; save=%s relay=%s"),Step,*Message,*Saves->LastDetail,*Relay->LastDetail);
        FFileHelper::SaveStringToFile(Message,*EvidencePath(Slot,TEXT("-failure.txt")));
        FPlatformMisc::RequestExitWithStatus(false,1);
    };
    if(Now>Deadline){Fail(TEXT("Timed out waiting for ")+Steps[Step].Verb+TEXT(" ")+Steps[Step].Argument.ToString());return;}
    if(Now<Next)return;
    if(Step==0 && Phase==0)
    {
        if(FParse::Param(FCommandLine::Get(),TEXT("CoastalRecordAudio")))
        {
            UE_LOG(LogTemp,Display,TEXT("COASTAL_AUDIO_OBSERVE volume=%f unfocused=%f foreground=%d"),
                FApp::GetVolumeMultiplier(),FApp::GetUnfocusedVolumeMultiplier(),
                GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport && GEngine->GameViewport->Viewport->IsForegroundWindow());
            UAudioMixerBlueprintLibrary::StartRecordingOutput(Host,60);
        }
        UI->SetSaveSetText(FText::FromString(Slot));
        for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
            if(It->GetWorld()==Host->GetWorld() && It->IsInViewport() && It->WidgetTree)
                It->WidgetTree->ForEachWidget([&](UWidget* W)
                {if(auto* Field=Cast<UEditableTextBox>(W))Field->SetText(FText::FromString(Slot));});
        Screenshot(Slot,TEXT("startup"));Phase=1;Next=Now+1;return;
    }
    const auto& S=Steps[Step]; bool Done=false;
    if(Phase==0 && FParse::Param(FCommandLine::Get(),TEXT("CoastalRecordAudio")))
    {
        const auto Context=UI->GetAudioPlaybackContext();
        const auto Device=Host->GetWorld()->GetAudioDevice();
        if(Device.IsValid())
        {
            UE_LOG(LogTemp,Display,TEXT("COASTAL_AUDIO_DEVICE primary=%f muted=%d"),Device->GetPrimaryVolume(),Device->IsAudioDeviceMuted());
        }
        UE_LOG(LogTemp,Display,TEXT("COASTAL_AUDIO_CONTEXT step=%d active=%d ambience=%d interrupted=%d volume=%f"),
            Step,Context.active,Context.ambienceAllowed,Context.interrupted,FApp::GetVolumeMultiplier());
        for(TObjectIterator<UAudioComponent> It;It;++It)if(It->GetWorld()==Host->GetWorld() && It->Sound)
            UE_LOG(LogTemp,Display,TEXT("COASTAL_AUDIO_VOICE sound=%s playing=%d volume=%f class=%s class_volume=%f"),
                *It->Sound->GetPathName(),It->IsPlaying(),It->VolumeMultiplier,
                *GetNameSafe(It->SoundClassOverride),It->SoundClassOverride?It->SoundClassOverride->Properties.Volume:-1.0f);
    }
    if(S.Verb==TEXT("click"))Done=(Mode==TEXT("gamepad_ui") || Mode.StartsWith(TEXT("capacity_")))?NavigateGamepad(Host,S.Argument):Click(Host->GetWorld(),S.Argument);
    else if(S.Verb==TEXT("ambience_wait"))
    {
        FCoastalInventorySnapshot Snapshot;
        if(Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready){Fail(TEXT("Cannot read inventory during ambience interval"));return;}
        if(!Phase)
        {
            BeforeReturn=Snapshot.Payload;BeforeSave=Saves->GetGeneration();
            Phase=1;Deadline=Now+25;Next=Now+15;return;
        }
        bool Playing=false;
        for(TObjectIterator<UAudioComponent> It;It;++It)
            if(It->GetWorld()==Host->GetWorld() && It->Sound && It->Sound->GetName()==TEXT("SW_M1Ambience"))Playing|=It->IsPlaying();
        if(!Playing || Snapshot.Payload!=BeforeReturn || Saves->GetGeneration()!=BeforeSave)
        {Fail(TEXT("Ambience interval lost its source or changed campaign state"));return;}
        if(FParse::Param(FCommandLine::Get(),TEXT("CoastalRecordAudio")))
            UAudioMixerBlueprintLibrary::StopRecordingOutput(Host,EAudioRecordingExportType::WavFile,Slot+TEXT("-ambience-output"),
                FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"))));
        UE_LOG(LogTemp,Display,TEXT("COASTAL_AMBIENCE_INTERVAL_PASS 15 seconds, source playing, inventory and generation unchanged; inspect mixer output"));Done=true;
    }
    else if(S.Verb==TEXT("audio_wait"))
    {
        if(!Phase)
        {
            FCoastalInventorySnapshot Snapshot;
            if(Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready){Fail(TEXT("Cannot capture actual inventory before radio wait"));return;}
            BeforeReturn=Snapshot.Payload;BeforeSave=Saves->GetGeneration();
            Phase=1;Deadline=Now+30;Next=Now+21;return;
        }
        bool Found=false,Playing=false;
        for(TObjectIterator<UAudioComponent> It;It;++It)
            if(It->GetWorld()==Host->GetWorld() && It->Sound && It->Sound->GetName()==TEXT("SW_M1Radio"))
            {Found=true;Playing|=It->IsPlaying();}
        FCoastalInventorySnapshot Snapshot;
        if(!Found || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready || Snapshot.Payload!=BeforeReturn || Saves->GetGeneration()!=BeforeSave)
        {Fail(TEXT("Radio wait changed campaign/inventory or lost its owned source"));return;}
        // A paused world's component state can outlive its audible samples.
        // Preserve real output for duration/no-replay analysis; do not retry,
        // auto-acknowledge, or infer acoustic completion from IsPlaying().
        if(FParse::Param(FCommandLine::Get(),TEXT("CoastalRecordAudio")))
        {
            UAudioMixerBlueprintLibrary::StopRecordingOutput(Host,EAudioRecordingExportType::WavFile,Slot+TEXT("-pre-ack-output"),
                FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"))));
            UAudioMixerBlueprintLibrary::StartRecordingOutput(Host,30);
        }
        UE_LOG(LogTemp,Display,TEXT("COASTAL_RADIO_WAIT_CONSERVED_STATE component_playing=%d; verify recorded output separately"),Playing);Done=true;
    }
    else if(S.Verb==TEXT("use"))
    {
        auto* Object=Target(Host->GetWorld(),S.Argument);if(!Object){Fail(TEXT("Missing authored object"));return;}
        if(Phase==0)
        {
            FVector Position=Object->GetActorLocation()+FVector(-145,0,0);Position.Z=100;
            Character->GetCharacterMovement()->StopMovementImmediately();
            if(!Character->TeleportTo(Position,FRotator::ZeroRotator,false,false)){Fail(TEXT("Test positioning failed"));return;}
            PC->SetControlRotation((Object->GetActorLocation()-Position).Rotation());
            SampleStarted=Now;Phase=1;Next=Now+0.5;return;
        }
        if(Phase==1)
        {
            PC->SetControlRotation((Object->GetActorLocation()-PC->PlayerCameraManager->GetCameraLocation()).Rotation());
            const auto Offer=Relay->GetCurrentOffer();
            if(Offer.WorldId!=S.Argument || !Offer.bCanInteract)
            {
                if(Now-SampleStarted>2)
                {
                    FHitResult Hit;FCollisionQueryParams Params;Params.AddIgnoredActor(Character);
                    Host->GetWorld()->LineTraceSingleByChannel(Hit,PC->PlayerCameraManager->GetCameraLocation(),Object->GetActorLocation(),ECC_GameTraceChannel1,Params);
                    FHitResult RawHit;
                    Host->GetWorld()->LineTraceSingleByChannel(RawHit,PC->PlayerCameraManager->GetCameraLocation(),Object->GetActorLocation(),ECC_GameTraceChannel1);
                    const auto Preview=Bridge->PreviewInteraction(Object);
                    UE_LOG(LogTemp,Display,TEXT("COASTAL_FOCUS_DIAGNOSTIC raw_hit=%s direct_visible=%d direct_allowed=%d direct_detail=%s"),
                        *GetNameSafe(RawHit.GetActor()),Preview.bVisible,Preview.bCanInteract,*Preview.Detail.ToString());
                    UE_LOG(LogTemp,Display,TEXT("COASTAL_FOCUS_WAIT target=%s offer=%s allowed=%d detail=%s response=%d camera=%s pawn=%s object=%s diagnostic_hit=%s"),
                        *S.Argument.ToString(),*Offer.WorldId.ToString(),Offer.bCanInteract,*Offer.Detail.ToString(),
                        static_cast<int32>(Object->ProxyMesh->GetCollisionResponseToChannel(ECC_GameTraceChannel1)),
                        *PC->PlayerCameraManager->GetCameraLocation().ToString(),*Character->GetActorLocation().ToString(),
                        *Object->GetActorLocation().ToString(),*GetNameSafe(Hit.GetActor()));
                    Screenshot(Slot,FString::Printf(TEXT("waiting-focus-%d"),Step));SampleStarted=Now;
                }
                return;
            }
            if(Mode.StartsWith(TEXT("capacity_")) && Steps.IsValidIndex(Step+1) && Steps[Step+1].Argument==TEXT("refused"))
            {
                if(Provider->ExportInventory(CapacityBefore)!=ECoastalProviderResult::Ready){Fail(TEXT("Cannot capture capacity state before input"));return;}
                CapacityGeneration=Saves->GetGeneration();
            }
            Screenshot(Slot,FString::Printf(TEXT("focus-%d"),Step));
            Key(PC,EKeys::E,true);Phase=2;Next=Now+0.3;return;
        }
        Key(PC,EKeys::E,false);Done=true;
    }
    else if(S.Verb==TEXT("pause"))
    {
        if(Phase==0){Key(PC,EKeys::Escape,true);Phase=1;Next=Now+0.2;return;}
        Key(PC,EKeys::Escape,false);Done=UI->HasModal();
    }
    else if(S.Verb==TEXT("hazard"))
    {
        if(Phase==0)
        {
            FCoastalInventorySnapshot Snapshot;Provider->ExportInventory(Snapshot);BeforeReturn=Snapshot.Payload;
            Character->GetCharacterMovement()->StopMovementImmediately();
            if(!Character->TeleportTo(FVector(1300,-700,100),FRotator::ZeroRotator,false,false)){Fail(TEXT("Hazard positioning failed"));return;}
            Phase=1;Next=Now+2;return;
        }
        auto* Recovery=Character->FindComponentByClass<UCoastalPlayerRecoveryComponent>();
        if(!Recovery || Recovery->IsReturning())return;
        FCoastalInventorySnapshot Snapshot;Provider->ExportInventory(Snapshot);
        if(FVector::Dist(Character->GetActorLocation(),Saves->GetDryCheckpoint().GetLocation())>10 || Snapshot.Payload!=BeforeReturn)
        {Fail(TEXT("Safety return or inventory conservation failed"));return;}
        Screenshot(Slot,TEXT("returned"));Done=true;
    }
    else if(S.Verb==TEXT("capacity"))
    {
        FString Error;Done=RunCapacityStep(Host,S.Argument,Error);
        if(!Error.IsEmpty()){Fail(Error);return;}
    }
    else if(S.Verb==TEXT("save_fault"))
    {
        FString Error;Done=RunSaveFaultStep(Host,S.Argument,Error);
        if(!Error.IsEmpty()){Fail(Error);return;}
    }
    else if(S.Verb==TEXT("save_probe"))
    {
        FString Error;Done=RunSaveStep(Host,S.Argument,Error);
        if(!Error.IsEmpty()){Fail(Error);return;}
    }
    else if(S.Verb==TEXT("display"))
    {
        FString Error; Done=RunDisplayStep(Host,S.Argument,Error);
        if(!Error.IsEmpty()){Fail(Error);return;}
    }
    else if(S.Verb==TEXT("controls"))
    {
        FString Error; Done=RunControlsStep(Host,S.Argument,Error);
        if(!Error.IsEmpty()){Fail(Error);return;}
    }
    else if(S.Verb==TEXT("options_probe"))
    {
        FString Error;Done=RunOptionsStep(Host,S.Argument,Error);
        if(!Error.IsEmpty()){Fail(Error);return;}
    }
    else if(S.Verb==TEXT("live_audio"))
    {
        FString Error;Done=RunLiveAudioStep(Host,S.Argument,Error);
        if(!Error.IsEmpty()){Fail(Error);return;}
    }
    else if(S.Verb==TEXT("check"))
    {
        const auto Mission=Saves->GetMission()->ExportSnapshot();
        FCoastalContainerView Backpack,Storage;Provider->ReadContainerView(TEXT("container.player"),Backpack);Provider->ReadContainerView(TEXT("world.test.storage"),Storage);
        if(S.Argument==TEXT("new"))Done=Saves->HasActiveCampaign() && !UI->HasModal() && Backpack.Items.IsEmpty() && Storage.Items.IsEmpty();
        if(S.Argument==TEXT("inspected"))Done=Mission.bRadioInspected;
        if(S.Argument==TEXT("collected") || S.Argument==TEXT("collected_uninspected"))Done=Backpack.Items.Num()==2 && Storage.Items.IsEmpty() && Target(Host->GetWorld(),TEXT("world.test.battery"))->IsActive() && Target(Host->GetWorld(),TEXT("world.test.fuse"))->IsActive();
        if(S.Argument==TEXT("collected_uninspected"))
        {
            Done &= !Mission.bRadioInspected && !Mission.bRadioRepaired && !Mission.bMessageHeard
                && !Mission.bCabinVisited && !Mission.bDockDiscovered && !Mission.bMaintenanceNoteRead;
            if(Done)UE_LOG(LogTemp,Display,TEXT("COASTAL_ORDER_PARTS_FIRST_PASS carried_parts=2 inspected=0 optional_facts=0"));
        }
        if(S.Argument==TEXT("stored"))Done=Backpack.Items.Num()==1 && Storage.Items.Num()==1;
        if(S.Argument==TEXT("repaired"))Done=Mission.bRadioRepaired && !Mission.bMessageHeard && Backpack.Items.IsEmpty() && Storage.Items.IsEmpty();
        if(S.Argument==TEXT("unacknowledged"))Done=UI->HasModal() && !Mission.bMessageHeard;
        if(S.Argument==TEXT("complete"))Done=Mission.bRadioRepaired && Mission.bMessageHeard && !UI->HasModal();
        if(S.Argument==TEXT("saved"))Done=Saves->GetGeneration()>BeforeSave && !Saves->IsBusy() && !Saves->IsRecoveryRequired();
        if(S.Argument==TEXT("loaded"))
        {
            FString Previous;
            if(!FFileHelper::LoadFileToString(Previous,*EvidencePath(Slot,TEXT("-campaign.txt"))) || !FGuid::Parse(Previous,ExpectedCampaign)){Fail(TEXT("Missing expected prior campaign"));return;}
            Done=Saves->HasActiveCampaign() && Saves->GetCampaignId()==ExpectedCampaign && Mission.bRadioRepaired && Mission.bMessageHeard
                && Backpack.Items.IsEmpty() && Storage.Items.IsEmpty() && !UI->HasModal()
                && Target(Host->GetWorld(),TEXT("world.test.battery"))->IsActive() && Target(Host->GetWorld(),TEXT("world.test.fuse"))->IsActive();
        }
        if(!Done){Fail(TEXT("State check failed: ")+S.Argument.ToString());return;}
    }
    else if(S.Verb==TEXT("finish"))
    {
        FCoastalInventorySnapshot Snapshot; if(Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready || Snapshot.Receipts.Num()!=5)
        {Fail(TEXT("Final receipt ledger invalid"));return;}
        if(Mode==TEXT("new") || Mode==TEXT("audio") || Mode==TEXT("gamepad_ui") || Mode==TEXT("transcript_stale") || Mode==TEXT("order_new"))
            if(!FFileHelper::SaveStringToFile(Saves->GetCampaignId().ToString(),*EvidencePath(Slot,TEXT("-campaign.txt"))))
            {Fail(TEXT("Cannot record expected campaign for fresh-process verification"));return;}
        if(Mode==TEXT("order_new") || Mode==TEXT("order_continue"))
        {
            const auto Mission=Saves->GetMission()->ExportSnapshot();const auto Journal=Saves->GetJournal();
            if(Mission.bCabinVisited || Mission.bDockDiscovered || Mission.bMaintenanceNoteRead
                || !Mission.bRadioRepaired || !Mission.bMessageHeard || Journal.Num()!=2
                || !Journal.Contains(TEXT("journal.first_signal.transmission")) || !Journal.Contains(TEXT("journal.north_reach.lead"))
                || !Target(Host->GetWorld(),TEXT("world.test.battery"))->IsActive() || !Target(Host->GetWorld(),TEXT("world.test.fuse"))->IsActive())
            {Fail(TEXT("Out-of-order campaign lost journal/world state or required optional facts"));return;}
            UE_LOG(LogTemp,Display,TEXT("COASTAL_ORDER_COMPLETE_PASS mode=%s optional_facts=0 journal_entries=2 pickups_consumed=2 receipts=5"),*Mode);
        }
        const FString Result=FString::Printf(TEXT("mode=%s campaign=%s generation=%lld receipts=%d repaired=true acknowledged=true\n"),*Mode,*Saves->GetCampaignId().ToString(),Saves->GetGeneration(),Snapshot.Receipts.Num());
        FFileHelper::SaveStringToFile(Result,*EvidencePath(Slot,TEXT("-")+Mode+TEXT("-pass.txt")));
        UE_LOG(LogTemp,Display,TEXT("COASTAL_ACCEPTANCE_PASS %s"),*Result);Screenshot(Slot,Mode+TEXT("-complete"));Done=true;
        if(FParse::Param(FCommandLine::Get(),TEXT("CoastalRecordAudio")))
            UAudioMixerBlueprintLibrary::StopRecordingOutput(Host,EAudioRecordingExportType::WavFile,Slot+TEXT("-output"),
                FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"))));
    }
    if(Done)
    {
        UE_LOG(LogTemp,Display,TEXT("COASTAL_ACCEPTANCE_STEP %d %s %s"),Step,*S.Verb,*S.Argument.ToString());
        ++Step;Phase=0;Next=Now+0.5;Deadline=Now+15;
        if(Step>=Steps.Num())Finished=true;
        else if(Steps[Step].Verb==TEXT("click") && Steps[Step].Argument==TEXT("save"))BeforeSave=Saves->GetGeneration();
    }
}
#else
FCoastalCampaignProbe::FCoastalCampaignProbe(const FString& InMode) { Finished=true; }
void FCoastalCampaignProbe::Tick(UCoastalHostSession*) {}
#endif
