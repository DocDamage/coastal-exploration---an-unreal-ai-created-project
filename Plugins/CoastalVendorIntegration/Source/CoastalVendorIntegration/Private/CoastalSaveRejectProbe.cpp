#include "CoastalCampaignProbe.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "CoastalHostSession.h"
#include "CoastalAGISAdapter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalPanelWidget.h"
#include "CoastalWorldObject.h"
#include "CoastalCampaignSave.h"
#include "Core/SaveEnvelope.h"
#include "Kismet/GameplayStatics.h"
#include "FirstSignalComponent.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool FCoastalCampaignProbe::VerifyRejectedSave(UCoastalHostSession* Host,FString& Error)
{
    auto* PC=Cast<APlayerController>(Host->GetOwner());auto* Pawn=PC?PC->GetPawn().Get():nullptr;
    auto* Provider=Pawn?Pawn->FindComponentByClass<UCoastalAGISAdapter>():nullptr;
    auto* Bridge=Pawn?Pawn->FindComponentByClass<UCoastalInteractionBridge>():nullptr;
    auto* Saves=Bridge?Bridge->GetCoordinator():nullptr;
    if(!Saves || !Provider || Saves->HasActiveCampaign())
    {Error=TEXT("Refusal test requires original unstarted host");return false;}
    bool Found=false,Enabled=false;
    for(TObjectIterator<UCoastalPanelWidget> It;It;++It)
        if(It->GetWorld()==Host->GetWorld() && It->IsInViewport() && It->IsVisible() && It->HasBeenPresented() && It->WidgetTree)
            It->WidgetTree->ForEachWidget([&](UWidget* W)
            {if(auto* B=Cast<UCoastalCommandButton>(W))if(B->Command==TEXT("continue")){Found=true;Enabled|=B->GetIsEnabled();}});
    if(!Found || !Enabled){Error=TEXT("Actual presented Continue command is unavailable");return false;}
    TArray<FString> Paths;TArray<TArray<uint8>> Bytes;
    for(const TCHAR* Suffix:{TEXT("_A.sav"),TEXT("_B.sav")})
    {
        const auto Path=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("SaveGames"),TEXT("Coastal_")+Slot+Suffix);
        TArray<uint8> Data;
        if(!FFileHelper::LoadFileToArray(Data,*Path)){Error=TEXT("Missing prepared rejection fixture");return false;}
        Paths.Add(Path);Bytes.Add(MoveTemp(Data));
    }
    if(Mode==TEXT("equal_save") || Mode==TEXT("foreign_save"))
    {
        TArray<FCoastalCampaignSnapshot> Prepared;
        for(const auto& Data:Bytes)
        {
            std::vector<uint8> Payload;
            if(coastal::DecodeSave(Data.GetData(),Data.Num(),Payload)!=coastal::EnvelopeStatus::Valid)
            {Error=TEXT("Conflict fixture must have valid envelope/CRC");return false;}
            TArray<uint8> Native;Native.Append(Payload.data(),int32(Payload.size()));
            auto* Record=Cast<UCoastalCampaignSave>(UGameplayStatics::LoadGameFromMemory(Native));
            if(!Record || Provider->ValidateInventory(Record->Snapshot.Inventory)!=ECoastalProviderResult::Ready)
            {Error=TEXT("Conflict fixture must contain a real valid AGIS snapshot");return false;}
            Prepared.Add(Record->Snapshot);
        }
        const bool Equal=Mode==TEXT("equal_save");
        if((Equal && (Prepared[0].Generation!=Prepared[1].Generation || Prepared[0].CampaignId!=Prepared[1].CampaignId))
            || (!Equal && (Prepared[0].CampaignId==Prepared[1].CampaignId || Prepared[0].Generation==Prepared[1].Generation)))
        {Error=TEXT("Fixture does not isolate requested campaign conflict");return false;}
        UE_LOG(LogTemp,Display,TEXT("COASTAL_CAMPAIGN_CONFLICT_FIXTURE mode=%s generation_a=%lld generation_b=%lld campaign_a=%s campaign_b=%s"),
            *Mode,Prepared[0].Generation,Prepared[1].Generation,*Prepared[0].CampaignId.ToString(),*Prepared[1].CampaignId.ToString());
    }
    FCoastalInventorySnapshot Before,After;
    if(Provider->ExportInventory(Before)!=ECoastalProviderResult::Ready){Error=TEXT("Cannot capture initial real AGIS state");return false;}
    const auto Id=Saves->GetCampaignId();const auto Gen=Saves->GetGeneration();const auto Epoch=Saves->GetSessionEpoch();
    const auto Position=Pawn->GetActorTransform();const auto Journal=Saves->GetJournal();
    TMap<FName,bool> WorldState;
    for(TActorIterator<ACoastalWorldObject> It(Host->GetWorld());It;++It)WorldState.Add(It->WorldId,It->IsActive());
    // The UI intentionally allows a load attempt to report its exact error.
    // Verify the real coordinator boundary against the actual disk fixtures.
    const auto Expected=(Mode==TEXT("future_save") || Mode==TEXT("equal_save"))?ECoastalSaveResult::Incompatible:ECoastalSaveResult::InvalidSnapshot;
    const auto Loaded=Saves->LoadCampaign(FName(Slot));
    if(Mode==TEXT("foreign_save") && !Saves->LastDetail.Contains(TEXT("A/B slots belong to different campaigns")))
    {Error=TEXT("Foreign pair refused for a reason other than campaign identity mismatch");return false;}
    const auto New=Saves->StartNewCampaign(FName(Slot),Position);
    bool Good=Loaded==Expected && New==ECoastalSaveResult::InvalidSnapshot && !Saves->HasActiveCampaign()
        && Saves->GetCampaignId()==Id && Saves->GetGeneration()==Gen && Saves->GetSessionEpoch()==Epoch
        && !Saves->IsBusy() && !Saves->IsRecoveryRequired() && Pawn->GetActorTransform().Equals(Position)
        && Saves->GetJournal()==Journal && Provider->ExportInventory(After)==ECoastalProviderResult::Ready && Before.Payload==After.Payload;
    const auto Mission=Saves->GetMission()->ExportSnapshot();
    Good &= !Mission.bRadioInspected && !Mission.bRadioRepaired && !Mission.bMessageHeard;
    for(TActorIterator<ACoastalWorldObject> It(Host->GetWorld());It;++It)
        Good &= WorldState.Contains(It->WorldId) && WorldState[It->WorldId]==It->IsActive();
    for(int32 I=0;I<Paths.Num();++I)
    {TArray<uint8> Data;Good &= FFileHelper::LoadFileToArray(Data,*Paths[I]) && Data==Bytes[I];}
    if(!Good){Error=TEXT("Invalid/future pair refusal changed disk or live campaign state, or returned the wrong result");return false;}
    UE_LOG(LogTemp,Display,TEXT("COASTAL_SAVE_REJECTION_PASS mode=%s continue_present=1 coordinator_load_refused=1 new_refused=1 disk_and_live_state_unchanged=1"),*Mode);
    return true;
}
#endif
