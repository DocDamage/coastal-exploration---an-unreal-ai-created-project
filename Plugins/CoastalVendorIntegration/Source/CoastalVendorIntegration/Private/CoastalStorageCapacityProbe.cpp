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

bool FCoastalCampaignProbe::RunStorageCapacityStep(UCoastalHostSession* Host,FName Kind,FString& Error)
{
    auto* PC=Cast<APlayerController>(Host->GetOwner());auto* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* UI=PC?PC->FindComponentByClass<UCoastalUISessionComponent>():nullptr;
    auto* Provider=Pawn?Pawn->FindComponentByClass<UCoastalAGISAdapter>():nullptr;
    auto* Bridge=Pawn?Pawn->FindComponentByClass<UCoastalInteractionBridge>():nullptr;
    auto* Saves=Bridge?Bridge->GetCoordinator():nullptr;
    auto Fail=[&](const TCHAR* Why){Error=Why;return false;};
    if(!Saves || !Provider || !UI || !Saves->HasActiveCampaign() || Saves->IsBusy() || Saves->IsRecoveryRequired())
        return Fail(TEXT("Storage capacity owners unavailable"));
    FCoastalInventorySnapshot Snapshot;FCoastalContainerView Bag,Storage;
    if(Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
        || Provider->ReadContainerView(TEXT("container.player"),Bag)!=ECoastalProviderResult::Ready
        || Provider->ReadContainerView(TEXT("world.test.storage"),Storage)!=ECoastalProviderResult::Ready)
        return Fail(TEXT("Cannot read real storage-capacity state"));
    const bool Complete=Kind==TEXT("complete"),Stored=Kind==TEXT("stored");
    const bool OneCell=Kind==TEXT("one_cell") || Kind==TEXT("one_refused");
    const int32 ExpectedBag=(Complete || Stored || OneCell)?2:1,ExpectedStore=Complete?46:(Stored || OneCell)?47:48;
    const auto Mission=Saves->GetMission()->ExportSnapshot();
    if(Bag.Grid!=FIntPoint(6,4) || Storage.Grid!=FIntPoint(8,6) || Bag.Items.Num()!=ExpectedBag || Storage.Items.Num()!=ExpectedStore
        || !Mission.bRadioInspected || Mission.bRadioRepaired!=Complete || Mission.bMessageHeard!=Complete)
        return Fail(TEXT("Storage capacity grid, quantity or mission mismatch"));
    int32 Postcards=0,Batteries=0;TSet<FGuid> Ids;
    for(const auto* View:{&Bag,&Storage})for(const auto& Item:View->Items)
    {
        if(Item.Quantity!=1 || Ids.Contains(Item.InstanceId))return Fail(TEXT("Duplicate or stacked capacity item"));
        Ids.Add(Item.InstanceId);
        if(Item.ItemId==TEXT("item.old_postcard"))++Postcards;
        else if(!Complete && Item.ItemId==TEXT("item.radio_battery"))
        {
            ++Batteries;
            if((View==&Storage)!=Stored)return Fail(TEXT("Battery is in the wrong real container"));
        }
        else return Fail(TEXT("Unexpected capacity inventory item"));
    }
    if(Postcards!=48 || Batteries!=(Complete?0:1) || Snapshot.ProviderVersion!=TEXT("agis-blueprint-20260906.codec2"))
        return Fail(TEXT("Storage capacity item conservation failed"));
    int32 Pickups=0;
    for(TActorIterator<ACoastalWorldObject> It(Host->GetWorld());It;++It)
    {
        const int32 Uid=Provider->UidForWorld(It->WorldId);if(!Uid)continue;++Pickups;
        const bool Collected=Uid==1?true:Uid==2?Complete:Uid<51;
        if(It->IsActive()!=Collected || It->IsHidden()!=Collected || It->GetActorEnableCollision()==Collected)
            return Fail(TEXT("Storage capacity pickup visibility/collision mismatch"));
    }
    if(Pickups!=74)return Fail(TEXT("Capacity world pickup manifest incomplete"));
    const FString Base=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CoastalAcceptance"),Slot);
    if(Kind==TEXT("full") || Kind==TEXT("one_cell"))
    {CapacityBefore=Snapshot;CapacityGeneration=Saves->GetGeneration();}
    else if(Kind==TEXT("refused") || Kind==TEXT("one_refused"))
    {
        if(Snapshot.Payload!=CapacityBefore.Payload || Snapshot.CampaignId!=CapacityBefore.CampaignId
            || Snapshot.Receipts.Num()!=CapacityBefore.Receipts.Num() || Saves->GetGeneration()!=CapacityGeneration
            || !UI->Status().ToString().Contains(TEXT("No space. The source item has not been removed.")))
            return Fail(TEXT("Full storage transfer did not visibly refuse without mutation"));
        for(int32 I=0;I<Snapshot.Receipts.Num();++I)
            if(Snapshot.Receipts[I].TransactionId!=CapacityBefore.Receipts[I].TransactionId
                || Snapshot.Receipts[I].Fingerprint!=CapacityBefore.Receipts[I].Fingerprint)
                return Fail(TEXT("Rejected storage transfer changed receipt"));
        FScreenshotRequest::RequestScreenshot(Base+TEXT("-")+Mode+TEXT("-")+Kind.ToString()+TEXT(".png"),true,false);
    }
    else if(Kind==TEXT("write_full"))
    {
        if(!FFileHelper::SaveStringToFile(Saves->GetCampaignId().ToString(),*(Base+TEXT("-campaign.txt")))
            || !FFileHelper::SaveArrayToFile(Snapshot.Payload,*(Base+TEXT("-storage-payload.bin"))))
            return Fail(TEXT("Cannot record full-storage persistence evidence"));
    }
    else if(Kind==TEXT("read_full"))
    {
        FString Id;TArray<uint8> Payload;
        if(!FFileHelper::LoadFileToString(Id,*(Base+TEXT("-campaign.txt"))) || !FGuid::Parse(Id,ExpectedCampaign)
            || ExpectedCampaign!=Saves->GetCampaignId() || !FFileHelper::LoadFileToArray(Payload,*(Base+TEXT("-storage-payload.bin")))
            || Payload!=Snapshot.Payload || Snapshot.Receipts.Num()!=97)
            return Fail(TEXT("Fresh process did not restore exact full-storage campaign"));
    }
    if(Kind==TEXT("write_full") || Complete)
    {
        if(Snapshot.Receipts.Num()!=(Complete?103:97))return Fail(TEXT("Full-storage receipt count mismatch"));
        UE_LOG(LogTemp,Display,TEXT("COASTAL_STORAGE_CAPACITY_PASS mode=%s campaign=%s generation=%lld postcards=%d receipts=%d complete=%d"),
            *Mode,*Saves->GetCampaignId().ToString(),Saves->GetGeneration(),Postcards,Snapshot.Receipts.Num(),Complete);
    }
    return true;
}
#endif
