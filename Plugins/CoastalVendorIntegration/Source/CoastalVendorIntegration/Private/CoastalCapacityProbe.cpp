#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalAGISAdapter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalUISessionComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalWorldObject.h"
#include "FirstSignalComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool FCoastalCampaignProbe::ConfigureCapacitySteps()
{
    if(Mode==TEXT("capacity_new"))
    {
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},{TEXT("use"),TEXT("world.test.radio")}};
        for(int32 Uid=3;Uid<27;++Uid)Steps.Add({TEXT("use"),UCoastalAGISAdapter::WorldIdFor(Uid)});
        Steps.Append({{TEXT("capacity"),TEXT("full")},{TEXT("use"),TEXT("world.test.battery")},
            {TEXT("capacity"),TEXT("refused")},{TEXT("pause"),NAME_None},{TEXT("click"),TEXT("save")},
            {TEXT("check"),TEXT("saved")},{TEXT("capacity"),TEXT("write_full")},
            {TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}});
    }
    else if(Mode==TEXT("capacity_continue"))
    {
        Steps={{TEXT("click"),TEXT("continue")},{TEXT("capacity"),TEXT("read_full")},
            {TEXT("capacity"),TEXT("full")},{TEXT("use"),TEXT("world.test.battery")},{TEXT("capacity"),TEXT("refused")},
            {TEXT("use"),TEXT("world.test.storage")},{TEXT("click"),TEXT("transfer")},
            {TEXT("click"),TEXT("transfer")},{TEXT("click"),TEXT("transfer")},{TEXT("click"),TEXT("back")},
            {TEXT("use"),TEXT("world.test.battery")},{TEXT("use"),TEXT("world.test.fuse")},
            {TEXT("capacity"),TEXT("parts")},{TEXT("use"),TEXT("world.test.radio")},
            {TEXT("use"),TEXT("world.test.radio")},{TEXT("click"),TEXT("acknowledge")},
            {TEXT("check"),TEXT("complete")},{TEXT("pause"),NAME_None},{TEXT("click"),TEXT("save")},
            {TEXT("check"),TEXT("saved")},{TEXT("capacity"),TEXT("complete")},
            {TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    }
    else if(Mode==TEXT("capacity_storage_new"))
    {
        Steps={{TEXT("click"),TEXT("start")},{TEXT("check"),TEXT("new")},{TEXT("use"),TEXT("world.test.radio")}};
        for(int32 Batch=0;Batch<2;++Batch)
        {
            for(int32 I=0;I<24;++I)Steps.Add({TEXT("use"),UCoastalAGISAdapter::WorldIdFor(3+Batch*24+I)});
            Steps.Add({TEXT("use"),TEXT("world.test.storage")});
            for(int32 I=0;I<24;++I)Steps.Add({TEXT("click"),TEXT("transfer")});
            Steps.Add({TEXT("click"),TEXT("back")});
        }
        Steps.Append({{TEXT("use"),TEXT("world.test.battery")},{TEXT("use"),TEXT("world.test.storage")},
            {TEXT("capacity"),TEXT("full")},{TEXT("click"),TEXT("transfer")},{TEXT("capacity"),TEXT("refused")},
            {TEXT("click"),TEXT("back")},{TEXT("pause"),NAME_None},{TEXT("click"),TEXT("save")},
            {TEXT("check"),TEXT("saved")},{TEXT("capacity"),TEXT("write_full")},
            {TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}});
    }
    else if(Mode==TEXT("capacity_storage_continue"))
        Steps={{TEXT("click"),TEXT("continue")},{TEXT("capacity"),TEXT("read_full")},
            {TEXT("use"),TEXT("world.test.storage")},{TEXT("capacity"),TEXT("full")},
            {TEXT("click"),TEXT("transfer")},{TEXT("capacity"),TEXT("refused")},
            {TEXT("click"),TEXT("switch_side")},{TEXT("click"),TEXT("transfer")},
            {TEXT("click"),TEXT("switch_side")},{TEXT("capacity"),TEXT("one_cell")},
            {TEXT("click"),TEXT("transfer")},{TEXT("capacity"),TEXT("one_refused")},
            {TEXT("click"),TEXT("switch_side")},{TEXT("click"),TEXT("transfer")},
            {TEXT("click"),TEXT("switch_side")},{TEXT("click"),TEXT("transfer")},
            {TEXT("capacity"),TEXT("stored")},{TEXT("click"),TEXT("switch_side")},
            {TEXT("click"),TEXT("transfer")},{TEXT("click"),TEXT("back")},
            {TEXT("use"),TEXT("world.test.fuse")},{TEXT("use"),TEXT("world.test.radio")},
            {TEXT("use"),TEXT("world.test.radio")},{TEXT("click"),TEXT("acknowledge")},
            {TEXT("check"),TEXT("complete")},{TEXT("pause"),NAME_None},{TEXT("click"),TEXT("save")},
            {TEXT("check"),TEXT("saved")},{TEXT("capacity"),TEXT("complete")},
            {TEXT("click"),TEXT("quit")},{TEXT("click"),TEXT("exit_without_save")}};
    else return false;
    return true;
}

bool FCoastalCampaignProbe::RunCapacityStep(UCoastalHostSession* Host,FName Kind,FString& Error)
{
    if(Mode.StartsWith(TEXT("capacity_storage_")))return RunStorageCapacityStep(Host,Kind,Error);
    auto* PC=Cast<APlayerController>(Host->GetOwner());auto* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* UI=PC?PC->FindComponentByClass<UCoastalUISessionComponent>():nullptr;
    auto* Provider=Pawn?Pawn->FindComponentByClass<UCoastalAGISAdapter>():nullptr;
    auto* Bridge=Pawn?Pawn->FindComponentByClass<UCoastalInteractionBridge>():nullptr;
    auto* Saves=Bridge?Bridge->GetCoordinator():nullptr;
    auto Fail=[&](const TCHAR* Why){Error=Why;return false;};
    if(!Saves || !Provider || !UI || !Saves->HasActiveCampaign() || Saves->IsBusy() || Saves->IsRecoveryRequired())
        return Fail(TEXT("Capacity campaign owners unavailable"));
    FCoastalInventorySnapshot Snapshot;FCoastalContainerView Bag,Storage;
    if(Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
        || Provider->ReadContainerView(TEXT("container.player"),Bag)!=ECoastalProviderResult::Ready
        || Provider->ReadContainerView(TEXT("world.test.storage"),Storage)!=ECoastalProviderResult::Ready)
        return Fail(TEXT("Cannot read actual capacity inventory"));
    const auto Mission=Saves->GetMission()->ExportSnapshot();
    const bool Parts=Kind==TEXT("parts"),Complete=Kind==TEXT("complete");
    const int32 ExpectedBag=Parts?23:Complete?21:24,ExpectedStore=(Parts || Complete)?3:0;
    if(Bag.Grid!=FIntPoint(6,4) || Storage.Grid!=FIntPoint(8,6) || Bag.Items.Num()!=ExpectedBag || Storage.Items.Num()!=ExpectedStore
        || !Mission.bRadioInspected || Mission.bRadioRepaired!=Complete || Mission.bMessageHeard!=Complete)
        return Fail(TEXT("Capacity quantities, grid sizes or mission state differ"));
    int32 Postcards=0;TSet<FGuid> Ids;
    for(const auto* View:{&Bag,&Storage})for(const auto& Item:View->Items)
    {
        if(Item.Quantity!=1 || Ids.Contains(Item.InstanceId))return Fail(TEXT("Invalid capacity item amount/identity"));
        Ids.Add(Item.InstanceId);
        if(Item.ItemId==TEXT("item.old_postcard"))++Postcards;
        else if(!Parts || (Item.ItemId!=TEXT("item.radio_battery") && Item.ItemId!=TEXT("item.marine_fuse")))
            return Fail(TEXT("Unexpected item in capacity inventory"));
    }
    if(Postcards!=24 || Snapshot.ProviderVersion!=TEXT("agis-blueprint-20260906.codec2"))return Fail(TEXT("Postcard conservation or codec failed"));
    int32 Pickups=0;
    for(TActorIterator<ACoastalWorldObject> It(Host->GetWorld());It;++It)
    {
        const int32 Uid=Provider->UidForWorld(It->WorldId);if(!Uid)continue;++Pickups;
        const bool Collected=Uid<=2?(Parts || Complete):Uid<27;
        if(It->IsActive()!=Collected || It->IsHidden()!=Collected || It->GetActorEnableCollision()==Collected)
            return Fail(TEXT("Capacity world pickup state/visibility/collision differs"));
    }
    if(Pickups!=74)return Fail(TEXT("Capacity world pickup manifest incomplete"));
    const FString Base=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot);
    if(Kind==TEXT("full")){CapacityBefore=Snapshot;CapacityGeneration=Saves->GetGeneration();}
    else if(Kind==TEXT("refused"))
    {
        if(Saves->GetGeneration()!=CapacityGeneration || Snapshot.Payload!=CapacityBefore.Payload
            || Snapshot.CampaignId!=CapacityBefore.CampaignId || Snapshot.Receipts.Num()!=CapacityBefore.Receipts.Num()
            || !UI->Status().ToString().Contains(TEXT("No space. The source item has not been removed.")))
            return Fail(TEXT("Full pickup did not visibly refuse while conserving campaign state"));
        for(int32 I=0;I<Snapshot.Receipts.Num();++I)
            if(Snapshot.Receipts[I].TransactionId!=CapacityBefore.Receipts[I].TransactionId
                || Snapshot.Receipts[I].Fingerprint!=CapacityBefore.Receipts[I].Fingerprint)
                return Fail(TEXT("Capacity refusal changed a receipt"));
        FScreenshotRequest::RequestScreenshot(Base+TEXT("-no-space.png"),true,false);
    }
    else if(Kind==TEXT("write_full"))
    {
        if(!FFileHelper::SaveStringToFile(Saves->GetCampaignId().ToString(),*(Base+TEXT("-campaign.txt")))
            || !FFileHelper::SaveArrayToFile(Snapshot.Payload,*(Base+TEXT("-capacity-payload.bin"))))
            return Fail(TEXT("Could not record full-capacity relaunch evidence"));
    }
    else if(Kind==TEXT("read_full"))
    {
        FString Id;TArray<uint8> Payload;
        if(!FFileHelper::LoadFileToString(Id,*(Base+TEXT("-campaign.txt"))) || !FGuid::Parse(Id,ExpectedCampaign)
            || ExpectedCampaign!=Saves->GetCampaignId() || !FFileHelper::LoadFileToArray(Payload,*(Base+TEXT("-capacity-payload.bin")))
            || Payload!=Snapshot.Payload || Snapshot.Receipts.Num()!=24)
            return Fail(TEXT("Fresh process did not restore exact full-capacity campaign"));
    }
    if(Kind==TEXT("write_full") || Complete)
    {
        if(Snapshot.Receipts.Num()!=(Complete?30:24))return Fail(TEXT("Capacity receipt count differs"));
        UE_LOG(LogTemp,Display,TEXT("COASTAL_CAPACITY_PASS mode=%s campaign=%s generation=%lld postcards=%d receipts=%d complete=%d"),
            *Mode,*Saves->GetCampaignId().ToString(),Saves->GetGeneration(),Postcards,Snapshot.Receipts.Num(),Complete);
        FScreenshotRequest::RequestScreenshot(Base+TEXT("-")+Mode+TEXT("-complete.png"),true,false);
    }
    return true;
}
#endif
