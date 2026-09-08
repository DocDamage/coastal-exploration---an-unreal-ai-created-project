#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalPanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"

void FCoastalCampaignProbe::PrependGamepadOptions()
{
    TArray<FStep> Prefix={{TEXT("click"),TEXT("options")},{TEXT("click"),TEXT("option_defaults")}};
    if(FParse::Param(FCommandLine::Get(),TEXT("CoastalText150")))
    {
        for(int32 I=0;I<3;++I)Prefix.Add({TEXT("click"),TEXT("option_next")});
        for(int32 I=0;I<5;++I)Prefix.Add({TEXT("click"),TEXT("option_increase")});
    }
    Prefix.Append({{TEXT("click"),TEXT("option_session")},{TEXT("click"),TEXT("back")}});
    Prefix.Append(Steps);Steps=MoveTemp(Prefix);
}

bool FCoastalCampaignProbe::NavigateGamepad(UCoastalHostSession* Host,FName Command)
{
    if(!FSlateApplication::IsInitialized())return false;
    for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
    {
        auto* Panel=*It;
        if(Panel->GetWorld()!=Host->GetWorld() || !Panel->IsInViewport() || !Panel->IsVisible()
            || !Panel->HasBeenPresented() || !Panel->WidgetTree)continue;
        UCoastalCommandButton* Target=nullptr;
        Panel->WidgetTree->ForEachWidget([&](UWidget* W)
        {if(auto* B=Cast<UCoastalCommandButton>(W))if(B->Command==Command && B->GetIsEnabled())Target=B;});
        if(!Target)continue;
        auto Send=[](FKey Key)
        {
            const FKeyEvent Event(Key,FModifierKeysState(),0,false,0,0);
            const bool Handled=FSlateApplication::Get().ProcessKeyDownEvent(Event);
            FSlateApplication::Get().ProcessKeyUpEvent(Event);return Handled;
        };
        if(!Target->HasAnyUserFocus())
        {
            PendingGamepadCommand=NAME_None;
            Send(EKeys::Gamepad_DPad_Down);Next=FPlatformTime::Seconds()+0.15;
            return false;
        }
        if(PendingGamepadCommand!=Command || PendingGamepadTicket!=Panel->Ticket.id)
        {
            PendingGamepadCommand=Command;PendingGamepadTicket=Panel->Ticket.id;
            const FString File=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),
                Slot+TEXT("-gamepad-focus-")+FString::FromInt(Step)+TEXT(".png"));
            FScreenshotRequest::RequestScreenshot(File,true,false);
            Next=FPlatformTime::Seconds()+0.2;return false;
        }
        // No direct focus assignment or button-delegate invocation: Slate
        // routes the actual navigation/confirmation keys through the widget.
        const bool Handled=Send(EKeys::Gamepad_FaceButton_Bottom);
        PendingGamepadCommand=NAME_None;
        UE_LOG(LogTemp,Display,TEXT("COASTAL_GAMEPAD_UI_CONFIRM command=%s slate_handled=%d"),*Command.ToString(),Handled);
        return Handled;
    }
    return false;
}
#endif
