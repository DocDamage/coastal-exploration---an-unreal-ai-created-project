#include "CoastalAGISAdapter.h"
#include "CoastalContractLibrary.h"

namespace
{
const FName Provider(TEXT("coastal.agis.m1"));
const FString Version(TEXT("agis-blueprint-20260906.codec1"));
const FString PostcardVersion(TEXT("agis-blueprint-20260906.codec2"));
FString Fingerprint(std::initializer_list<int32> Uids)
{
    TArray<FCoastalItemRequirement> Parts;
    for (int32 Uid : Uids) { FCoastalItemRequirement P; P.ItemId=UCoastalAGISAdapter::ItemId(Uid); Parts.Add(P); }
    FString Out; UCoastalContractLibrary::MakeRequirementsFingerprint(Parts,Out); return Out;
}
}
bool UCoastalAGISAdapter::Encode(FGuid Id,const TArray<FCoastalAGISItem>& Items,
    const TArray<FCoastalReceipt>& Ledger,FCoastalInventorySnapshot& Snapshot) const
{
    // Keep existing two-part campaigns byte-compatible. A postcard receipt is
    // permanent, so the extended format cannot silently downgrade later.
    const bool Extended=Ledger.ContainsByPredicate([](const auto& R){
        return R.TransactionId.ToString().StartsWith(TEXT("pickup.world.test.postcard."));});
    Snapshot={}; Snapshot.CampaignId=Id; Snapshot.ProviderId=Provider; Snapshot.ProviderVersion=Extended?PostcardVersion:Version;
    Snapshot.Receipts=Ledger;
    // Fixed versioned mapping: UID 1=battery, 2=fuse, 3..74=postcards; grid 1=6x4 backpack,
    // grid 2=8x6 cabin. Each entry carries the real AGIS UID/address/rotation.
    Snapshot.Payload={ 'A','G','I','S',static_cast<uint8>(Extended?2:1),static_cast<uint8>(Items.Num()) };
    auto Sorted=Items; Sorted.Sort([](const auto& A,const auto& B){return A.Uid<B.Uid;});
    for (const auto& I : Sorted)
        Snapshot.Payload.Append({static_cast<uint8>(I.Uid),static_cast<uint8>(I.Container),static_cast<uint8>(I.Slot),static_cast<uint8>(I.Rotated)});
    TArray<FCoastalAGISItem> Recheck; return Decode(Snapshot,Recheck);
}
bool UCoastalAGISAdapter::Decode(const FCoastalInventorySnapshot& S,TArray<FCoastalAGISItem>& Items) const
{
    Items.Reset(); const auto& P=S.Payload;
    const bool Extended=S.ProviderVersion==PostcardVersion;
    const int32 MaxUid=Extended?LastUid:2,MaxItems=Extended?72:2;
    if (S.SchemaVersion!=1 || !S.CampaignId.IsValid() || S.ProviderId!=Provider || (!Extended && S.ProviderVersion!=Version)
        || P.Num()<6 || P.Num()>6+4*MaxItems || P[0]!='A' || P[1]!='G' || P[2]!='I' || P[3]!='S' || P[4]!=(Extended?2:1)
        || P[5]>MaxItems || P.Num()!=6+4*P[5] || S.Receipts.Num()>4096) return false;
    TSet<int32> Uids; TSet<int32> Cells;
    for (int32 N=0; N<P[5]; ++N)
    {
        const int32 Offset=6+4*N;
        FCoastalAGISItem I{P[Offset],P[Offset+1],P[Offset+2],P[Offset+3]!=0};
        if (ItemId(I.Uid).IsNone() || I.Uid>MaxUid || Uids.Contains(I.Uid) || ContainerId(I.Container).IsNone() || P[Offset+3]>1) return false;
        Uids.Add(I.Uid); const auto D=Grid(I.Container),Size=ItemSize(I.Uid,I.Rotated);
        const int32 X=I.Slot%D.X,Y=I.Slot/D.X;
        if (X+Size.X>D.X || Y+Size.Y>D.Y) return false;
        for(int32 Dy=0;Dy<Size.Y;++Dy) for(int32 Dx=0;Dx<Size.X;++Dx)
        { const int32 Cell=I.Container*100+(Y+Dy)*D.X+X+Dx; if(Cells.Contains(Cell))return false; Cells.Add(Cell); }
        Items.Add(I);
    }
    TSet<int32> Collected,Transferred;
    bool Repaired=false,HasPostcard=false; TSet<FName> Transactions;
    TMap<FString,int32> Transfers;
    for(int32 Uid=1;Uid<=MaxUid;++Uid)for(int32 Source:{1,2})
        Transfers.Add(FString::Printf(TEXT("transfer.v1|%s|%s|%s|1"),
            *InstanceFor(S.CampaignId,Uid).ToString(EGuidFormats::Digits).ToLower(),
            *ContainerId(Source).ToString(),*ContainerId(3-Source).ToString()),Uid);
    for (const auto& R:S.Receipts)
    {
        if(R.TransactionId.IsNone() || Transactions.Contains(R.TransactionId) || R.Fingerprint.Len()>512) return false;
        Transactions.Add(R.TransactionId);
        const FString Transaction=R.TransactionId.ToString();
        if(Transaction.StartsWith(TEXT("pickup.")))
        {
            const int32 Uid=UidForWorld(FName(*Transaction.Mid(7)));
            if(!Uid || Uid>MaxUid || R.Fingerprint!=Fingerprint({Uid}))return false;
            Collected.Add(Uid); HasPostcard|=Uid>=3;
        }
        else if(R.TransactionId==TEXT("first_signal.radio_repair.v1"))
        { if(R.Fingerprint!=Fingerprint({1,2}))return false; Repaired=true; }
        else
        {
            const FString Id=R.TransactionId.ToString(); FGuid Operation;
            if(!Id.StartsWith(TEXT("transfer.")) || Id.Len()!=41 || !FGuid::ParseExact(Id.Mid(9),EGuidFormats::Digits,Operation)
                || !Operation.IsValid() || Id!=TEXT("transfer.")+Operation.ToString(EGuidFormats::Digits).ToLower())return false;
            const int32* Uid=Transfers.Find(R.Fingerprint);
            if(!Uid)return false;
            Transferred.Add(*Uid);
        }
    }
    if(HasPostcard!=Extended || (Repaired && (!Collected.Contains(1) || !Collected.Contains(2))))return false;
    for(int32 Uid=1;Uid<=MaxUid;++Uid)
        if((Transferred.Contains(Uid) && !Collected.Contains(Uid))
            || Uids.Contains(Uid)!=(Collected.Contains(Uid) && !(Uid<=2 && Repaired)))return false;
    Items.Sort([](const auto& A,const auto& B){return A.Uid<B.Uid;}); return true;
}
ECoastalProviderResult UCoastalAGISAdapter::ExportInventory_Implementation(FCoastalInventorySnapshot& S) const
{
    S={}; TArray<FCoastalAGISItem> Items;
    return GetProviderStatus()==ECoastalProviderResult::Ready && ReadLive(Items) && Encode(Campaign,Items,Receipts,S)
        ? ECoastalProviderResult::Ready : ECoastalProviderResult::Failed;
}
ECoastalProviderResult UCoastalAGISAdapter::ValidateInventory_Implementation(const FCoastalInventorySnapshot& S) const
{
    if(GetProviderStatus()!=ECoastalProviderResult::Ready)return ECoastalProviderResult::NotConfigured;
    // Preserve future/foreign saves as incompatible, rather than permitting the
    // coordinator to classify them as corrupt and reuse their disk slot.
    const int32 Format=S.ProviderVersion==Version?1:S.ProviderVersion==PostcardVersion?2:0;
    if(S.SchemaVersion!=1 || S.ProviderId!=Provider || !Format
        || (S.Payload.Num()>=5 && S.Payload[0]=='A' && S.Payload[1]=='G'
            && S.Payload[2]=='I' && S.Payload[3]=='S' && S.Payload[4]!=Format))
        return ECoastalProviderResult::Incompatible;
    TArray<FCoastalAGISItem> Items;
    return Decode(S,Items) ? ECoastalProviderResult::Ready : ECoastalProviderResult::Failed;
}
ECoastalProviderResult UCoastalAGISAdapter::BuildNewInventory_Implementation(FGuid Id,FCoastalInventorySnapshot& S) const
{
    return GetProviderStatus()==ECoastalProviderResult::Ready && Encode(Id,{}, {},S)
        ? ECoastalProviderResult::Ready : ECoastalProviderResult::Failed;
}
ECoastalProviderResult UCoastalAGISAdapter::RestoreInventory_Implementation(const FCoastalInventorySnapshot& S)
{
    const auto Validation=ValidateInventory(S);
    if(Validation!=ECoastalProviderResult::Ready)return Validation;
    TGuardValue<bool> Lock(Busy,true); TArray<FCoastalAGISItem> Items;
    return Decode(S,Items) && Replace(Items,S.Receipts,S.CampaignId) ? ECoastalProviderResult::Ready : ECoastalProviderResult::Failed;
}
