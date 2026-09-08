#include "CoastalAGISAdapter.h"
#include "CoastalVendorReflection.h"
#include "Engine/CompositeDataTable.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
using namespace CoastalVendor;

FName UCoastalAGISAdapter::ItemId(int32 Uid)
{ return Uid == 1 ? FName(TEXT("item.radio_battery")) : Uid == 2 ? FName(TEXT("item.marine_fuse"))
    : Uid>=3 && Uid<=LastUid ? FName(TEXT("item.old_postcard")) : NAME_None; }
FName UCoastalAGISAdapter::WorldIdFor(int32 Uid)
{ return Uid==1?FName(TEXT("world.test.battery")):Uid==2?FName(TEXT("world.test.fuse"))
    : Uid>=3 && Uid<=LastUid?FName(*FString::Printf(TEXT("world.test.postcard.%03d"),Uid-2)):NAME_None; }
int32 UCoastalAGISAdapter::UidForWorld(FName World)
{ for(int32 Uid=1;Uid<=LastUid;++Uid)if(World==WorldIdFor(Uid))return Uid;return 0; }
FName UCoastalAGISAdapter::ContainerId(int32 Index)
{ return Index == 1 ? FName(TEXT("container.player")) : Index == 2 ? FName(TEXT("world.test.storage")) : NAME_None; }
FIntPoint UCoastalAGISAdapter::Grid(int32 Index) { return Index == 1 ? FIntPoint(6,4) : FIntPoint(8,6); }
FIntPoint UCoastalAGISAdapter::ItemSize(int32 Uid,bool Rotated)
{ return Uid == 1 ? (Rotated ? FIntPoint(2,1) : FIntPoint(1,2)) : FIntPoint(1,1); }
FGuid UCoastalAGISAdapter::InstanceId(int32 Uid) const
{ return InstanceFor(Campaign,Uid); }
FGuid UCoastalAGISAdapter::InstanceFor(FGuid Id,int32 Uid)
{ return FGuid::NewDeterministicGuid(Id.ToString(EGuidFormats::Digits)+TEXT(":coastal.agis:")+FString::FromInt(Uid)); }

bool UCoastalAGISAdapter::RegisterDefinitions()
{
    VendorDefinitions=LoadObject<UCompositeDataTable>(nullptr,TEXT("/Game/INVENTORY/Items/CDT_Items.CDT_Items"));
    if (!VendorDefinitions || !VendorDefinitions->GetRowStruct()) return false;
    auto* Row=VendorDefinitions->RowStruct.Get();
    for (int32 Uid : {1,2,3}) if (VendorDefinitions->GetRowMap().Contains(ItemId(Uid))) return false;
    const FGameplayTag General=FGameplayTag::RequestGameplayTag(TEXT("Slots.GeneralInventorySlot"),false);
    if (!General.IsValid()) return false;
    CoastalDefinitions=NewObject<UDataTable>(this); CoastalDefinitions->RowStruct=Row;
    for (int32 Uid : {1,2,3})
    {
        FStructOnScope Data(Row);
        auto* Name=FieldAs<FTextProperty>(Row,TEXT("Name"));
        auto* Description=FieldAs<FTextProperty>(Row,TEXT("Description"));
        auto* Size=FieldAs<FStructProperty>(Row,TEXT("ItemSize"));
        auto* Tags=FieldAs<FStructProperty>(Row,TEXT("DroppableSlotTypes"));
        if (!Name || !Description || !Size || Size->Struct!=TBaseStructure<FIntPoint>::Get()
            || !Tags || Tags->Struct!=FGameplayTagContainer::StaticStruct()) return false;
        Name->SetPropertyValue_InContainer(Data.GetStructMemory(),FText::FromString(Uid==1 ? TEXT("Radio Battery") : Uid==2 ? TEXT("Marine Fuse") : TEXT("Old Postcard")));
        Description->SetPropertyValue_InContainer(Data.GetStructMemory(),FText::FromString(Uid<=2
            ? TEXT("Required radio equipment. Can be stored; consumed only by radio repair.") : TEXT("A keepsake. Can be stored; not consumed by radio repair.")));
        *Size->ContainerPtrToValuePtr<FIntPoint>(Data.GetStructMemory())=ItemSize(Uid,false);
        Tags->ContainerPtrToValuePtr<FGameplayTagContainer>(Data.GetStructMemory())->AddTag(General);
        if (!SetBool(Row,Data.GetStructMemory(),TEXT("Stackable"),false)
            || !SetInt(Row,Data.GetStructMemory(),TEXT("MaxStackSize"),1)) return false;
        // No item actor, attachments, use/drop menu actions, item-data payload or own grids.
        for (const TCHAR* ArrayName : {TEXT("MenuActions"),TEXT("AttachmentSlots")})
        {
            auto* P=FieldAs<FArrayProperty>(Row,ArrayName); if (!P) return false;
            FScriptArrayHelper(P,P->ContainerPtrToValuePtr<void>(Data.GetStructMemory())).EmptyValues();
        }
        CoastalDefinitions->AddRow(ItemId(Uid),Data.GetStructMemory(),Row);
    }
    VendorDefinitions->AddParentTable(CoastalDefinitions);
    for (int32 Uid : {1,2,3})
    {
        Call Query(Library,TEXT("Get Item Defaults By ID"));
        if (!Query.Name(TEXT("RowName"),ItemId(Uid)) || !Query.Object(TEXT("__WorldContext"),this)
            || !Query.Run() || !Query.Result(TEXT("Found"))) return false;
    }
    return true;
}

