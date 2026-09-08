#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalAGISAdapter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "FirstSignalComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
bool RunCoastalCampaignWriteProbe(FCoastalCampaignProbe& Probe,UCoastalHostSession* Host,FName Kind,FString& Error)
{
    auto* PC=Cast<APlayerController>(Host->GetOwner());APawn* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* Provider=Pawn?Pawn->FindComponentByClass<UCoastalAGISAdapter>():nullptr;
    auto* Bridge=Pawn?Pawn->FindComponentByClass<UCoastalInteractionBridge>():nullptr;
    auto* Saves=Bridge?Bridge->GetCoordinator():nullptr;
    auto Fail=[&](const TCHAR* Why){Error=Why;return false;};
    if(!Saves || !Provider || !Probe.Slot.StartsWith(TEXT("coastal_test_")) || Saves->IsBusy() || Saves->IsRecoveryRequired())return Fail(TEXT("Campaign write probe missing actual owners"));
    const FString Base=FPaths::ProjectSavedDir()/TEXT("CoastalAcceptance")/Probe.Slot;
    auto ReadPair=[&](TArray<TArray<uint8>>& Pair){
        Pair.Empty();
        for(const TCHAR* Suffix:{TEXT("_A"),TEXT("_B")})
        {
            TArray<uint8> Bytes;
            if(!UGameplayStatics::LoadDataFromSlot(Bytes,TEXT("Coastal_")+Probe.Slot+Suffix,0))return false;
            Pair.Add(MoveTemp(Bytes));
        }
        return true;
    };
    if(Kind==TEXT("campaign_write_arm"))
    {
        if(Saves->SaveNow()!=ECoastalSaveResult::Saved)return Fail(TEXT("Cannot establish verified campaign pair"));
        FCoastalInventorySnapshot Snapshot;
        if(Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready || Snapshot.Receipts.Num()!=1
            || !ReadPair(Probe.FaultDisk))return Fail(TEXT("Cannot capture actual pre-failure campaign"));
        Probe.BeforeReturn=Snapshot.Payload;Probe.BeforeSave=Saves->GetGeneration();
        if(!FFileHelper::SaveStringToFile(Saves->GetCampaignId().ToString(),*(Base+TEXT("-campaign.txt")))
            || !FFileHelper::SaveArrayToFile(Snapshot.Payload,*(Base+TEXT("-payload.bin")))
            || !FFileHelper::SaveStringToFile(FString::Printf(TEXT("%lld"),Probe.BeforeSave+1),*(Base+TEXT("-generation.txt")))
            || !FFileHelper::SaveStringToFile(TEXT("explicit disposable acceptance fault"),*(Base+TEXT("-write-after.arm"))))return Fail(TEXT("Cannot arm campaign write fixture"));
        const auto Result=Saves->SaveNow();
        const bool Disarmed=IFileManager::Get().Delete(*(Base+TEXT("-write-after.arm")),true,false,true);
        if(!Disarmed || Result!=ECoastalSaveResult::DiskFailure || Saves->GetGeneration()!=Probe.BeforeSave)
            return Fail(TEXT("Valid write did not return DiskFailure while preserving live generation"));
        return true;
    }
    if(Kind==TEXT("campaign_write_verify"))
    {
        TArray<TArray<uint8>> After;
        FCoastalInventorySnapshot Snapshot;
        if(!ReadPair(After) || (int32(After[0]!=Probe.FaultDisk[0])+int32(After[1]!=Probe.FaultDisk[1]))!=1
            || Saves->GetGeneration()!=Probe.BeforeSave || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
            || Snapshot.Payload!=Probe.BeforeReturn || Snapshot.Receipts.Num()!=1)return Fail(TEXT("Ambiguous write mutated live inventory or both slots"));
        if(Saves->SaveNow()!=ECoastalSaveResult::InvalidSnapshot || !Saves->LastDetail.Contains(TEXT("newer disk generation"))
            || Saves->GetGeneration()!=Probe.BeforeSave)return Fail(TEXT("Next write did not block newer disk generation"));
        TArray<TArray<uint8>> Retried;
        if(!ReadPair(Retried) || Retried!=After || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready
            || Snapshot.Payload!=Probe.BeforeReturn || Snapshot.Receipts.Num()!=1)return Fail(TEXT("Refused retry changed disk or inventory"));
        for(int32 I=0;I<2;++I)
            if(!FFileHelper::SaveArrayToFile(After[I],*(Base+FString::Printf(TEXT("-after%d.bin"),I))))return Fail(TEXT("Cannot record post-fault pair"));
        UE_LOG(LogTemp,Display,TEXT("COASTAL_CAMPAIGN_AMBIGUOUS_PASS live_generation=%lld disk_generation=%lld previous_slot=1 inventory_unchanged=1 retry_blocked=1"),Probe.BeforeSave,Probe.BeforeSave+1);return true;
    }
    if(Kind==TEXT("campaign_write_reload"))
    {
        FString Campaign,Generation;TArray<uint8> Payload;TArray<TArray<uint8>> Disk;
        FCoastalInventorySnapshot Snapshot;FCoastalContainerView Bag;
        if(!FFileHelper::LoadFileToString(Campaign,*(Base+TEXT("-campaign.txt")))
            || !FFileHelper::LoadFileToString(Generation,*(Base+TEXT("-generation.txt")))
            || !FFileHelper::LoadFileToArray(Payload,*(Base+TEXT("-payload.bin"))) || !ReadPair(Disk)
            || Campaign!=Saves->GetCampaignId().ToString() || FCString::Atoi64(*Generation)!=Saves->GetGeneration()
            || !Saves->HasActiveCampaign() || !Bridge->AllowsWorldInput()
            || Provider->ExportInventory(Snapshot)!=ECoastalProviderResult::Ready || Snapshot.Payload!=Payload || Snapshot.Receipts.Num()!=1
            || Provider->ReadContainerView(TEXT("container.player"),Bag)!=ECoastalProviderResult::Ready
            || Bag.Items.Num()!=1 || Bag.Items[0].ItemId!=TEXT("item.radio_battery") || Bag.Items[0].Quantity!=1
            || Saves->GetMission()->ExportSnapshot().bRadioRepaired)return Fail(TEXT("Fresh Continue did not restore newer valid campaign and actual battery"));
        for(int32 I=0;I<2;++I)
        {
            TArray<uint8> Prior;
            if(!FFileHelper::LoadFileToArray(Prior,*(Base+FString::Printf(TEXT("-after%d.bin"),I))) || Prior!=Disk[I])return Fail(TEXT("Continue rewrote fault pair"));
        }
        const auto Before=Saves->GetGeneration();
        if(Saves->SaveNow()!=ECoastalSaveResult::Saved || Saves->GetGeneration()!=Before+1)return Fail(TEXT("Validated reload did not allow next verified save"));
        UE_LOG(LogTemp,Display,TEXT("COASTAL_CAMPAIGN_AMBIGUOUS_RELOAD_PASS campaign=%s loaded_generation=%lld next_verified=%lld battery=1 receipts=1"),*Campaign,Before,Saves->GetGeneration());return true;
    }
    return Fail(TEXT("Unknown campaign write step"));
}
#endif
