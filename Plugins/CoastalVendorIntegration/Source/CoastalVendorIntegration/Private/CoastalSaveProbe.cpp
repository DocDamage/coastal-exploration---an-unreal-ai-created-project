#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalAGISAdapter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalWorldObject.h"
#include "CoastalUISessionComponent.h"
#include "CoastalPanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "UObject/StrongObjectPtr.h"
#include "FirstSignalComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool FCoastalCampaignProbe::ConfigureSaveSteps()
{
    if(Mode==TEXT("campaign_ambiguous"))
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},
            {TEXT("use"),TEXT("world.test.battery")},{TEXT("save_fault"),TEXT("campaign_write_arm")},
            {TEXT("save_fault"),TEXT("campaign_write_verify")},
            {TEXT("pause"),NAME_None},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("campaign_ambiguous_reload"))
        Steps={{TEXT("click"),TEXT("continue")},{TEXT("save_fault"),TEXT("campaign_write_reload")},
            {TEXT("pause"),NAME_None},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("occlusion_radio") || Mode==TEXT("distance_radio"))
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},
            {TEXT("use"),TEXT("world.test.radio")},{TEXT("check"),TEXT("inspected")},
            {TEXT("use"),TEXT("world.test.battery")},{TEXT("use"),TEXT("world.test.fuse")},{TEXT("check"),TEXT("collected")},
            {TEXT("save_fault"),TEXT("occlusion_arm")},{TEXT("save_fault"),TEXT("occlusion_input")},
            {TEXT("save_fault"),TEXT("occlusion_release")},{TEXT("save_fault"),TEXT("occlusion_verify")},
            {TEXT("use"),TEXT("world.test.radio")},{TEXT("save_fault"),TEXT("occlusion_collected")},
            {TEXT("pause"),NAME_None},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("occlusion") || Mode==TEXT("distance"))
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},
            {TEXT("save_fault"),TEXT("occlusion_arm")},{TEXT("save_fault"),TEXT("occlusion_input")},
            {TEXT("save_fault"),TEXT("occlusion_release")},{TEXT("save_fault"),TEXT("occlusion_verify")},
            {TEXT("use"),TEXT("world.test.battery")},{TEXT("save_fault"),TEXT("occlusion_collected")},
            {TEXT("pause"),NAME_None},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("recovery_fault"))
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},
            {TEXT("use"),TEXT("world.test.battery")},{TEXT("save_fault"),TEXT("terminal_arm")},
            {TEXT("save_fault"),TEXT("terminal_verify")},{TEXT("pause"),NAME_None},
            {TEXT("save_fault"),TEXT("terminal_verify")},{TEXT("save_fault"),TEXT("terminal_back")},
            {TEXT("save_fault"),TEXT("terminal_verify")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("recovery_reload"))
        Steps={{TEXT("click"),TEXT("continue")},{TEXT("save_fault"),TEXT("terminal_reloaded")},
            {TEXT("pause"),NAME_None},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("storage_revision"))
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},
            {TEXT("use"),TEXT("world.test.battery")},{TEXT("use"),TEXT("world.test.storage")},
            {TEXT("save_probe"),TEXT("revision_out")},{TEXT("save_probe"),TEXT("revision_back")},
            {TEXT("save_probe"),TEXT("revision_ready")},
            {TEXT("click"),TEXT("transfer")},{TEXT("save_probe"),TEXT("revision_refused")},
            {TEXT("click"),TEXT("transfer")},{TEXT("save_probe"),TEXT("revision_reviewed")},
            {TEXT("click"),TEXT("back")},{TEXT("pause"),NAME_None},
            {TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("storage_stale"))
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},
            {TEXT("use"),TEXT("world.test.battery")},{TEXT("use"),TEXT("world.test.storage")},
            {TEXT("save_probe"),TEXT("stale_move")},{TEXT("save_probe"),TEXT("stale_closed")},
            {TEXT("use"),TEXT("world.test.storage")},{TEXT("click"),TEXT("transfer")},
            {TEXT("save_probe"),TEXT("stale_reopened")},{TEXT("click"),TEXT("back")},
            {TEXT("pause"),NAME_None},{TEXT("click"),TEXT("quit")},
            {TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("partial_new"))
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},
            {TEXT("use"),TEXT("world.test.radio")},{TEXT("use"),TEXT("world.test.battery")},
            {TEXT("use"),TEXT("world.test.storage")},{TEXT("click"),TEXT("transfer")},
            {TEXT("click"),TEXT("back")},{TEXT("pause"),NAME_None},
            {TEXT("click"),TEXT("save")},{TEXT("check"),TEXT("saved")},
            {TEXT("save_probe"),TEXT("write_partial")},{TEXT("click"),TEXT("quit")},
            {TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("partial_continue"))
        Steps={{TEXT("click"),TEXT("continue")},{TEXT("save_probe"),TEXT("read_partial")},
            {TEXT("use"),TEXT("world.test.fuse")},{TEXT("use"),TEXT("world.test.storage")},
            {TEXT("click"),TEXT("switch_side")},{TEXT("click"),TEXT("transfer")},
            {TEXT("check"),TEXT("collected")},{TEXT("click"),TEXT("back")},
            {TEXT("use"),TEXT("world.test.radio")},{TEXT("check"),TEXT("repaired")},
            {TEXT("use"),TEXT("world.test.radio")},{TEXT("click"),TEXT("acknowledge")},
            {TEXT("check"),TEXT("complete")},{TEXT("pause"),NAME_None},
            {TEXT("click"),TEXT("save")},{TEXT("check"),TEXT("saved")},
            {TEXT("finish"),NAME_None},{TEXT("click"),TEXT("quit")},
            {TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("write_failure"))
        Steps={{TEXT("click"),TEXT("continue")},{TEXT("check"),TEXT("loaded")},
            {TEXT("pause"),NAME_None},{TEXT("save_fault"),TEXT("arm")},
            {TEXT("click"),TEXT("save")},{TEXT("save_fault"),TEXT("verify")},
            {TEXT("click"),TEXT("save")},{TEXT("check"),TEXT("saved")},
            {TEXT("finish"),NAME_None},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else if(Mode==TEXT("both_corrupt") || Mode==TEXT("future_save") || Mode==TEXT("equal_save") || Mode==TEXT("foreign_save"))
        Steps={{TEXT("save_fault"),TEXT("reject")},{TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else return false;
    return true;
}
bool FCoastalCampaignProbe::RunSaveStep(UCoastalHostSession* Host,FName Kind,FString& Error)
{
    auto* PC=Cast<APlayerController>(Host->GetOwner());auto* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* Provider=Pawn?Pawn->FindComponentByClass<UCoastalAGISAdapter>():nullptr;
    auto* Bridge=Pawn?Pawn->FindComponentByClass<UCoastalInteractionBridge>():nullptr;
    auto* Saves=Bridge?Bridge->GetCoordinator():nullptr;
    if(!Saves || !Provider){Error=TEXT("Missing real save/provider owners");return false;}
    FCoastalContainerView Bag,Storage;FCoastalInventorySnapshot Snapshot;
    if(Mode==TEXT("transcript_stale"))
    {
        static TStrongObjectPtr<UCoastalPanelWidget> OldPanel;
        static TStrongObjectPtr<UCoastalCommandButton> OldButton;
        static FGuid OldToken;
        auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();
        auto Fail=[&](const TCHAR* Why){OldButton.Reset();OldPanel.Reset();OldToken.Invalidate();Error=Why;return false;};
        const auto Mission=Saves->GetMission()->ExportSnapshot();
        if(!UI || !Saves->HasActiveCampaign() || Saves->IsBusy() || Saves->IsRecoveryRequired()
            || !Mission.bRadioRepaired || Mission.bMessageHeard
            || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready)
            return Fail(TEXT("Stale transcript requires a repaired, unacknowledged real campaign"));
        if(Kind==TEXT("transcript_load"))
        {
            for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
            {
                auto* Panel=*It;
                if(Panel->GetWorld()!=Host->GetWorld() || !Panel->IsInViewport() || !Panel->IsVisible()
                    || !Panel->HasBeenPresented() || !Panel->WidgetTree)continue;
                Panel->WidgetTree->ForEachWidget([&](UWidget* W)
                {if(auto* B=Cast<UCoastalCommandButton>(W))if(B->Command==TEXT("acknowledge") && B->GetIsEnabled())
                    {OldPanel.Reset(Panel);OldButton.Reset(B);}});
            }
            if(!OldPanel.IsValid() || !OldButton.IsValid())return Fail(TEXT("No painted native acknowledgement callback to retain"));
            OldToken=Bridge->GetTranscriptTokenForAcceptance();
            if(!OldToken.IsValid())return Fail(TEXT("Actual open transcript has no issued token"));
            if(Saves->SaveNow()!=ECoastalSaveResult::Saved)return Fail(TEXT("Pre-load unacknowledged save failed"));
            const uint64 Epoch=Saves->GetSessionEpoch();
            CapacityBefore=Snapshot;
            if(Saves->LoadCampaign(Saves->GetActiveSaveSet())!=ECoastalSaveResult::Loaded
                || Saves->GetSessionEpoch()==Epoch)return Fail(TEXT("Successful real load did not advance epoch"));
            UE_LOG(LogTemp,Display,TEXT("COASTAL_TRANSCRIPT_STALE_LOAD_PASS old_widget_retained=1"));
            return true;
        }
        const bool RawToken=Kind==TEXT("transcript_old_token");
        if((!RawToken && (Kind!=TEXT("transcript_old_callback") || !OldPanel.IsValid() || !OldButton.IsValid()
            || OldPanel->IsInViewport())) || UI->HasModal() || !OldToken.IsValid())return Fail(TEXT("Load did not retire old transcript UI/token fixture"));
        const auto Journal=Saves->GetJournal();const auto Generation=Saves->GetGeneration();
        if(RawToken)
        {
            if(Bridge->AcknowledgeTranscript(OldToken)!=ECoastalActionResult::InvalidTarget)
                return Fail(TEXT("Direct old token was not rejected as InvalidTarget after load"));
        }
        else OldButton->OnClicked.Broadcast();
        if(Saves->GetMission()->ExportSnapshot().bMessageHeard || Saves->GetJournal()!=Journal
            || Saves->GetGeneration()!=Generation || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
            || Snapshot.Payload!=CapacityBefore.Payload || Snapshot.CampaignId!=CapacityBefore.CampaignId
            || Snapshot.Receipts.Num()!=CapacityBefore.Receipts.Num())
            return Fail(TEXT("Old widget callback changed mission, journal, inventory or save"));
        for(int32 I=0;I<Snapshot.Receipts.Num();++I)
            if(Snapshot.Receipts[I].TransactionId!=CapacityBefore.Receipts[I].TransactionId
                || Snapshot.Receipts[I].Fingerprint!=CapacityBefore.Receipts[I].Fingerprint)
                return Fail(TEXT("Old widget callback changed receipts"));
        OldButton.Reset();OldPanel.Reset();
        if(RawToken)
        {
            OldToken.Invalidate();
            UE_LOG(LogTemp,Display,TEXT("COASTAL_TRANSCRIPT_STALE_TOKEN_PASS invalid_target=1 mission_unacknowledged=1 state_unchanged=1"));
        }
        else
        {
            UE_LOG(LogTemp,Display,TEXT("COASTAL_TRANSCRIPT_STALE_CALLBACK_PASS mission_unacknowledged=1 state_unchanged=1"));
        }
        return true;
    }
    if(Mode==TEXT("storage_revision"))
    {
        auto Fail=[&](const TCHAR* Message){Error=Message;return false;};
        auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();
        if(!UI || !UI->HasModal() || !Saves->HasActiveCampaign() || Saves->IsBusy()
            || Saves->IsRecoveryRequired() || Bridge->ValidateOpenStorage()!=ECoastalActionResult::Applied
            || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
            || Provider->ReadContainerView(TEXT("container.player"),Bag)!=ECoastalProviderResult::Ready
            || Provider->ReadContainerView(TEXT("world.test.storage"),Storage)!=ECoastalProviderResult::Ready)
            return Fail(TEXT("Revision test requires real open storage and ready owners"));
        if(Kind==TEXT("revision_out") || Kind==TEXT("revision_back"))
        {
            const bool Out=Kind==TEXT("revision_out");
            const auto& Source=Out?Bag:Storage;
            if(Source.Items.Num()!=1 || Source.Items[0].ItemId!=TEXT("item.radio_battery"))
                return Fail(TEXT("Revision fixture has no unique battery"));
            if(Out)CapacityGeneration=Bag.Revision;
            FCoastalTransferRequest Request;
            Request.OperationId=FGuid::NewGuid();Request.ItemInstanceId=Source.Items[0].InstanceId;
            Request.SourceContainer=Out?FName(TEXT("container.player")):FName(TEXT("world.test.storage"));
            Request.DestinationContainer=Out?FName(TEXT("world.test.storage")):FName(TEXT("container.player"));
            // Real coordinator-guarded transactions outside the UI leave its displayed projection stale.
            if(Bridge->TransferWithOpenStorage(Request)!=ECoastalActionResult::Applied
                || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready)
                return Fail(TEXT("Real revision fixture transfer failed"));
            if(!Out)
            {
                if(Provider->ReadContainerView(TEXT("container.player"),Bag)!=ECoastalProviderResult::Ready
                    || Bag.Revision==CapacityGeneration || Snapshot.Receipts.Num()!=3)
                    return Fail(TEXT("Round trip did not advance the real provider revision"));
                CapacityBefore=Snapshot;CapacityGeneration=Saves->GetGeneration();
            }
            return true;
        }
        if(Kind==TEXT("revision_ready"))
        {
            // Fixture autosave is deferred to coordinator tick; baseline only after it settles.
            if(Snapshot.Payload!=CapacityBefore.Payload || Snapshot.Receipts.Num()!=3)
                return Fail(TEXT("Revision fixture changed while settling"));
            CapacityGeneration=Saves->GetGeneration();return true;
        }
        if(Kind==TEXT("revision_refused"))
        {
            if(Bag.Items.Num()!=1 || !Storage.Items.IsEmpty() || Snapshot.Payload!=CapacityBefore.Payload
                || Snapshot.CampaignId!=CapacityBefore.CampaignId || Snapshot.Receipts.Num()!=3
                || Saves->GetGeneration()!=CapacityGeneration)
                return Fail(TEXT("Old displayed revision initiated a transfer or save"));
            for(int32 I=0;I<3;++I)
                if(Snapshot.Receipts[I].TransactionId!=CapacityBefore.Receipts[I].TransactionId
                    || Snapshot.Receipts[I].Fingerprint!=CapacityBefore.Receipts[I].Fingerprint)
                    return Fail(TEXT("Old display changed a receipt"));
            bool PresentedReview=false;
            for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
            {
                auto* Panel=*It;
                if(Panel->GetWorld()!=Host->GetWorld() || !Panel->IsInViewport() || !Panel->IsVisible()
                    || !Panel->HasBeenPresented() || !Panel->WidgetTree)continue;
                Panel->WidgetTree->ForEachWidget([&](UWidget* W)
                {if(auto* Text=Cast<UTextBlock>(W))PresentedReview|=Text->GetText().ToString().Contains(TEXT("Review the refreshed selection"));});
            }
            if(!PresentedReview)return Fail(TEXT("Refusal did not present the required review notice"));
        }
        else if(Kind==TEXT("revision_reviewed"))
        {
            if(!Bag.Items.IsEmpty() || Storage.Items.Num()!=1 || Storage.Items[0].ItemId!=TEXT("item.radio_battery")
                || Storage.Items[0].Quantity!=1 || Snapshot.Receipts.Num()!=4)
                return Fail(TEXT("Explicit reviewed transfer did not apply exactly once"));
        }
        else return Fail(TEXT("Unknown revision test step"));
        UE_LOG(LogTemp,Display,TEXT("COASTAL_STORAGE_REVISION_PASS step=%s receipts=%d"),*Kind.ToString(),Snapshot.Receipts.Num());
        return true;
    }
    if(Mode==TEXT("storage_stale"))
    {
        auto Fail=[&](const TCHAR* Message){Error=Message;return false;};
        auto* UI=PC->FindComponentByClass<UCoastalUISessionComponent>();
        if(!UI || !Saves->HasActiveCampaign() || Saves->IsBusy() || Saves->IsRecoveryRequired()
            || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
            || Provider->ReadContainerView(TEXT("container.player"),Bag)!=ECoastalProviderResult::Ready
            || Provider->ReadContainerView(TEXT("world.test.storage"),Storage)!=ECoastalProviderResult::Ready)
            return Fail(TEXT("Stale-storage test requires ready real owners"));
        if(Kind==TEXT("stale_reopened"))
        {
            if(!UI->HasModal() || Bridge->ValidateOpenStorage()!=ECoastalActionResult::Applied
                || !Bag.Items.IsEmpty() || Storage.Items.Num()!=1
                || Storage.Items[0].ItemId!=TEXT("item.radio_battery") || Storage.Items[0].Quantity!=1
                || Snapshot.Receipts.Num()!=2)
                return Fail(TEXT("Reopened actual storage UI did not transfer the preserved battery exactly once"));
            UE_LOG(LogTemp,Display,TEXT("COASTAL_STORAGE_STALE_PASS reopened_transfer receipts=2"));
            return true;
        }
        if(Bag.Items.Num()!=1 || Bag.Items[0].ItemId!=TEXT("item.radio_battery")
            || Bag.Items[0].Quantity!=1 || !Storage.Items.IsEmpty() || Snapshot.Receipts.Num()!=1)
            return Fail(TEXT("Stale storage changed battery ownership or receipts"));
        FCoastalTransferRequest Request;
        Request.OperationId=FGuid::NewGuid();Request.ItemInstanceId=Bag.Items[0].InstanceId;
        Request.SourceContainer=TEXT("container.player");Request.DestinationContainer=TEXT("world.test.storage");
        ECoastalActionResult Expected=ECoastalActionResult::InvalidTarget;
        if(Kind==TEXT("stale_move"))
        {
            if(!UI->HasModal() || Bridge->ValidateOpenStorage()!=ECoastalActionResult::Applied)
                return Fail(TEXT("Storage must be open and valid before moving"));
            CapacityBefore=Snapshot;
            // Move vertically beyond reach without crossing a checkpoint or waiting for UI tick.
            Pawn->SetActorLocation(Pawn->GetActorLocation()+FVector(0,0,600),false,nullptr,ETeleportType::TeleportPhysics);
            Expected=ECoastalActionResult::TooFar;
        }
        else if(Kind==TEXT("stale_closed"))
        {
            if(UI->HasModal() || Bridge->ValidateOpenStorage()!=ECoastalActionResult::InvalidTarget)
                return Fail(TEXT("UI did not retire the unreachable storage context"));
        }
        else return Fail(TEXT("Unknown stale-storage step"));
        const int64 Generation=Saves->GetGeneration();
        if(Bridge->TransferWithOpenStorage(Request)!=Expected
            || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
            || Snapshot.Payload!=CapacityBefore.Payload || Snapshot.CampaignId!=CapacityBefore.CampaignId
            || Snapshot.Receipts.Num()!=CapacityBefore.Receipts.Num() || Saves->GetGeneration()!=Generation)
            return Fail(TEXT("Stale transfer was not refused without inventory/save mutation"));
        for(int32 I=0;I<Snapshot.Receipts.Num();++I)
            if(Snapshot.Receipts[I].TransactionId!=CapacityBefore.Receipts[I].TransactionId
                || Snapshot.Receipts[I].Fingerprint!=CapacityBefore.Receipts[I].Fingerprint)
                return Fail(TEXT("Stale transfer changed a prior receipt"));
        UE_LOG(LogTemp,Display,TEXT("COASTAL_STORAGE_STALE_PASS step=%s result=%d receipts=1"),*Kind.ToString(),int32(Expected));
        return true;
    }
    const auto Mission=Saves->GetMission()->ExportSnapshot();
    bool Battery=false,Fuse=false;
    for(TActorIterator<ACoastalWorldObject> It(Host->GetWorld());It;++It)
    {
        if(It->WorldId==TEXT("world.test.battery"))Battery=It->IsActive();
        if(It->WorldId==TEXT("world.test.fuse"))Fuse=It->IsActive();
    }
    if(!Saves->HasActiveCampaign() || Saves->IsBusy() || Saves->IsRecoveryRequired()
        || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
        || Provider->ReadContainerView(TEXT("container.player"),Bag)!=ECoastalProviderResult::Ready
        || Provider->ReadContainerView(TEXT("world.test.storage"),Storage)!=ECoastalProviderResult::Ready
        || !Bag.Items.IsEmpty() || Storage.Items.Num()!=1
        || Storage.Items[0].ItemId!=TEXT("item.radio_battery") || Storage.Items[0].Quantity!=1
        || Snapshot.Receipts.Num()!=2 || !Battery || Fuse
        || !Mission.bRadioInspected || Mission.bRadioRepaired || Mission.bMessageHeard)
    {Error=TEXT("Partial campaign is not exactly one stored battery, recoverable fuse and inspected/unrepaired radio");return false;}
    const FString Base=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot);
    const FString Campaign=Saves->GetCampaignId().ToString();
    if(Kind==TEXT("write_partial"))
    {
        if(!FFileHelper::SaveStringToFile(Campaign,*(Base+TEXT("-campaign.txt")))
            || !FFileHelper::SaveArrayToFile(Snapshot.Payload,*(Base+TEXT("-partial-payload.bin"))))
        {Error=TEXT("Could not record prior partial campaign evidence");return false;}
    }
    else if(Kind==TEXT("read_partial"))
    {
        FString Prior;TArray<uint8> Payload;
        if(!FFileHelper::LoadFileToString(Prior,*(Base+TEXT("-campaign.txt"))) || Prior!=Campaign
            || !FFileHelper::LoadFileToArray(Payload,*(Base+TEXT("-partial-payload.bin"))) || Payload!=Snapshot.Payload)
        {Error=TEXT("Fresh-process partial campaign identity or complete AGIS payload differs from prior evidence");return false;}
    }
    else {Error=TEXT("Unknown partial-save test step");return false;}
    UE_LOG(LogTemp,Display,TEXT("COASTAL_PARTIAL_SAVE_PASS mode=%s campaign=%s stored_battery=1 world_fuse=1 receipts=2"),*Mode,*Campaign);
    return true;
}
#endif
