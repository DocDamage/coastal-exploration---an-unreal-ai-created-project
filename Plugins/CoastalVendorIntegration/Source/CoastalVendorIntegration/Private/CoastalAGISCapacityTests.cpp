#include "CoastalAGISAdapter.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// Yield between bounded batches so independent inventory operations do not
// accumulate into one Blueprint runaway-loop budget for the entire fixture.
class FCoastalAGISCapacitySteps : public IAutomationLatentCommand
{
public:
    explicit FCoastalAGISCapacitySteps(FAutomationTestBase* InTest):Test(InTest){}
    virtual ~FCoastalAGISCapacitySteps(){Cleanup();}
    bool Update() override
    {
        auto TestEqual=[this](const TCHAR* Label,const auto& Actual,const auto& Expected){return Test->TestEqual(Label,Actual,Expected);};
        auto TestTrue=[this](const TCHAR* Label,bool Value){return Test->TestTrue(Label,Value);};
        if(Step==0)
        {
    const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
        .RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
    AActor* Owner=World->SpawnActor<AActor>();
    Provider=NewObject<UCoastalAGISAdapter>(Owner); Owner->AddInstanceComponent(Provider); Provider->RegisterComponent(); Owner->DispatchBeginPlay();
    if(!TestTrue(TEXT("Actual AGIS capacity provider initializes"),Provider->InitializeRealProvider())){Cleanup();return true;}
    Provider->ExportInventory(Empty);
            ++Step;return false;
        }
    auto Collect=[&](int32 Uid){FCoastalItemRequirement Item;Item.ItemId=Provider->ItemId(Uid);
        return Provider->TryCollectWorldItem(Provider->WorldIdFor(Uid),Item);};
    auto Move=[&](int32 Uid,int32 Source){FCoastalTransferRequest R;R.OperationId=FGuid::NewGuid();
        R.ItemInstanceId=Provider->InstanceId(Uid);R.SourceContainer=Provider->ContainerId(Source);
        R.DestinationContainer=Provider->ContainerId(3-Source);return Provider->TryTransfer(R);};
    auto Snapshot=[&](){FCoastalInventorySnapshot S;TestEqual(TEXT("Export actual AGIS state"),Provider->ExportInventory(S),ECoastalProviderResult::Ready);return S;};
    auto Unchanged=[&](const FCoastalInventorySnapshot& Before){const auto After=Snapshot();
        TestTrue(TEXT("Rejected operation preserves complete payload"),Before.Payload==After.Payload);
        TestEqual(TEXT("Rejected operation preserves campaign"),After.CampaignId,Before.CampaignId);
        TestEqual(TEXT("Rejected operation preserves codec"),After.ProviderVersion,Before.ProviderVersion);
        TestEqual(TEXT("Rejected operation preserves receipt count"),After.Receipts.Num(),Before.Receipts.Num());
        for(int32 I=0;I<FMath::Min(Before.Receipts.Num(),After.Receipts.Num());++I)
        {TestEqual(TEXT("Receipt ID preserved"),After.Receipts[I].TransactionId,Before.Receipts[I].TransactionId);
         TestEqual(TEXT("Receipt fingerprint preserved"),After.Receipts[I].Fingerprint,Before.Receipts[I].Fingerprint);}};
        if(Step>=1 && Step<=24)
        {
            if(!TestEqual(TEXT("Collect distinct postcard into actual backpack"),Collect(Step+2),ECoastalInventoryCommit::Committed)){Cleanup();return true;}
        }
        else if(Step==25)
        {
    Provider->ReadContainerView(Provider->ContainerId(1),Bag);
    TestEqual(TEXT("Authored backpack remains 6x4"),Bag.Grid,FIntPoint(6,4));
    TestEqual(TEXT("Backpack has 24 separate real items"),Bag.Items.Num(),24);
    TSet<FGuid> Instances;for(const auto& I:Bag.Items)Instances.Add(I.InstanceId);
    TestEqual(TEXT("Postcards have distinct instance identities"),Instances.Num(),24);
    FullBag=Snapshot();const int64 BagRevision=Bag.Revision;
    TestEqual(TEXT("Extended codec used only with postcard receipts"),FullBag.ProviderVersion,FString(TEXT("agis-blueprint-20260906.codec2")));
    TestEqual(TEXT("Full backpack refuses battery"),Collect(1),ECoastalInventoryCommit::NoSpace);
    TestEqual(TEXT("Full backpack refuses fuse"),Collect(2),ECoastalInventoryCommit::NoSpace);
    TestEqual(TEXT("Full backpack refuses another postcard"),Collect(27),ECoastalInventoryCommit::NoSpace);
    Unchanged(FullBag);Provider->ReadContainerView(Provider->ContainerId(1),Bag);
    TestEqual(TEXT("Capacity refusal does not advance revision"),Bag.Revision,BagRevision);
    TestEqual(TEXT("Existing postcard retry stays idempotent when full"),Collect(3),ECoastalInventoryCommit::AlreadyCommitted);
        }
        else if(Step>=26 && Step<=28)
            TestEqual(TEXT("Free space by real storage transfer"),Move(Step-23,1),ECoastalInventoryCommit::Committed);
        else if(Step==29)
        {
    TestEqual(TEXT("Battery succeeds after freeing cells"),Collect(1),ECoastalInventoryCommit::Committed);
    TestEqual(TEXT("Fuse succeeds after freeing cells"),Collect(2),ECoastalInventoryCommit::Committed);
    FCoastalItemRequirement Battery;Battery.ItemId=Provider->ItemId(1);
    FCoastalItemRequirement Fuse;Fuse.ItemId=Provider->ItemId(2);
    TestEqual(TEXT("Repair consumes only the critical parts"),Provider->TryCommitRequirements(TEXT("first_signal.radio_repair.v1"),{Battery,Fuse}),ECoastalInventoryCommit::Committed);
    Provider->ReadContainerView(Provider->ContainerId(1),Bag);Provider->ReadContainerView(Provider->ContainerId(2),Store);
    TestEqual(TEXT("Repair retains all 24 postcards"),Bag.Items.Num()+Store.Items.Num(),24);
    TestEqual(TEXT("Restore full postcard snapshot"),Provider->RestoreInventory(FullBag),ECoastalProviderResult::Ready);Unchanged(FullBag);
    auto Lost=FullBag;Lost.Payload.SetNum(Lost.Payload.Num()-4);--Lost.Payload[5];
    TestEqual(TEXT("Lost postcard rejected"),Provider->ValidateInventory(Lost),ECoastalProviderResult::Failed);
    auto Duplicate=FullBag;Duplicate.Payload[10]=Duplicate.Payload[6];
    TestEqual(TEXT("Duplicate postcard instance rejected"),Provider->ValidateInventory(Duplicate),ECoastalProviderResult::Failed);
    auto MissingReceipt=FullBag;MissingReceipt.Receipts.RemoveAt(0);
    TestEqual(TEXT("Postcard without pickup receipt rejected"),Provider->ValidateInventory(MissingReceipt),ECoastalProviderResult::Failed);
    TestEqual(TEXT("Restore original codec1 snapshot"),Provider->RestoreInventory(Empty),ECoastalProviderResult::Ready);Unchanged(Empty);
        }
        else if(Step>=30 && Step<=125)
        {
            const int32 Uid=3+(Step-30)/2;
            const auto Result=(Step%2==0)?Collect(Uid):Move(Uid,1);
            if(!TestEqual(TEXT("Fill real cabin one operation per frame"),Result,ECoastalInventoryCommit::Committed)){Cleanup();return true;}
        }
        else if(Step==126)
        {
    Provider->ReadContainerView(Provider->ContainerId(2),Store);
    TestEqual(TEXT("Authored cabin remains 8x6"),Store.Grid,FIntPoint(8,6));
    TestEqual(TEXT("Cabin has 48 separate items"),Store.Items.Num(),48);
    TestEqual(TEXT("Collect battery with cabin full"),Collect(1),ECoastalInventoryCommit::Committed);
    FullStore=Snapshot();
    TestEqual(TEXT("Real full cabin refuses transfer"),Move(1,1),ECoastalInventoryCommit::NoSpace);Unchanged(FullStore);
    TestEqual(TEXT("Free first cabin cell"),Move(3,2),ECoastalInventoryCommit::Committed);
    OneCell=Snapshot();
    TestEqual(TEXT("One cell cannot fit the two-cell battery"),Move(1,1),ECoastalInventoryCommit::NoSpace);Unchanged(OneCell);
        }
        else if(Step==127)
        {
    TestEqual(TEXT("Free adjacent cabin cell"),Move(4,2),ECoastalInventoryCommit::Committed);
    TestEqual(TEXT("Battery transfer succeeds with real space"),Move(1,1),ECoastalInventoryCommit::Committed);
        }
        else if(Step==128)
        {
    TestEqual(TEXT("Full cabin snapshot restores"),Provider->RestoreInventory(FullStore),ECoastalProviderResult::Ready);Unchanged(FullStore);
    auto Future=FullStore;Future.ProviderVersion=TEXT("agis-blueprint-20260906.codec3");Future.Payload[4]=3;
    TestEqual(TEXT("Future extended format stays incompatible"),Provider->RestoreInventory(Future),ECoastalProviderResult::Incompatible);Unchanged(FullStore);
            Cleanup();return true;
        }
        ++Step;return false;
    }
private:
    void Cleanup(){if(Provider){Provider->DestroyComponent();Provider=nullptr;}if(World){World->DestroyWorld(false);World=nullptr;}}
    FAutomationTestBase* Test;
    UWorld* World=nullptr;
    UCoastalAGISAdapter* Provider=nullptr;
    FCoastalInventorySnapshot Empty,FullBag,FullStore,OneCell;
    FCoastalContainerView Bag,Store;
    int32 Step=0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalRealAGISCapacityTest,"Coastal.Vendor.AGIS.Capacity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalRealAGISCapacityTest::RunTest(const FString& Parameters)
{
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCoastalAGISCapacitySteps>(this));
    return true;
}
#endif
