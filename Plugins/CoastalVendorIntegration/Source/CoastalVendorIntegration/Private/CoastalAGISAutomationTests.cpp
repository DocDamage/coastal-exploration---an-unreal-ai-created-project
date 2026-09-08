#include "CoastalAGISAdapter.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "CoastalIntegrationLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalRealAGISTest,"Coastal.Vendor.AGIS.Transactions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalRealAGISTest::RunTest(const FString& Parameters)
{
    const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
        .RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    AActor* Owner=World->SpawnActor<AActor>();
    auto* Provider=NewObject<UCoastalAGISAdapter>(Owner); Provider->RegisterComponent(); Owner->DispatchBeginPlay();
    if(!TestTrue(TEXT("Real AGIS provider initializes"),Provider->InitializeRealProvider()))
    { Provider->DestroyComponent(); World->DestroyWorld(false); return false; }
    FCoastalInventorySnapshot Empty;
    TestEqual(TEXT("Export pristine vendor grids"),Provider->ExportInventory(Empty),ECoastalProviderResult::Ready);
    FCoastalIntegrationReport Report;
    TestTrue(TEXT("Actual provider passes startup audit"),UCoastalIntegrationLibrary::AuditProvisionalProvider(Provider,Report));
    FCoastalItemRequirement Battery; Battery.ItemId=TEXT("item.radio_battery");
    FCoastalItemRequirement Fuse; Fuse.ItemId=TEXT("item.marine_fuse");
    const TArray<FCoastalItemRequirement> Parts{Battery,Fuse};
    TestEqual(TEXT("Missing parts rejects repair"),Provider->TryCommitRequirements(TEXT("first_signal.radio_repair.v1"),Parts),ECoastalInventoryCommit::MissingItems);
    TestEqual(TEXT("AGIS inserts battery"),Provider->TryCollectWorldItem(TEXT("world.test.battery"),Battery),ECoastalInventoryCommit::Committed);
    TestEqual(TEXT("Pickup retry is idempotent"),Provider->TryCollectWorldItem(TEXT("world.test.battery"),Battery),ECoastalInventoryCommit::AlreadyCommitted);
    TestEqual(TEXT("Wrong source item rejected"),Provider->TryCollectWorldItem(TEXT("world.test.battery"),Fuse),ECoastalInventoryCommit::Failed);
    FCoastalContainerView View; Provider->ReadContainerView(TEXT("container.player"),View);
    if(!TestEqual(TEXT("One real item after retry"),View.Items.Num(),1)) { Provider->DestroyComponent(); World->DestroyWorld(false); return false; }
    TestEqual(TEXT("Battery occupies 1x2 cells"),View.Items[0].Size,FIntPoint(1,2));
    FCoastalTransferRequest Move; Move.OperationId=FGuid::NewGuid(); Move.ItemInstanceId=View.Items[0].InstanceId;
    Move.SourceContainer=TEXT("container.player"); Move.DestinationContainer=TEXT("world.test.storage");
    TestEqual(TEXT("Transfer to actual cabin grid"),Provider->TryTransfer(Move),ECoastalInventoryCommit::Committed);
    TestEqual(TEXT("Transfer retry"),Provider->TryTransfer(Move),ECoastalInventoryCommit::AlreadyCommitted);
    FCoastalInventorySnapshot Partial; TestEqual(TEXT("Partial export"),Provider->ExportInventory(Partial),ECoastalProviderResult::Ready);
    auto Orphan=Empty; Orphan.Receipts.Add(Partial.Receipts.Last());
    TestEqual(TEXT("Transfer without prior pickup rejected"),Provider->ValidateInventory(Orphan),ECoastalProviderResult::Failed);
    TestEqual(TEXT("Stored battery does not satisfy carried requirement"),Provider->CheckRequirements({Battery}),ECoastalAvailability::MissingItems);
    auto Collision=Move; Swap(Collision.SourceContainer,Collision.DestinationContainer);
    TestEqual(TEXT("Same operation with different request fails"),Provider->TryTransfer(Collision),ECoastalInventoryCommit::Failed);
    TestEqual(TEXT("AGIS inserts fuse"),Provider->TryCollectWorldItem(TEXT("world.test.fuse"),Fuse),ECoastalInventoryCommit::Committed);
    TestEqual(TEXT("Repair with stored part consumes nothing"),Provider->TryCommitRequirements(TEXT("first_signal.radio_repair.v1"),Parts),ECoastalInventoryCommit::MissingItems);
    TestEqual(TEXT("Restore stored-part snapshot"),Provider->RestoreInventory(Partial),ECoastalProviderResult::Ready);
    Provider->ReadContainerView(TEXT("container.player"),View); TestEqual(TEXT("Restore removed later fuse"),View.Items.Num(),0);
    auto Malformed=Partial; Malformed.Payload.Add(255);
    TestEqual(TEXT("Malformed payload rejected"),Provider->RestoreInventory(Malformed),ECoastalProviderResult::Failed);
    auto Future=Partial; Future.ProviderVersion=TEXT("future-codec");
    TestEqual(TEXT("Future codec is incompatible, not corrupt"),Provider->ValidateInventory(Future),ECoastalProviderResult::Incompatible);
    TestEqual(TEXT("Future restore cannot replace live state"),Provider->RestoreInventory(Future),ECoastalProviderResult::Incompatible);
    auto Foreign=Partial; Foreign.ProviderId=TEXT("another.provider");
    TestEqual(TEXT("Foreign provider preserved as incompatible"),Provider->ValidateInventory(Foreign),ECoastalProviderResult::Incompatible);
    auto Duplicated=Partial; Duplicated.Payload[5]=2; Duplicated.Payload.Append({1,1,0,0});
    TestEqual(TEXT("Duplicated critical instance rejected"),Provider->ValidateInventory(Duplicated),ECoastalProviderResult::Failed);
    auto Lost=Partial; Lost.Payload.SetNum(6); Lost.Payload[5]=0;
    TestEqual(TEXT("Lost critical item rejected"),Provider->ValidateInventory(Lost),ECoastalProviderResult::Failed);
    FCoastalInventorySnapshot AfterFailure; Provider->ExportInventory(AfterFailure);
    TestTrue(TEXT("Failed restore preserves payload"),AfterFailure.Payload==Partial.Payload);
    TestEqual(TEXT("Failed restore preserves receipt count"),AfterFailure.Receipts.Num(),Partial.Receipts.Num());
    Collision.OperationId=FGuid::NewGuid();
    TestEqual(TEXT("Return battery preserves UID"),Provider->TryTransfer(Collision),ECoastalInventoryCommit::Committed);
    TestEqual(TEXT("Recollect fuse after restore"),Provider->TryCollectWorldItem(TEXT("world.test.fuse"),Fuse),ECoastalInventoryCommit::Committed);
    if(FParse::Param(FCommandLine::Get(),TEXT("CoastalFailRepairStaging")))
    {
        FCoastalInventorySnapshot BeforeRepair,AfterRepair;
        TestEqual(TEXT("Capture before staged repair failure"),Provider->ExportInventory(BeforeRepair),ECoastalProviderResult::Ready);
        FCoastalContainerView BeforeBag,AfterBag;Provider->ReadContainerView(TEXT("container.player"),BeforeBag);
        TArray<UActorComponent*> Components;Owner->GetComponents(Components);const int32 ComponentCount=Components.Num();
        const FString Arm=FPaths::ProjectSavedDir()/TEXT("CoastalAcceptance")/(TEXT("coastal_test_repair_stage_")+BeforeRepair.CampaignId.ToString()+TEXT(".arm"));
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Arm),true);
        if(!TestTrue(TEXT("Create exclusive repair failure marker"),!IFileManager::Get().FileExists(*Arm)
            && FFileHelper::SaveStringToFile(TEXT("explicit native AGIS acceptance fixture"),*Arm)))
        {Provider->DestroyComponent();World->DestroyWorld(false);return false;}
        const auto FailedRepair=Provider->TryCommitRequirements(TEXT("first_signal.radio_repair.v1"),Parts);
        TestTrue(TEXT("Remove repair failure marker"),IFileManager::Get().Delete(*Arm,true,false,true));
        TestEqual(TEXT("Staged actual repair failure is explicit"),FailedRepair,ECoastalInventoryCommit::Failed);
        TestEqual(TEXT("Provider guard released after repair failure"),Provider->GetProviderStatus(),ECoastalProviderResult::Ready);
        TestEqual(TEXT("Export after staged repair failure"),Provider->ExportInventory(AfterRepair),ECoastalProviderResult::Ready);
        TestTrue(TEXT("Staged failure preserves complete inventory"),AfterRepair.Payload==BeforeRepair.Payload);
        TestEqual(TEXT("Staged failure preserves campaign"),AfterRepair.CampaignId,BeforeRepair.CampaignId);
        TestEqual(TEXT("Staged failure preserves receipt count"),AfterRepair.Receipts.Num(),BeforeRepair.Receipts.Num());
        for(int32 I=0;I<FMath::Min(AfterRepair.Receipts.Num(),BeforeRepair.Receipts.Num());++I)
        {
            TestEqual(TEXT("Staged failure preserves receipt ID"),AfterRepair.Receipts[I].TransactionId,BeforeRepair.Receipts[I].TransactionId);
            TestEqual(TEXT("Staged failure preserves receipt fingerprint"),AfterRepair.Receipts[I].Fingerprint,BeforeRepair.Receipts[I].Fingerprint);
        }
        Provider->ReadContainerView(TEXT("container.player"),AfterBag);
        TestEqual(TEXT("Staged failure preserves inventory revision"),AfterBag.Revision,BeforeBag.Revision);
        TestEqual(TEXT("Both repair parts remain available"),Provider->CheckRequirements(Parts),ECoastalAvailability::Available);
        Components.Reset();Owner->GetComponents(Components);
        TestEqual(TEXT("Staged components released"),Components.Num(),ComponentCount);
    }
    TestEqual(TEXT("Atomic radio consumption"),Provider->TryCommitRequirements(TEXT("first_signal.radio_repair.v1"),Parts),ECoastalInventoryCommit::Committed);
    TestEqual(TEXT("Repair retry after consumption"),Provider->TryCommitRequirements(TEXT("first_signal.radio_repair.v1"),Parts),ECoastalInventoryCommit::AlreadyCommitted);
    FCoastalInventorySnapshot Repaired;
    TestEqual(TEXT("Capture committed repair before conflicting requests"),Provider->ExportInventory(Repaired),ECoastalProviderResult::Ready);
    auto ConflictingParts=Parts; ConflictingParts[0].Quantity=2;
    TestEqual(TEXT("Same committed repair ID with different quantity fails"),Provider->TryCommitRequirements(TEXT("first_signal.radio_repair.v1"),ConflictingParts),ECoastalInventoryCommit::Failed);
    TestEqual(TEXT("Same committed repair ID with omitted requirement fails"),Provider->TryCommitRequirements(TEXT("first_signal.radio_repair.v1"),{Battery}),ECoastalInventoryCommit::Failed);
    auto ReorderedParts=Parts; Swap(ReorderedParts[0],ReorderedParts[1]);
    TestEqual(TEXT("Equivalent reordered repair remains already committed"),Provider->TryCommitRequirements(TEXT("first_signal.radio_repair.v1"),ReorderedParts),ECoastalInventoryCommit::AlreadyCommitted);
    FCoastalInventorySnapshot AfterConflicts;
    TestEqual(TEXT("Export after conflicting repair requests"),Provider->ExportInventory(AfterConflicts),ECoastalProviderResult::Ready);
    TestTrue(TEXT("Repair conflicts preserve complete AGIS payload"),AfterConflicts.Payload==Repaired.Payload);
    TestEqual(TEXT("Repair conflicts preserve campaign identity"),AfterConflicts.CampaignId,Repaired.CampaignId);
    TestEqual(TEXT("Repair conflicts preserve receipt count"),AfterConflicts.Receipts.Num(),Repaired.Receipts.Num());
    for(int32 I=0;I<FMath::Min(AfterConflicts.Receipts.Num(),Repaired.Receipts.Num());++I)
    {
        TestEqual(TEXT("Repair conflicts preserve transaction IDs"),AfterConflicts.Receipts[I].TransactionId,Repaired.Receipts[I].TransactionId);
        TestEqual(TEXT("Repair conflicts preserve fingerprints"),AfterConflicts.Receipts[I].Fingerprint,Repaired.Receipts[I].Fingerprint);
    }
    TestEqual(TEXT("Restore repaired snapshot"),Provider->RestoreInventory(Repaired),ECoastalProviderResult::Ready);
    Provider->ReadContainerView(TEXT("container.player"),View); TestEqual(TEXT("Repaired backpack empty"),View.Items.Num(),0);
    FCoastalInventorySnapshot New; TestEqual(TEXT("New campaign built read-only"),Provider->BuildNewInventory(FGuid::NewGuid(),New),ECoastalProviderResult::Ready);
    TestEqual(TEXT("Other campaign validates"),Provider->ValidateInventory(New),ECoastalProviderResult::Ready);
    FCoastalInventorySnapshot StillRepaired; Provider->ExportInventory(StillRepaired);
    TestEqual(TEXT("BuildNew preserves current campaign"),StillRepaired.CampaignId,Repaired.CampaignId);
    TestEqual(TEXT("Restore original pristine snapshot"),Provider->RestoreInventory(Empty),ECoastalProviderResult::Ready);
    Provider->DestroyComponent(); World->DestroyWorld(false); return true;
}
#endif
