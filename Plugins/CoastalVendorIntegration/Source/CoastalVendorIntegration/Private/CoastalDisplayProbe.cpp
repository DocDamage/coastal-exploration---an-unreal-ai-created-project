#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalUISessionComponent.h"
#include "CoastalDisplaySettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
namespace
{
coastal::DisplayMode SavedMode()
{
    const auto* S=GEngine->GetGameUserSettings(); const auto Size=S->GetScreenResolution();
    coastal::DisplayWindowMode Mode=coastal::DisplayWindowMode::Windowed;
    if(S->GetFullscreenMode()==EWindowMode::Fullscreen)Mode=coastal::DisplayWindowMode::Fullscreen;
    if(S->GetFullscreenMode()==EWindowMode::WindowedFullscreen)Mode=coastal::DisplayWindowMode::Borderless;
    return {Size.X,Size.Y,Mode};
}
}
bool FCoastalCampaignProbe::RunDisplayStep(UCoastalHostSession* Host,FName Kind,FString& Error)
{
    auto* PC=Cast<APlayerController>(Host->GetOwner());auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();
    UCoastalDisplaySettings* Display=nullptr;
    for(TObjectIterator<UCoastalDisplaySettings> It;It;++It)if(It->GetOuter()==UI)Display=*It;
    if(!Display || !Display->Ready())
    {Error=TEXT("Actual native display owner unavailable: ")+(Display?Display->Status():TEXT("missing"));return false;}
    const double Now=FPlatformTime::Seconds();
    auto Check=[&](bool Good,const TCHAR* Message){if(!Good)Error=Message;return Good;};
    const FString Path=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot+TEXT("-display-expected.txt"));
    if(Kind==TEXT("capture"))
    {
        BeforeDisplay=Display->Current();SavedDisplay=SavedMode();
        int32 Windowed=0;
        for(const auto& M:Display->Modes())
        {
            UE_LOG(LogTemp,Display,TEXT("COASTAL_DISPLAY_CANDIDATE %s"),*UCoastalDisplaySettings::Describe(M));
            if(M.window==BeforeDisplay.window)++Windowed;
        }
        // A small desktop may report only the current windowed resolution.
        // Exercise the real mode selector instead of waiting on a disabled Test.
        if(Windowed<2 || FParse::Param(FCommandLine::Get(),TEXT("CoastalDisplayModeCycle")))
            for(auto& S:Steps)if(S.Verb==TEXT("click") && S.Argument==TEXT("display_next"))S.Argument=TEXT("display_mode");
        UE_LOG(LogTemp,Display,TEXT("COASTAL_DISPLAY_CAPTURE actual=%s candidates=%d"),
            *UCoastalDisplaySettings::Describe(BeforeDisplay),static_cast<int32>(Display->Modes().size()));
        return Check(Display->Modes().size()>1,TEXT("No alternate real reported display mode"));
    }
    if(Kind==TEXT("draft"))return Check(Display->Current()==BeforeDisplay && SavedMode()==SavedDisplay,TEXT("Draft changed runtime or settings"));
    if(Kind==TEXT("settled"))
    {
        if(!Phase || Now>=Next)
        {
            UE_LOG(LogTemp,Display,TEXT("COASTAL_DISPLAY_OBSERVE actual=%s proposed=%s settled=%d status=%s"),
                *UCoastalDisplaySettings::Describe(Display->Current()),*UCoastalDisplaySettings::Describe(Display->Trial().Proposed()),
                Display->Trial().Settled(),*Display->Status());
            Phase=1;Next=Now+1;
        }
        if(Display->Trial().Phase()!=coastal::DisplayPhase::Testing){Error=TEXT("Trial not active: ")+Display->Status();return false;}
        return Display->Trial().Settled() && Display->Current()==Display->Trial().Proposed();
    }
    if(Kind==TEXT("restored"))
    {
        if(Display->Trial().Busy())return false;
        return Check(Display->Current()==BeforeDisplay && SavedMode()==SavedDisplay,TEXT("Revert failed to restore actual prior mode without saving"));
    }
    if(Kind==TEXT("session_keep"))
    {
        if(!Check(!Display->Trial().Busy() && Display->Current()!=BeforeDisplay && SavedMode()==SavedDisplay,TEXT("Session Keep altered saved fields or failed")))return false;
        BeforeDisplay=Display->Current();return true;
    }
    if(Kind==TEXT("timeout"))
    {
        if(!Phase){Phase=1;Deadline=Now+25;Next=Now+16;return false;}
        return Check(Display->Trial().Phase()!=coastal::DisplayPhase::Testing,TEXT("Real-time timeout failed while UI paused"));
    }
    if(Kind==TEXT("saved"))
    {
        if(!Check(!Display->Trial().Busy() && SavedMode()==Display->Current(),TEXT("Explicit Keep did not stage selected engine settings")))return false;
        const auto M=Display->Current();
        const FString Text=FString::Printf(TEXT("%d %d %d"),M.width,M.height,static_cast<int32>(M.window));
        if(!FFileHelper::SaveStringToFile(Text,*Path)){Error=TEXT("Failed to record expected display");return false;}
        UE_LOG(LogTemp,Display,TEXT("COASTAL_DISPLAY_SESSION_PASS draft, revert, session keep, timeout; save requested: %s"),*Text);
        FScreenshotRequest::RequestScreenshot(FPaths::ChangeExtension(Path,TEXT("png")),true,false);return true;
    }
    if(Kind==TEXT("reload"))
    {
        FString Text;TArray<FString> Parts;
        if(!FFileHelper::LoadFileToString(Text,*Path)){Error=TEXT("Missing prior display expectation");return false;}
        Text.ParseIntoArrayWS(Parts);
        if(Parts.Num()!=3){Error=TEXT("Invalid prior display expectation");return false;}
        const coastal::DisplayMode Expected{FCString::Atoi(*Parts[0]),FCString::Atoi(*Parts[1]),static_cast<coastal::DisplayWindowMode>(FCString::Atoi(*Parts[2]))};
        if(!Check(Display->Current()==Expected && SavedMode()==Expected,TEXT("Saved display did not survive fresh process without launch size overrides")))return false;
        UE_LOG(LogTemp,Display,TEXT("COASTAL_DISPLAY_RELOAD_PASS %s"),*Text);return true;
    }
    Error=TEXT("Unknown display test step");return false;
}
#endif