UActorComponent* UCoastalAGISAdapter::MakeInventory(int32 Index) const
{
    if (!InventoryClass || !ContainerType || !GetOwner()) return nullptr;
    auto* Result=NewObject<UActorComponent>(GetOwner(),InventoryClass,NAME_None,RF_Transient);
    auto Fail=[&]() -> UActorComponent* { Result->DestroyComponent(); return nullptr; };
    if (!SetBool(InventoryClass,Result,TEXT("Auto Init Component"),false)
        || !SetBool(InventoryClass,Result,TEXT("Print Debug Texts"),false)) return Fail();
    Result->SetIsReplicated(false); Result->RegisterComponent(); Result->SetComponentTickEnabled(false);
    FStructOnScope Container(ContainerType);
    auto* Size=FieldAs<FStructProperty>(ContainerType,TEXT("ContainerSize"));
    auto* Tag=FieldAs<FStructProperty>(ContainerType,TEXT("ContainerType"));
    if (!Size || Size->Struct!=TBaseStructure<FIntPoint>::Get() || !Tag || Tag->Struct!=FGameplayTag::StaticStruct()) return Fail();
    *Size->ContainerPtrToValuePtr<FIntPoint>(Container.GetStructMemory())=Grid(Index);
    *Tag->ContainerPtrToValuePtr<FGameplayTag>(Container.GetStructMemory())=FGameplayTag::RequestGameplayTag(TEXT("Slots.GeneralInventorySlot"),false);
    if (!SetInt(ContainerType,Container.GetStructMemory(),TEXT("ContainerUID"),Index)
        || !SetName(ContainerType,Container.GetStructMemory(),TEXT("Tag"),ContainerId(Index))
        || !SetBool(ContainerType,Container.GetStructMemory(),TEXT("IsAccessible"),true)
        || !SetBool(ContainerType,Container.GetStructMemory(),TEXT("MakesContentsAccessible"),true)
        || !SetBool(ContainerType,Container.GetStructMemory(),TEXT("IsBlocked"),false)) return Fail();
    Call Slots(Library,TEXT("Create Slots For Container"));
    if (!Slots.Struct(TEXT("Container"),ContainerType,Container.GetStructMemory())
        || !Slots.Object(TEXT("__WorldContext"),Result) || !Slots.Run()) return Fail();
    auto* NewContainer=FieldAs<FStructProperty>(Slots.Function,TEXT("NewContainer"));
    if (!NewContainer || NewContainer->Struct!=ContainerType) return Fail();
    Call Add(Result,TEXT("Add Containerr"));
    if (!Add.Struct(TEXT("Container"),ContainerType,NewContainer->ContainerPtrToValuePtr<void>(Slots.Data())) || !Add.Run()) return Fail();
    Call Refresh(Result,TEXT("Refresh Containers"));
    if (!Refresh.Object(TEXT("SourceInventory"),Result) || !Refresh.Run()) return Fail();
    return Result;
}

