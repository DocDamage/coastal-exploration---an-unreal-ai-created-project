#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalLocalOptions.h"
#include "CoastalUISessionComponent.h"
#include "CoastalAudioOptionsComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "CoastalPanelWidget.h"
#include "Blueprint/WidgetTree.h"
namespace
{
coastal::PlayerOptions ExpectedOptions()
{ return {125,125,90,150,true,true,95,90,85,80}; }
FString OptionsSlot(int32 I)
{ return I==0?TEXT("CoastalLocalOptions_v1_A"):TEXT("CoastalLocalOptions_v1_B"); }
}
bool FCoastalCampaignProbe::ConfigureOptionsSteps()
{
    if(Mode==TEXT("options_ambiguous") || Mode==TEXT("options_ambiguous_reload"))
    {
        Steps={{TEXT("click"),TEXT("options")},{TEXT("options_probe"),TEXT("capture")}};
        if(Mode==TEXT("options_ambiguous"))
            Steps.Append({{TEXT("click"),TEXT("option_defaults")},{TEXT("options_probe"),TEXT("draft")},
                {TEXT("click"),TEXT("option_save")},{TEXT("options_probe"),TEXT("ambiguous")}});
        else Steps.Add({TEXT("options_probe"),TEXT("ambiguous_reload")});
        Steps.Append({{TEXT("click"),TEXT("back")},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}});return true;
    }
    if(Mode==TEXT("options_legacy1") || Mode==TEXT("options_legacy2"))
    {
        Steps={{TEXT("click"),TEXT("options")},{TEXT("options_probe"),TEXT("capture")},
            {TEXT("options_probe"),TEXT("legacy_loaded")}};
        for(int32 I=0;I<6;++I)Steps.Add({TEXT("click"),TEXT("option_next")});
        Steps.Append({{TEXT("click"),TEXT("option_decrease")},{TEXT("options_probe"),TEXT("draft")},
            {TEXT("click"),TEXT("option_save")},{TEXT("options_probe"),TEXT("legacy_saved")},
            {TEXT("click"),TEXT("back")},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}});
        return true;
    }
    if(Mode==TEXT("options_write_failure") || Mode==TEXT("options_external_change"))
    {
        Steps={{TEXT("click"),TEXT("options")},{TEXT("options_probe"),TEXT("capture")},
            {TEXT("click"),TEXT("option_defaults")},{TEXT("options_probe"),TEXT("draft")},
            {TEXT("click"),TEXT("option_save")},{TEXT("options_probe"),TEXT("write_failed")},
            {TEXT("click"),TEXT("option_session")},{TEXT("options_probe"),TEXT("session")},
            {TEXT("click"),TEXT("back")},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
        if(Mode==TEXT("options_external_change"))
        {
            Steps[5].Argument=TEXT("external_refused");
            Steps.Insert({TEXT("options_probe"),TEXT("await_external")},4);
        }
        return true;
    }
    if(Mode==TEXT("options_recovered") || Mode==TEXT("options_blocked") || Mode==TEXT("options_read_error"))
    {
        Steps={{TEXT("click"),TEXT("options")},{TEXT("options_probe"),TEXT("capture")},
            {TEXT("options_probe"),Mode==TEXT("options_recovered")?TEXT("recovered"):TEXT("blocked")},
            {TEXT("click"),TEXT("back")},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
        if(Mode==TEXT("options_read_error") || Mode==TEXT("options_blocked"))
        {
            Steps.Insert({TEXT("click"),TEXT("option_increase")},3);
            Steps.Insert({TEXT("options_probe"),TEXT("draft")},4);
            Steps.Insert({TEXT("click"),TEXT("option_session")},5);
            Steps.Insert({TEXT("options_probe"),TEXT("blocked_session")},6);
        }
        return true;
    }
    if(Mode!=TEXT("options_save") && Mode!=TEXT("options_reload") && Mode!=TEXT("options_session"))return false;
    Steps={{TEXT("click"),TEXT("options")},{TEXT("options_probe"),TEXT("capture")}};
    if(Mode==TEXT("options_save"))
    {
        Steps.Add({TEXT("click"),TEXT("option_defaults")});
        for(int32 Field=0;Field<10;++Field)
        {
            if(Field)Steps.Add({TEXT("click"),TEXT("option_next")});
            const int32 Count=Field==3?5:(Field>=6?Field-5:1);
            for(int32 I=0;I<Count;++I)Steps.Add({TEXT("click"),Field>=6?TEXT("option_decrease"):TEXT("option_increase")});
        }
        Steps.Append({{TEXT("options_probe"),TEXT("draft")},{TEXT("click"),TEXT("option_save")},
            {TEXT("options_probe"),TEXT("saved")}});
    }
    else
    {
        Steps.Add({TEXT("options_probe"),TEXT("loaded")});
        if(Mode==TEXT("options_session"))
            Steps.Append({{TEXT("click"),TEXT("option_defaults")},{TEXT("options_probe"),TEXT("draft")},
                {TEXT("click"),TEXT("option_session")},{TEXT("options_probe"),TEXT("session")}});
    }
    Steps.Append({{TEXT("click"),TEXT("back")},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}});
    return true;
}
bool FCoastalCampaignProbe::RunOptionsStep(UCoastalHostSession* Host,FName Kind,FString& Error)
{
    auto* PC=Cast<APlayerController>(Host->GetOwner());auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();
    UCoastalLocalOptions* Profile=nullptr;
    for(TObjectIterator<UCoastalLocalOptions> It;It;++It)if(It->GetOuter()==UI)Profile=*It;
    auto Check=[&](bool Good,const TCHAR* Why){if(!Good)Error=Why;return Good;};
    if(!Check(Profile && Profile->IsInitialized(),TEXT("Missing initialized preference owner")))return false;
    auto* Camera=PC->GetPawn()->FindComponentByClass<UCameraComponent>();
    auto* Audio=PC->FindComponentByClass<UCoastalAudioOptionsComponent>();
    if(!Check(Camera && Audio && Audio->IsAudioReady(),TEXT("Actual camera/audio preference consumers unavailable")))return false;
    TArray<TArray<uint8>> Disk;
    coastal::OptionsSlot Records[2];
    for(int32 I=0;I<2;++I)
    {
        TArray<uint8> Bytes;
        if(UGameplayStatics::DoesSaveGameExist(OptionsSlot(I),0))
        {
            if(!UGameplayStatics::LoadDataFromSlot(Bytes,OptionsSlot(I),0))
            {
                if(!Check(Mode==TEXT("options_read_error"),TEXT("Preference disk read failed")))return false;
                Records[I].status=coastal::OptionsStatus::ReadError;
            }
            else Records[I].status=coastal::DecodeOptions(Bytes.GetData(),Bytes.Num(),Records[I].record);
        }
        Disk.Add(MoveTemp(Bytes));
    }
    if(Mode==TEXT("options_read_error"))
    {
        const int32 Errors=int32(Records[0].status==coastal::OptionsStatus::ReadError)
            +int32(Records[1].status==coastal::OptionsStatus::ReadError);
        if(!Check(Errors==1,TEXT("Read-error fixture must cause exactly one actual existing-slot read failure")))return false;
    }
    if(Kind==TEXT("blocked_session"))
    {
        auto Expected=coastal::PlayerOptions{};Expected.mousePercent=125;
        if(!Check(Profile->Get()==Expected && !Profile->CanWrite() && Disk==FaultDisk
            && FMath::IsNearlyEqual(Camera->FieldOfView,85.f)
            && Profile->Notice().Contains(TEXT("Disk writes remain blocked")),TEXT("Blocked session-only change failed or authorized writes")))return false;
        if(Mode==TEXT("options_read_error")){UE_LOG(LogTemp,Display,TEXT("COASTAL_OPTIONS_READ_ERROR_PASS session_mouse=125 writes_blocked=1 actual_read_errors=1"));}
        else {UE_LOG(LogTemp,Display,TEXT("COASTAL_OPTIONS_BLOCKED_SESSION_PASS session_mouse=125 writes_blocked=1"));}
        return true;
    }
    const FString Base=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot+TEXT("-options"));
    if(Kind==TEXT("capture"))
    {
        FaultDisk=Disk;
        std::vector<uint8> Encoded;
        if(!Check(coastal::EncodeOptions({Profile->Get(),1},Encoded),TEXT("Invalid live options")))return false;
        BeforeReturn.Reset();BeforeReturn.Append(Encoded.data(),static_cast<int32>(Encoded.size()));
        return true;
    }
    if(Kind==TEXT("draft"))
    {
        coastal::OptionsRecord Before;
        coastal::DecodeOptions(BeforeReturn.GetData(),BeforeReturn.Num(),Before);
        return Check(Disk==FaultDisk && Profile->Get()==Before.values && FMath::IsNearlyEqual(Camera->FieldOfView,static_cast<float>(Before.values.fieldOfView)),TEXT("Unapplied draft changed disk/live preferences or camera"));
    }
    if(Kind==TEXT("await_external"))
    {
        if(!Phase){UE_LOG(LogTemp,Display,TEXT("COASTAL_OPTIONS_AWAIT_EXTERNAL_CHANGE"));Phase=1;}
        if(Disk==FaultDisk){Next=FPlatformTime::Seconds()+0.25;return false;}
        FaultDisk=Disk;return true;
    }
    if(Kind==TEXT("session"))
    {
        if(!Check(Disk==FaultDisk && Profile->Get()==coastal::PlayerOptions{} && FMath::IsNearlyEqual(Camera->FieldOfView,85.f)
            && ((Mode!=TEXT("options_write_failure") && Mode!=TEXT("options_external_change")) || !Profile->CanWrite()),TEXT("Session defaults changed disk or failed to apply")))return false;
        UE_LOG(LogTemp,Display,TEXT("COASTAL_OPTIONS_SESSION_PASS all ten defaults applied; both disk slots unchanged"));return true;
    }
    const auto Selection=coastal::SelectOptions(Records[0],Records[1]);
    if(Kind==TEXT("ambiguous") || Kind==TEXT("ambiguous_reload"))
    {
        coastal::OptionsRecord Before;
        if(!Check(coastal::DecodeOptions(BeforeReturn.GetData(),BeforeReturn.Num(),Before)==coastal::OptionsStatus::Valid,TEXT("Missing captured live values")))return false;
        if(Kind==TEXT("ambiguous_reload"))
        {
            if(!Check(Selection.selected>=0 && Records[Selection.selected].record.values==Profile->Get()
                && Disk==FaultDisk && Profile->CanWrite(),TEXT("Relaunch failed to select valid disk result")))return false;
            UE_LOG(LogTemp,Display,TEXT("COASTAL_OPTIONS_AMBIGUOUS_RELOAD_PASS schema=%u generation=%llu master=%d"),
                Records[Selection.selected].record.sourceSchema,Records[Selection.selected].record.generation,Profile->Get().masterPercent);return true;
        }
        coastal::OptionsSlot Prior[2];
        for(int32 I=0;I<2;++I)Prior[I].status=coastal::DecodeOptions(FaultDisk[I].GetData(),FaultDisk[I].Num(),Prior[I].record);
        const auto Previous=coastal::SelectOptions(Prior[0],Prior[1]);
        bool SaveFound=false,SaveEnabled=false;
        for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
            if(It->GetWorld()==Host->GetWorld() && It->IsInViewport() && It->HasBeenPresented() && It->WidgetTree)
                It->WidgetTree->ForEachWidget([&](UWidget* W){if(auto* B=Cast<UCoastalCommandButton>(W))
                    if(B->Command==TEXT("option_save")){SaveFound=true;SaveEnabled=B->GetIsEnabled();}});
        if(!Check(Previous.selected>=0 && Disk[Previous.selected]==FaultDisk[Previous.selected]
            && Profile->Get()==Before.values && !Profile->CanWrite() && SaveFound && !SaveEnabled
            && FMath::IsNearlyEqual(Camera->FieldOfView,float(Before.values.fieldOfView))
            && Profile->Notice().Contains(TEXT("write could not be verified")),TEXT("Unverified write changed live settings/prior slot or allowed saving")))return false;
        if(!Check(!Profile->SaveAndApply(coastal::PlayerOptions{}) && Profile->Get()==Before.values,TEXT("Second write unexpectedly accepted")))return false;
        for(int32 I=0;I<2;++I)
        {
            TArray<uint8> Now;
            if(!Check(UGameplayStatics::LoadDataFromSlot(Now,OptionsSlot(I),0) && Now==Disk[I],TEXT("Blocked retry changed disk")))return false;
        }
        UE_LOG(LogTemp,Display,TEXT("COASTAL_OPTIONS_AMBIGUOUS_PASS previous_live=1 previous_slot=1 save_disabled=1 retry_blocked=1"));return true;
    }
    if(Kind==TEXT("legacy_loaded") || Kind==TEXT("legacy_saved"))
    {
        const bool Saved=Kind==TEXT("legacy_saved");
        const uint32 Schema=Mode==TEXT("options_legacy1")?1:2;
        coastal::OptionsSlot Prior[2];
        for(int32 I=0;I<2;++I)
        {
            TArray<uint8> Original;
            if(!Check(FFileHelper::LoadFileToArray(Original,*(Base+FString::FromInt(I)+TEXT(".bin")))
                && FaultDisk[I]==Original,TEXT("Legacy load rewrote fixture bytes")))return false;
            Prior[I].status=coastal::DecodeOptions(Original.GetData(),Original.Num(),Prior[I].record);
        }
        const auto Before=coastal::SelectOptions(Prior[0],Prior[1]);
        if(!Check(Before.selected>=0 && Before.target>=0 && Selection.selected>=0,TEXT("Legacy pair selection failed")))return false;
        auto Expected=coastal::PlayerOptions{125,125,90,150,true,Schema==2};
        if(Saved)Expected.masterPercent=95;
        const auto& Current=Records[Selection.selected].record;
        if(!Check(Prior[Before.selected].record.sourceSchema==Schema && Profile->Get()==Expected
            && Current.values==Expected && Current.sourceSchema==(Saved?3:Schema)
            && Current.generation==(Saved?Before.nextGeneration:Prior[Before.selected].record.generation)
            && Profile->CanWrite() && FMath::IsNearlyEqual(Camera->FieldOfView,90.f) && UI->FontSize(24)==36,
            TEXT("Legacy values/defaults, consumers, schema or generation mismatch")))return false;
        if(Saved)
        {
            if(!Check(Selection.selected==Before.target && Disk[Before.selected]==FaultDisk[Before.selected]
                && Profile->Notice().Contains(TEXT("read back and verified")),TEXT("Upgrade did not preserve prior active slot or verify inactive write")))return false;
        }
        else if(!Check(Disk==FaultDisk,TEXT("Legacy initialization wrote options")))return false;
        UE_LOG(LogTemp,Display,TEXT("COASTAL_OPTIONS_LEGACY_PASS schema=%u step=%s generation=%llu"),Schema,*Kind.ToString(),Current.generation);
        return true;
    }
    if(Kind==TEXT("recovered") || Kind==TEXT("blocked") || Kind==TEXT("write_failed") || Kind==TEXT("external_refused"))
    {
        const bool Blocked=Kind==TEXT("blocked");
        const bool External=Kind==TEXT("external_refused");
        const bool WriteFailed=Kind==TEXT("write_failed") || External;
        bool SaveFound=false,SaveEnabled=false;
        for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
            if(It->GetWorld()==Host->GetWorld() && It->IsInViewport() && It->HasBeenPresented() && It->WidgetTree)
                It->WidgetTree->ForEachWidget([&](UWidget* W){if(auto* B=Cast<UCoastalCommandButton>(W))
                    if(B->Command==TEXT("option_save")){SaveFound=true;SaveEnabled=B->GetIsEnabled();}});
        const auto Expected=Blocked?coastal::PlayerOptions{}:ExpectedOptions();
        if(!Check(Disk==FaultDisk && Profile->Get()==Expected && Profile->CanWrite()==!(Blocked || WriteFailed)
            && SaveFound && SaveEnabled==!(Blocked || WriteFailed) && FMath::IsNearlyEqual(Camera->FieldOfView,static_cast<float>(Expected.fieldOfView))
            && Profile->Notice().Contains(External?TEXT("files changed or became unreadable"):WriteFailed?TEXT("write could not be verified"):Blocked?TEXT("disk writes blocked"):TEXT("recovered from the remaining valid slot")),TEXT("Preference recovery/refusal state or native Save availability incorrect")))return false;
        UE_LOG(LogTemp,Display,TEXT("COASTAL_OPTIONS_FAULT_PASS %s notice=%s"),*Kind.ToString(),*Profile->Notice());return true;
    }
    if(!Check(Selection.selected>=0,TEXT("No selectable persisted preference record")))return false;
    const auto& Record=Records[Selection.selected].record;
    if(!Check(Record.values==ExpectedOptions() && Profile->Get()==ExpectedOptions() && Record.sourceSchema==3
        && FMath::IsNearlyEqual(Camera->FieldOfView,90.f) && UI->FontSize(24)==36,TEXT("Ten-field saved/live preference or camera/text mismatch")))return false;
    if(Kind==TEXT("saved"))
    {
        if(!Check(Profile->Notice().Contains(TEXT("read back and verified")),TEXT("UI did not report verified options save")))return false;
        for(int32 I=0;I<2;++I)
            if(!Check(FFileHelper::SaveArrayToFile(Disk[I],*(Base+FString::FromInt(I)+TEXT(".bin"))),TEXT("Cannot record saved preference evidence")))return false;
    }
    else if(Kind==TEXT("loaded"))
    {
        for(int32 I=0;I<2;++I)
        {
            TArray<uint8> Prior;
            if(!Check(FFileHelper::LoadFileToArray(Prior,*(Base+FString::FromInt(I)+TEXT(".bin"))) && Prior==Disk[I],TEXT("Fresh process changed saved preference bytes")))return false;
        }
    }
    else {Error=TEXT("Unknown options step");return false;}
    UE_LOG(LogTemp,Display,TEXT("COASTAL_OPTIONS_PASS %s generation=%llu ten fields; actual FOV=90 text=150; audio owner ready (not acoustic proof)"),*Kind.ToString(),Record.generation);
    return true;
}
#endif
