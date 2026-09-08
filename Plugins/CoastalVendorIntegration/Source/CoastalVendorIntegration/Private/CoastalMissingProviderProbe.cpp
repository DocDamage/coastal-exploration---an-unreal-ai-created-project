#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalAGISAdapter.h"
#include "CoastalUISessionComponent.h"
#include "CoastalPanelWidget.h"
#include "CoastalMissionDirector.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalWorldObject.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
void FCoastalCampaignProbe::TickMissingProvider(UCoastalHostSession* Host)
{
    const double Now=FPlatformTime::Seconds();
    if(Now<Next)return;
    auto Fail=[&](const TCHAR* Why){Finished=true;UE_LOG(LogTemp,Error,TEXT("COASTAL_MISSING_PROVIDER_FAILED %s"),Why);FPlatformMisc::RequestExitWithStatus(false,1);};
    if(Now>Deadline){Fail(TEXT("Diagnostic test timed out"));return;}
    auto* PC=Cast<APlayerController>(Host->GetOwner());auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();
    auto* Provider=PC->GetPawn()->FindComponentByClass<UCoastalAGISAdapter>();
    if(Host->Started || !Provider || Provider->GetProviderStatus()!=ECoastalProviderResult::NotConfigured
        || !UI || !UI->IsInitialized() || !UI->HasModal())
    {Fail(TEXT("Missing provider did not retain blocked startup and native diagnostic modal"));return;}
    for(TActorIterator<ACoastalMissionDirector> It(Host->GetWorld());It;++It)
        if(It->Saves->HasActiveCampaign() || It->Saves->GetGeneration()!=0){Fail(TEXT("Diagnostic created a campaign"));return;}
    int32 Objects=0;
    for(TActorIterator<ACoastalWorldObject> It(Host->GetWorld());It;++It)
    {++Objects;if(It->IsActive() || It->IsHidden()){Fail(TEXT("Diagnostic changed authored world state"));return;}}
    if(Objects!=7){Fail(TEXT("Unexpected test-room manifest"));return;}
    bool Start=false,Continue=false,Diagnostic=false;UCoastalCommandButton* Quit=nullptr;UCoastalCommandButton* Exit=nullptr;
    for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
        if(It->GetWorld()==Host->GetWorld() && It->IsInViewport() && It->HasBeenPresented() && It->WidgetTree)
            It->WidgetTree->ForEachWidget([&](UWidget* W)
            {
                if(auto* T=Cast<UTextBlock>(W))Diagnostic|=T->GetText().ToString().Contains(TEXT("NOT CONFIGURED"));
                if(auto* B=Cast<UCoastalCommandButton>(W))
                {
                    if(B->Command==TEXT("start"))Start=!B->GetIsEnabled();
                    if(B->Command==TEXT("continue"))Continue=!B->GetIsEnabled();
                    if(B->Command==TEXT("quit") && B->GetIsEnabled())Quit=B;
                    if(B->Command==TEXT("exit_without_save") && B->GetIsEnabled())Exit=B;
                }
            });
    if(Phase==0)
    {
        if(!Start || !Continue || !Diagnostic || !Quit)return;
        FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot+TEXT("-diagnostic.png")),true,false);
        UE_LOG(LogTemp,Display,TEXT("COASTAL_MISSING_PROVIDER_PASS visible NotConfigured; Start/Continue disabled; seven pristine objects; no campaign"));
        Phase=1;Next=Now+3;return;
    }
    if(Phase==1 && Quit){Quit->OnClicked.Broadcast();Phase=2;Next=Now+1;return;}
    if(Phase==2 && Exit){UE_LOG(LogTemp,Display,TEXT("COASTAL_MISSING_PROVIDER_UI_EXIT"));Finished=true;Exit->OnClicked.Broadcast();}
}
#endif
