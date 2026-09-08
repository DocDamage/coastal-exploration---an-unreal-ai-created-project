#include "CoastalAGISAdapter.h"
#include "CoastalVendorReflection.h"
#include "StructUtils/InstancedStruct.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#endif
using namespace CoastalVendor;

namespace
{
bool Integer(const UStruct* Type,const void* Data,const TCHAR* Name,int32& Out)
{ auto* P=FieldAs<FIntProperty>(Type,Name); if (!P) return false; Out=P->GetPropertyValue_InContainer(Data); return true; }
}
bool UCoastalAGISAdapter::ReadItems(UActorComponent* Inventory,int32 Index,TArray<FCoastalAGISItem>& Items) const
{
    if (!IsValid(Inventory)) return false;
    auto* Containers=FieldAs<FArrayProperty>(InventoryClass,TEXT("Containers"));
    auto* ContainerInner=Containers ? CastField<FStructProperty>(Containers->Inner) : nullptr;
    if (!ContainerInner || ContainerInner->Struct!=ContainerType) return false;
    FScriptArrayHelper List(Containers,Containers->ContainerPtrToValuePtr<void>(Inventory));
    if (List.Num()!=1) return false;
    void* C=List.GetRawPtr(0); int32 CUid;
    auto* Size=FieldAs<FStructProperty>(ContainerType,TEXT("ContainerSize"));
    if (!Integer(ContainerType,C,TEXT("ContainerUID"),CUid) || CUid!=Index || !Size
        || Size->Struct!=TBaseStructure<FIntPoint>::Get() || *Size->ContainerPtrToValuePtr<FIntPoint>(C)!=Grid(Index)) return false;
    auto* Array=FieldAs<FArrayProperty>(ContainerType,TEXT("Items"));
    auto* Inner=Array ? CastField<FStructProperty>(Array->Inner) : nullptr;
    auto* Slots=FieldAs<FArrayProperty>(ContainerType,TEXT("Slots"));
    auto* SlotInner=Slots ? CastField<FStructProperty>(Slots->Inner) : nullptr;
    if (!Inner || Inner->Struct!=ItemType || !SlotInner) return false;
    FScriptArrayHelper ItemList(Array,Array->ContainerPtrToValuePtr<void>(C));
    FScriptArrayHelper SlotList(Slots,Slots->ContainerPtrToValuePtr<void>(C));
    const FIntPoint Dimensions=Grid(Index);
    if (ItemList.Num()>Dimensions.X*Dimensions.Y || SlotList.Num()!=Dimensions.X*Dimensions.Y) return false;
    TArray<int32> Occupied; Occupied.Init(-1,SlotList.Num());
    for (int32 I=0; I<ItemList.Num(); ++I)
    {
        void* V=ItemList.GetRawPtr(I); FCoastalAGISItem Item; Item.Container=Index; int32 Amount;
        auto* Row=FieldAs<FNameProperty>(ItemType,TEXT("ItemID"));
        auto* Address=FieldAs<FStructProperty>(ItemType,TEXT("ItemAddress"));
        auto* Rotated=FieldAs<FBoolProperty>(ItemType,TEXT("Rotated?"));
        auto* Actor=FieldAs<FObjectPropertyBase>(ItemType,TEXT("ItemActor"));
        auto* Data=FieldAs<FStructProperty>(ItemType,TEXT("ItemData"));
        auto* Owned=FieldAs<FArrayProperty>(ItemType,TEXT("OwnContainerUIDs"));
        if (!Integer(ItemType,V,TEXT("ItemUID"),Item.Uid) || ItemId(Item.Uid).IsNone()
            || !Integer(ItemType,V,TEXT("ItemAmount"),Amount) || Amount!=1 || !Row
            || Row->GetPropertyValue_InContainer(V)!=ItemId(Item.Uid) || !Address || !Rotated
            || !Actor || Actor->GetObjectPropertyValue_InContainer(V) || !Data
            || Data->Struct!=FInstancedStruct::StaticStruct() || Data->ContainerPtrToValuePtr<FInstancedStruct>(V)->IsValid()
            || !Owned || FScriptArrayHelper(Owned,Owned->ContainerPtrToValuePtr<void>(V)).Num()!=0) return false;
        void* A=Address->ContainerPtrToValuePtr<void>(V); int32 OwnerUid;
        if (!Integer(Address->Struct,A,TEXT("ContainerUID"),OwnerUid) || OwnerUid!=Index
            || !Integer(Address->Struct,A,TEXT("SlotID"),Item.Slot)) return false;
        Item.Rotated=Rotated->GetPropertyValue_InContainer(V);
        const FIntPoint Extent=ItemSize(Item.Uid,Item.Rotated);
        const int32 X=Item.Slot%Dimensions.X,Y=Item.Slot/Dimensions.X;
        if (Item.Slot<0 || X+Extent.X>Dimensions.X || Y+Extent.Y>Dimensions.Y) return false;
        for (int32 Dy=0; Dy<Extent.Y; ++Dy) for (int32 Dx=0; Dx<Extent.X; ++Dx)
        {
            const int32 S=(Y+Dy)*Dimensions.X+X+Dx;
            if (Occupied[S]!=-1) return false; Occupied[S]=Item.Slot;
        }
        Items.Add(Item);
    }
    for (int32 I=0; I<SlotList.Num(); ++I)
    {
        int32 SlotId,ItemSlot;
        if (!Integer(SlotInner->Struct,SlotList.GetRawPtr(I),TEXT("SlotID"),SlotId) || SlotId!=I
            || !Integer(SlotInner->Struct,SlotList.GetRawPtr(I),TEXT("ItemSlotID"),ItemSlot) || ItemSlot!=Occupied[I]) return false;
    }
    return true;
}
bool UCoastalAGISAdapter::ReadLive(TArray<FCoastalAGISItem>& Items) const
{
    Items.Reset(); if (!ReadItems(Backpack,1,Items) || !ReadItems(Storage,2,Items)) return false;
    Items.Sort([](const auto& A,const auto& B){ return A.Uid<B.Uid; });
    for (int32 I=1; I<Items.Num(); ++I) if (Items[I].Uid==Items[I-1].Uid) return false;
    return true;
}
ECoastalInventoryCommit UCoastalAGISAdapter::FindSpace(UActorComponent* Inventory,int32 Uid,int32& Slot,bool& Rotated) const
{
    Call Find(Inventory,TEXT("Find Space For Item Stack"));
    if (!Find.Name(TEXT("ItemID"),ItemId(Uid)) || !Find.Int(TEXT("Amount"),1)
        || !Find.Bool(TEXT("OnlyAccessibleContainers"),true) || !Find.Run()) return ECoastalInventoryCommit::Failed;
    // Only a real, typed AGIS negative result means no space. Reflection or
    // address failures must not masquerade as an inventory-capacity refusal.
    auto* Found=FieldAs<FBoolProperty>(Find.Function,TEXT("Found"));
    if(!Found)return ECoastalInventoryCommit::Failed;
    if(!Found->GetPropertyValue_InContainer(Find.Data()))return ECoastalInventoryCommit::NoSpace;
    auto* A=FieldAs<FStructProperty>(Find.Function,TEXT("SpaceAddress"));
    auto* R=FieldAs<FBoolProperty>(Find.Function,TEXT("Rotated"));
    int32 Remainder;
    if (!A || !R || !Integer(Find.Function,Find.Data(),TEXT("Remainder"),Remainder) || Remainder!=0
        || !Integer(A->Struct,A->ContainerPtrToValuePtr<void>(Find.Data()),TEXT("SlotID"),Slot)) return ECoastalInventoryCommit::Failed;
    Rotated=R->GetPropertyValue_InContainer(Find.Data()); return ECoastalInventoryCommit::Committed;
}
bool UCoastalAGISAdapter::Insert(UActorComponent* Inventory,const FCoastalAGISItem& Item) const
{
    FStructOnScope Data(ItemType);
    if (!SetInt(ItemType,Data.GetStructMemory(),TEXT("ItemUID"),Item.Uid)
        || !SetInt(ItemType,Data.GetStructMemory(),TEXT("ItemAmount"),1)
        || !SetName(ItemType,Data.GetStructMemory(),TEXT("ItemID"),ItemId(Item.Uid))
        || !SetBool(ItemType,Data.GetStructMemory(),TEXT("Rotated?"),Item.Rotated)) return false;
    Call Place(Inventory,TEXT("Set Item on Container By Address"));
    auto* A=FieldAs<FStructProperty>(Place.Function,TEXT("TargetAddress"));
    if (!A || !SetInt(A->Struct,A->ContainerPtrToValuePtr<void>(Place.Data()),TEXT("ContainerUID"),Item.Container)
        || !SetInt(A->Struct,A->ContainerPtrToValuePtr<void>(Place.Data()),TEXT("SlotID"),Item.Slot)
        || !Place.Struct(TEXT("ItemToSet"),ItemType,Data.GetStructMemory())
        || !Place.Bool(TEXT("RefreshContainers"),true) || !Place.Run() || !Place.Result()) return false;
    return true;
}
bool UCoastalAGISAdapter::Replace(const TArray<FCoastalAGISItem>& Items,const TArray<FCoastalReceipt>& Ledger,FGuid Id)
{
    auto* NewBackpack=MakeInventory(1); auto* NewStorage=MakeInventory(2);
    auto Cleanup=[&]() { if(NewBackpack)NewBackpack->DestroyComponent(); if(NewStorage)NewStorage->DestroyComponent(); };
    if (!NewBackpack || !NewStorage) { Cleanup(); return false; }
    for (const auto& Item : Items)
        if (!Insert(Item.Container==1 ? NewBackpack : NewStorage,Item)) { Cleanup(); return false; }
    TArray<FCoastalAGISItem> Actual;
    if (!ReadItems(NewBackpack,1,Actual) || !ReadItems(NewStorage,2,Actual)) { Cleanup(); return false; }
    Actual.Sort([](const auto& A,const auto& B){return A.Uid<B.Uid;});
    auto Expected=Items; Expected.Sort([](const auto& A,const auto& B){return A.Uid<B.Uid;});
    if (Actual!=Expected) { Cleanup(); return false; }
    TArray<FProperty*> Fields;
    for (const TCHAR* Name : {TEXT("Containers"),TEXT("Containers_Dirty"),TEXT("Cached ItemMap"),TEXT("Cached ContainerMap")})
    { auto* P=Field(InventoryClass,Name); if(!P) { Cleanup(); return false; } Fields.Add(P); }
#if WITH_DEV_AUTOMATION_TESTS
    const FName RepairId=TEXT("first_signal.radio_repair.v1");
    if(FParse::Param(FCommandLine::Get(),TEXT("CoastalFailRepairStaging"))
        && Ledger.ContainsByPredicate([&](const auto& R){return R.TransactionId==RepairId;})
        && !Receipts.ContainsByPredicate([&](const auto& R){return R.TransactionId==RepairId;})
        && IFileManager::Get().FileExists(*(FPaths::ProjectSavedDir()/TEXT("CoastalAcceptance")/(TEXT("coastal_test_repair_stage_")+Id.ToString()+TEXT(".arm")))))
    {
        Cleanup();UE_LOG(LogTemp,Display,TEXT("COASTAL_TEST_REPAIR_STAGING_FAILURE actual_candidate_validated=1 temporary_components_destroyed=1"));return false;
    }
#endif
    // All potentially failing vendor work finished. Native property copies have no
    // Blueprint callbacks; the game-thread operation guard covers the whole swap.
    for (FProperty* P : Fields)
    {
        P->CopyCompleteValue_InContainer(Backpack,NewBackpack);
        P->CopyCompleteValue_InContainer(Storage,NewStorage);
    }
    Receipts=Ledger; Campaign=Id; ++Revision; Cleanup(); return true;
}
