#include "CoastalAGISAdapter.h"
#include "CoastalContractLibrary.h"

ECoastalInventoryCommit UCoastalAGISAdapter::ReceiptResult(FName Id,const FString& Fingerprint) const
{
    for(const auto& R:Receipts)if(R.TransactionId==Id)
        return R.Fingerprint==Fingerprint ? ECoastalInventoryCommit::AlreadyCommitted : ECoastalInventoryCommit::Failed;
    return ECoastalInventoryCommit::Committed; // absent: caller still must validate and commit
}
ECoastalAvailability UCoastalAGISAdapter::CheckRequirements_Implementation(const TArray<FCoastalItemRequirement>& Requirements) const
{
    if(GetProviderStatus()!=ECoastalProviderResult::Ready)return ECoastalAvailability::NotConfigured;
    FString Fingerprint;
    if(!UCoastalContractLibrary::MakeRequirementsFingerprint(Requirements,Fingerprint))return ECoastalAvailability::Failed;
    TArray<FCoastalAGISItem> Items; if(!ReadLive(Items))return ECoastalAvailability::Failed;
    bool Missing=false;
    for(const auto& R:Requirements)
    {
        const int32 Uid=R.ItemId==ItemId(1)?1:R.ItemId==ItemId(2)?2:0;
        if(!Uid || R.Quantity!=1)return ECoastalAvailability::Failed;
        Missing |= !Items.ContainsByPredicate([&](const auto& I){return I.Uid==Uid && I.Container==1;});
    }
    return Missing ? ECoastalAvailability::MissingItems : ECoastalAvailability::Available;
}
ECoastalInventoryCommit UCoastalAGISAdapter::TryCollectWorldItem_Implementation(FName World,FCoastalItemRequirement Part)
{
    if(GetProviderStatus()!=ECoastalProviderResult::Ready)return ECoastalInventoryCommit::NotConfigured;
    const int32 Uid=UidForWorld(World);
    if(!Uid || Part.ItemId!=ItemId(Uid) || Part.Quantity!=1)return ECoastalInventoryCommit::Failed;
    FString Fingerprint; if(!UCoastalContractLibrary::MakeRequirementsFingerprint({Part},Fingerprint))return ECoastalInventoryCommit::Failed;
    const FName Id(*(TEXT("pickup.")+World.ToString()));
    const auto Existing=ReceiptResult(Id,Fingerprint); if(Existing!=ECoastalInventoryCommit::Committed)return Existing;
    TGuardValue<bool> Lock(Busy,true); TArray<FCoastalAGISItem> Items;
    if(!ReadLive(Items) || Items.ContainsByPredicate([&](const auto& I){return I.Uid==Uid;}))return ECoastalInventoryCommit::Failed;
    FCoastalAGISItem Item; Item.Uid=Uid; Item.Container=1;
    const auto Space=FindSpace(Backpack,Uid,Item.Slot,Item.Rotated);
    if(Space!=ECoastalInventoryCommit::Committed)return Space;
    Items.Add(Item); auto Ledger=Receipts; FCoastalReceipt R; R.TransactionId=Id; R.Fingerprint=Fingerprint; Ledger.Add(R);
    FCoastalInventorySnapshot Candidate;
    return Encode(Campaign,Items,Ledger,Candidate) && Replace(Items,Ledger,Campaign)
        ? ECoastalInventoryCommit::Committed : ECoastalInventoryCommit::Failed;
}
ECoastalInventoryCommit UCoastalAGISAdapter::TryCommitRequirements_Implementation(FName Id,const TArray<FCoastalItemRequirement>& Requirements)
{
    if(GetProviderStatus()!=ECoastalProviderResult::Ready)return ECoastalInventoryCommit::NotConfigured;
    FString Fingerprint; if(!UCoastalContractLibrary::MakeRequirementsFingerprint(Requirements,Fingerprint))return ECoastalInventoryCommit::Failed;
    if(Id!=TEXT("first_signal.radio_repair.v1"))return ECoastalInventoryCommit::Failed;
    const auto Existing=ReceiptResult(Id,Fingerprint); if(Existing!=ECoastalInventoryCommit::Committed)return Existing;
    if(Requirements.Num()!=2 || !Requirements.ContainsByPredicate([](const auto& R){return R.ItemId==ItemId(1)&&R.Quantity==1;})
        || !Requirements.ContainsByPredicate([](const auto& R){return R.ItemId==ItemId(2)&&R.Quantity==1;}))return ECoastalInventoryCommit::Failed;
    const auto Availability=CheckRequirements(Requirements);
    if(Availability!=ECoastalAvailability::Available)return Availability==ECoastalAvailability::MissingItems ? ECoastalInventoryCommit::MissingItems : ECoastalInventoryCommit::Failed;
    TGuardValue<bool> Lock(Busy,true); TArray<FCoastalAGISItem> Items;
    if(!ReadLive(Items))return ECoastalInventoryCommit::Failed;
    Items.RemoveAll([](const auto& I){return I.Uid==1 || I.Uid==2;});
    auto Ledger=Receipts; FCoastalReceipt R; R.TransactionId=Id; R.Fingerprint=Fingerprint; Ledger.Add(R);
    FCoastalInventorySnapshot Candidate;
    return Encode(Campaign,Items,Ledger,Candidate) && Replace(Items,Ledger,Campaign)
        ? ECoastalInventoryCommit::Committed : ECoastalInventoryCommit::Failed;
}
ECoastalInventoryCommit UCoastalAGISAdapter::TryTransfer_Implementation(const FCoastalTransferRequest& R)
{
    if(GetProviderStatus()!=ECoastalProviderResult::Ready)return ECoastalInventoryCommit::NotConfigured;
    const int32 Source=R.SourceContainer==ContainerId(1)?1:R.SourceContainer==ContainerId(2)?2:0;
    const int32 Dest=R.DestinationContainer==ContainerId(1)?1:R.DestinationContainer==ContainerId(2)?2:0;
    int32 Uid=0;
    for(int32 Candidate=1;Candidate<=LastUid;++Candidate)
        if(R.ItemInstanceId==InstanceId(Candidate)){Uid=Candidate;break;}
    if(!R.OperationId.IsValid() || !Source || !Dest || Source==Dest || !Uid || R.Quantity!=1)return ECoastalInventoryCommit::Failed;
    const FName Id(*(TEXT("transfer.")+R.OperationId.ToString(EGuidFormats::Digits).ToLower()));
    const FString Fingerprint=FString::Printf(TEXT("transfer.v1|%s|%s|%s|1"),*R.ItemInstanceId.ToString(EGuidFormats::Digits).ToLower(),*R.SourceContainer.ToString(),*R.DestinationContainer.ToString());
    const auto Existing=ReceiptResult(Id,Fingerprint); if(Existing!=ECoastalInventoryCommit::Committed)return Existing;
    TGuardValue<bool> Lock(Busy,true); TArray<FCoastalAGISItem> Items; if(!ReadLive(Items))return ECoastalInventoryCommit::Failed;
    auto* Item=Items.FindByPredicate([&](const auto& I){return I.Uid==Uid && I.Container==Source;});
    if(!Item)return ECoastalInventoryCommit::MissingItems;
    const auto Space=FindSpace(Dest==1 ? Backpack : Storage,Uid,Item->Slot,Item->Rotated);
    if(Space!=ECoastalInventoryCommit::Committed)return Space;
    Item->Container=Dest;
    auto Ledger=Receipts; FCoastalReceipt Receipt; Receipt.TransactionId=Id; Receipt.Fingerprint=Fingerprint; Ledger.Add(Receipt);
    FCoastalInventorySnapshot Candidate;
    return Encode(Campaign,Items,Ledger,Candidate) && Replace(Items,Ledger,Campaign)
        ? ECoastalInventoryCommit::Committed : ECoastalInventoryCommit::Failed;
}
ECoastalProviderResult UCoastalAGISAdapter::ReadContainerView_Implementation(FName Id,FCoastalContainerView& View) const
{
    View={}; const int32 Index=Id==ContainerId(1)?1:Id==ContainerId(2)?2:0;
    TArray<FCoastalAGISItem> Items;
    if(!Index || GetProviderStatus()!=ECoastalProviderResult::Ready || !ReadLive(Items))return ECoastalProviderResult::Failed;
    View.ContainerId=Id; View.Grid=Grid(Index); View.Revision=Revision;
    for(const auto& Item:Items)if(Item.Container==Index)
    {
        FCoastalInventoryViewItem V; V.InstanceId=InstanceId(Item.Uid); V.ItemId=ItemId(Item.Uid); V.Quantity=1;
        V.DisplayName=FText::FromString(Item.Uid==1?TEXT("Radio Battery"):Item.Uid==2?TEXT("Marine Fuse"):TEXT("Old Postcard"));
        V.Description=FText::FromString(Item.Uid<=2?TEXT("Required for the radio repair."):TEXT("A keepsake; not required for radio repair."));
        V.Position=FIntPoint(Item.Slot%View.Grid.X,Item.Slot/View.Grid.X); V.Size=ItemSize(Item.Uid,Item.Rotated); View.Items.Add(V);
    }
    return View.IsValidFor(Id) ? ECoastalProviderResult::Ready : ECoastalProviderResult::Failed;
}
