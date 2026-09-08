#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalAGISAdapter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalUISessionComponent.h"
#include "CoastalPanelWidget.h"
#include "FirstSignalComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool RunCoastalCampaignWriteProbe(FCoastalCampaignProbe& Probe,UCoastalHostSession* Host,FName Kind,FString& Error);

bool RunCoastalOcclusionProbe(FCoastalCampaignProbe& Probe,UCoastalHostSession* Host,FName Kind,FString& Error);

bool FCoastalCampaignProbe::ClearSaveFault()
{
    bool Good=true;auto& Files=FPlatformFileManager::Get().GetPlatformFile();
    for(const auto& Path:FaultPaths)
        if(!Files.SetReadOnly(*Path,false))Good=false;
    if(Good)FaultPaths.Empty();
    else UE_LOG(LogTemp,Error,TEXT("COASTAL_SAVE_FAULT_CLEANUP_FAILED: restore test-slot write attributes externally before retrying"));
    return Good;
}
bool FCoastalCampaignProbe::RunSaveFaultStep(UCoastalHostSession* Host,FName Kind,FString& Error)
{
    if(Mode==TEXT("campaign_ambiguous") || Mode==TEXT("campaign_ambiguous_reload"))return RunCoastalCampaignWriteProbe(*this,Host,Kind,Error);
    if(Mode==TEXT("occlusion") || Mode==TEXT("distance") || Mode==TEXT("occlusion_radio") || Mode==TEXT("distance_radio"))return RunCoastalOcclusionProbe(*this,Host,Kind,Error);
    if(Kind==TEXT("reject"))return VerifyRejectedSave(Host,Error);
    auto* PC=Cast<APlayerController>(Host->GetOwner());auto* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* Provider=Pawn?Pawn->FindComponentByClass<UCoastalAGISAdapter>():nullptr;
    auto* Bridge=Pawn?Pawn->FindComponentByClass<UCoastalInteractionBridge>():nullptr;
    auto* Saves=Bridge?Bridge->GetCoordinator():nullptr;
    if(!Saves || !Provider || !Slot.StartsWith(TEXT("coastal_test_")))
    {Error=TEXT("Save fault requires actual owners and a disposable test slot");return false;}
    const FString RecoveryBase=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot+TEXT("-recovery"));
    if(Kind==TEXT("terminal_reloaded"))
    {
        FString Prior;TArray<uint8> Payload;FCoastalInventorySnapshot Snapshot;FCoastalContainerView Bag;
        auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();
        if(!FFileHelper::LoadFileToString(Prior,*(RecoveryBase+TEXT("-campaign.txt")))
            || Prior!=Saves->GetCampaignId().ToString() || !Saves->HasActiveCampaign() || Saves->IsRecoveryRequired()
            || Saves->IsBusy() || !UI || UI->HasModal() || !Bridge->AllowsWorldInput()
            || !FFileHelper::LoadFileToArray(Payload,*(RecoveryBase+TEXT("-payload.bin")))
            || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready || Snapshot.Payload!=Payload
            || Snapshot.Receipts.Num()!=1 || Saves->GetMission()->ExportSnapshot().bRadioRepaired
            || Provider->ReadContainerView(TEXT("container.player"),Bag)!=ECoastalProviderResult::Ready
            || Bag.Items.Num()!=1 || Bag.Items[0].ItemId!=TEXT("item.radio_battery") || Bag.Items[0].Quantity!=1)
        {Error=TEXT("Fresh process did not restore the preserved partial campaign after terminal recovery");return false;}
        if(!Saves->BeginMutation()){Error=TEXT("Fresh session mutation gate remains blocked");return false;}
        Saves->EndMutation();
        UE_LOG(LogTemp,Display,TEXT("COASTAL_TERMINAL_RELAUNCH_PASS campaign=%s receipts=1 battery=1 input_restored=1"),*Prior);return true;
    }
    if(Kind==TEXT("terminal_back"))
    {
        auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();bool Sent=false;
        for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
            if(UI && It->GetWorld()==Host->GetWorld() && It->IsInViewport() && It->HasBeenPresented()
                && It->Ticket.kind==coastal::PanelKind::Recovery)
            {UI->Command(*It,TEXT("back"));Sent=true;break;}
        if(!Sent){Error=TEXT("Could not attempt stale Back command on recovery panel");return false;}
        return true;
    }
    if(Kind==TEXT("terminal_arm"))
    {
        if(Saves->SaveNow()!=ECoastalSaveResult::Saved){Error=TEXT("Recovery fixture save failed");return false;}
        FCoastalInventorySnapshot Prior;
        if(Provider->ExportInventory(Prior)!=ECoastalProviderResult::Ready
            || !FFileHelper::SaveStringToFile(Saves->GetCampaignId().ToString(),*(RecoveryBase+TEXT("-campaign.txt")))
            || !FFileHelper::SaveArrayToFile(Prior.Payload,*(RecoveryBase+TEXT("-payload.bin"))))
        {Error=TEXT("Could not record recovery relaunch evidence");return false;}
        FaultDisk.Empty();
        for(const TCHAR* Suffix:{TEXT("_A"),TEXT("_B")})
        {
            TArray<uint8> Bytes;
            if(!UGameplayStatics::LoadDataFromSlot(Bytes,TEXT("Coastal_")+Slot+Suffix,0))
            {Error=TEXT("Recovery fixture needs two verified disk files");return false;}
            FaultDisk.Add(MoveTemp(Bytes));
        }
        if(Saves->LoadCampaign(Saves->GetActiveSaveSet())!=ECoastalSaveResult::RecoveryRequired
            || !Saves->IsRecoveryRequired())
        {Error=TEXT("Late restore/rollback fault did not enter terminal recovery");return false;}
        BeforeSave=Saves->GetGeneration();
        UE_LOG(LogTemp,Display,TEXT("COASTAL_TERMINAL_RECOVERY_ARMED"));return true;
    }
    if(Kind==TEXT("terminal_verify"))
    {
        auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();
        bool RecoveryShown=false,ExitEnabled=false,OtherEnabled=false,GuidanceShown=false;
        for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
        {
            auto* P=*It;
            if(P->GetWorld()!=Host->GetWorld() || !P->IsInViewport() || !P->IsVisible() || !P->HasBeenPresented() || !P->WidgetTree)continue;
            RecoveryShown|=P->Ticket.kind==coastal::PanelKind::Recovery;
            P->WidgetTree->ForEachWidget([&](UWidget* W){
                if(auto* T=Cast<UTextBlock>(W))GuidanceShown|=T->GetText().ToString().Contains(TEXT("Close the game, relaunch, and load a validated save"));
                if(auto* B=Cast<UCoastalCommandButton>(W))if(B->GetIsEnabled())
                {if(B->Command==TEXT("exit_without_save"))ExitEnabled=true;else OtherEnabled=true;}});
        }
        const bool MutationAccepted=Saves->BeginMutation();
        if(MutationAccepted)Saves->EndMutation();
        if(!UI || !UI->HasModal() || !RecoveryShown || !GuidanceShown || !ExitEnabled || OtherEnabled || MutationAccepted
            || !Saves->IsRecoveryRequired() || Saves->SaveNow()!=ECoastalSaveResult::Busy
            || Bridge->AllowsWorldInput() || Saves->GetGeneration()!=BeforeSave)
        {Error=TEXT("Terminal recovery did not lock mutation/save/input or present exit-only UI");return false;}
        int32 I=0;
        for(const TCHAR* Suffix:{TEXT("_A"),TEXT("_B")})
        {
            TArray<uint8> Bytes;
            if(!UGameplayStatics::LoadDataFromSlot(Bytes,TEXT("Coastal_")+Slot+Suffix,0) || Bytes!=FaultDisk[I++])
            {Error=TEXT("Terminal recovery changed a disk save");return false;}
        }
        UE_LOG(LogTemp,Display,TEXT("COASTAL_TERMINAL_RECOVERY_PASS disk_unchanged=1 mutation_locked=1 save_locked=1 exit_only_ui=1"));return true;
    }
    auto& Files=FPlatformFileManager::Get().GetPlatformFile();
    if(Kind==TEXT("arm"))
    {
        if(!FaultPaths.IsEmpty()){Error=TEXT("Save fault already armed");return false;}
        FCoastalInventorySnapshot Snapshot;
        if(Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready)
        {Error=TEXT("Cannot export live provider before fault");return false;}
        BeforeReturn=Snapshot.Payload;BeforeSave=Saves->GetGeneration();FaultDisk.Empty();
        for(const TCHAR* Suffix:{TEXT("_A.sav"),TEXT("_B.sav")})
        {
            const FString Path=FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("SaveGames"),TEXT("Coastal_")+Slot+Suffix));
            TArray<uint8> Bytes;
            const FString Backup=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot+TEXT("-before-write-fault")+Suffix);
            if(Files.IsReadOnly(*Path) || Files.FileExists(*Backup) || !FFileHelper::LoadFileToArray(Bytes,*Path)
                || !FFileHelper::SaveArrayToFile(Bytes,*Backup))
            {Error=TEXT("Cannot preserve both writable test files before fault");return false;}
            FaultPaths.Add(Path);FaultDisk.Add(MoveTemp(Bytes));
        }
        for(const auto& Path:FaultPaths)
            if(!Files.SetReadOnly(*Path,true) || !Files.IsReadOnly(*Path))
            {Error=TEXT("Actual read-only fault could not be armed");return false;}
        UE_LOG(LogTemp,Display,TEXT("COASTAL_SAVE_FAULT_ARMED generation=%lld"),BeforeSave);return true;
    }
    if(Kind==TEXT("verify"))
    {
        bool Good=FaultPaths.Num()==2 && FaultDisk.Num()==2 && Saves->GetGeneration()==BeforeSave
            && !Saves->IsBusy() && !Saves->IsRecoveryRequired()
            && Saves->LastDetail==TEXT("Save write/readback validation failed. The other slot was not touched.");
        for(int32 I=0;I<FaultPaths.Num();++I)
        {
            TArray<uint8> Bytes;
            Good &= FFileHelper::LoadFileToArray(Bytes,*FaultPaths[I]) && FaultDisk.IsValidIndex(I) && Bytes==FaultDisk[I];
        }
        FCoastalInventorySnapshot Snapshot;
        Good &= Provider->ExportInventory(Snapshot)==ECoastalProviderResult::Ready && Snapshot.Payload==BeforeReturn;
        const bool Restored=ClearSaveFault();
        if(!Good || !Restored){Error=TEXT("Write refusal did not conserve both disk files/live state, or cleanup failed");return false;}
        UE_LOG(LogTemp,Display,TEXT("COASTAL_SAVE_FAULT_PASS actual write refused; both disk files, generation and AGIS payload unchanged; permissions restored"));
        return true;
    }
    Error=TEXT("Unknown save fault step");return false;
}
#endif