bool UCoastalAGISAdapter::InitializeRealProvider()
{
    if (Ready || Busy || !IsInGameThread() || !IsRegistered() || !GetWorld()
        || !GetWorld()->IsGameWorld() || !GetOwner()->HasAuthority()) return false;
    TGuardValue<bool> Lock(Busy,true);
    const TCHAR* InventoryPath=TEXT("/Game/INVENTORY/Core/InventoryComponents/Inventory__Main.Inventory__Main_C");
#if WITH_DEV_AUTOMATION_TESTS
    if(FParse::Param(FCommandLine::Get(),TEXT("CoastalMissingAGIS")))
        InventoryPath=TEXT("/Game/Coastal/Acceptance/MissingInventory.MissingInventory_C");
#endif
    InventoryClass=LoadClass<UActorComponent>(nullptr,InventoryPath);
    UClass* Lib=LoadClass<UObject>(nullptr,TEXT("/Game/INVENTORY/Core/Libraries/FL_AGIS.FL_AGIS_C"));
    Library=Lib ? Lib->GetDefaultObject() : nullptr;
    ContainerType=LoadObject<UScriptStruct>(nullptr,TEXT("/Game/INVENTORY/Data/Structures/ContainerInfo.ContainerInfo"));
    ItemType=LoadObject<UScriptStruct>(nullptr,TEXT("/Game/INVENTORY/Data/Structures/ItemInfo.ItemInfo"));
    if (!InventoryClass || !Library || !ContainerType || !ItemType || !RegisterDefinitions()) return false;
    Backpack=MakeInventory(1); Storage=MakeInventory(2);
    if (!Backpack || !Storage) return false;
    Campaign=FGuid::NewGuid(); Receipts.Reset(); Revision=0;
    TArray<FCoastalAGISItem> Items;
    Ready=ReadLive(Items) && Items.IsEmpty();
    return Ready;
}

ECoastalProviderResult UCoastalAGISAdapter::GetProviderStatus_Implementation() const
{ return Ready && !Busy && IsInGameThread() && IsValid(Backpack) && IsValid(Storage) && DefinitionsValid()
    ? ECoastalProviderResult::Ready : ECoastalProviderResult::NotConfigured; }

void UCoastalAGISAdapter::EndPlay(const EEndPlayReason::Type Reason)
{
    Ready=false;
    if (IsValid(VendorDefinitions) && IsValid(CoastalDefinitions)) VendorDefinitions->RemoveParentTable(CoastalDefinitions);
    if (IsValid(Backpack)) Backpack->DestroyComponent();
    if (IsValid(Storage)) Storage->DestroyComponent();
    Super::EndPlay(Reason);
}

bool UCoastalAGISAdapter::DefinitionsValid() const
{
    if(!IsValid(VendorDefinitions) || !IsValid(CoastalDefinitions))return false;
    const auto* Row=VendorDefinitions->GetRowStruct();
    auto* Size=FieldAs<FStructProperty>(Row,TEXT("ItemSize"));
    auto* Stack=FieldAs<FBoolProperty>(Row,TEXT("Stackable"));
    auto* Limit=FieldAs<FIntProperty>(Row,TEXT("MaxStackSize"));
    if(!Size || Size->Struct!=TBaseStructure<FIntPoint>::Get() || !Stack || !Limit)return false;
    for(int32 Uid:{1,2,3})
    {
        const auto* Ptr=VendorDefinitions->GetRowMap().Find(ItemId(Uid));
        if(!Ptr || !*Ptr || *Size->ContainerPtrToValuePtr<FIntPoint>(*Ptr)!=ItemSize(Uid,false)
            || Stack->GetPropertyValue_InContainer(*Ptr) || Limit->GetPropertyValue_InContainer(*Ptr)!=1)return false;
    }
    return true;
}
