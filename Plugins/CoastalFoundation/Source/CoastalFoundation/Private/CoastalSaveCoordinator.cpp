#include "CoastalSaveCoordinator.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalMissionDirector.h"
#include "CoastalWorldObject.h"
#include "FirstSignalComponent.h"
#include "CoastalDestinationQuestRules.h"
#include "Core/TransactionRules.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

UCoastalSaveCoordinator::UCoastalSaveCoordinator()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}
void UCoastalSaveCoordinator::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (Gate.TakeSaveRequest()) SaveNow();
}
ECoastalSaveResult UCoastalSaveCoordinator::Notice(ECoastalSaveResult Result, const FString& Detail)
{
    LastDetail = Detail;
    UE_LOG(LogTemp, Display, TEXT("Coastal save: %d | %s"), static_cast<int32>(Result), *Detail);
    OnSaveNotice.Broadcast(Result, Detail);
    return Result;
}
bool UCoastalSaveCoordinator::Ready() const
{
    return bConfigured && IsValid(Inventory) && IsValid(Player) && IsValid(Mission)
        && Inventory->GetProviderStatus() == ECoastalProviderResult::Ready;
}
bool UCoastalSaveCoordinator::Configure(UCoastalInventoryAdapter* Provider,
    ACharacter* Character, FName LogicalMapId)
{
    if (!IsInGameThread() || Gate.Busy() || bConfigured) return false;
    if (!IsValid(Provider) || !IsValid(Character) || LogicalMapId.IsNone()
        || !GetWorld() || Provider->GetWorld() != GetWorld() || Character->GetWorld() != GetWorld()) return false;
    Mission = GetOwner()->FindComponentByClass<UFirstSignalComponent>();
    int32 Directors = 0;
    for (TActorIterator<ACoastalMissionDirector> It(GetWorld()); It; ++It) ++Directors;
    if (!IsValid(Mission) || Directors != 1 || Provider->GetProviderStatus() != ECoastalProviderResult::Ready)
    { Notice(ECoastalSaveResult::NotConfigured, TEXT("Need one director and a connected AGIS adapter.")); return false; }
    TSet<FName> Ids;
    Objects.Reset();
    for (TActorIterator<ACoastalWorldObject> It(GetWorld()); It; ++It)
    {
        if (!coastal::ValidLogicalId(TCHAR_TO_UTF8(*It->WorldId.ToString()))
            || It->WorldId.IsNone() || Ids.Contains(It->WorldId))
        { Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("Empty, malformed or duplicate world ID.")); return false; }
        Ids.Add(It->WorldId);
        Objects.Add(*It);
    }
    if (Objects.IsEmpty()) return false;
    if (FCoastalDestinationQuestRules::IsFirstSignalMap(LogicalMapId))
    {
        FString ManifestError;
        for (const ACoastalWorldObject* Object : Objects)
            if (!FCoastalDestinationQuestRules::ValidateLiveObject(
                Object->WorldId, Object->Kind, Object->JournalEntry, ManifestError))
            { Notice(ECoastalSaveResult::InvalidSnapshot, ManifestError); Objects.Reset(); return false; }
        for (const FName RequiredId : {FName(TEXT("world.test.radio")), FName(TEXT("world.test.storage")),
            FName(TEXT("world.test.door")), FName(TEXT("world.test.battery")), FName(TEXT("world.test.fuse")),
            FName(TEXT("world.test.note")), FName(TEXT("world.test.postcard"))})
            if (!Ids.Contains(RequiredId))
            { Notice(ECoastalSaveResult::InvalidSnapshot, TEXT("A required First Signal world object is missing.")); Objects.Reset(); return false; }
    }
    Inventory = Provider;
    Player = Character;
    MapId = LogicalMapId;
    DryCheckpoint = Character->GetActorTransform();
    FCoastalInventorySnapshot Initial;
    if (Inventory->ExportInventory(Initial) != ECoastalProviderResult::Ready
        || !Initial.CampaignId.IsValid())
    { Notice(ECoastalSaveResult::NotConfigured, TEXT("Adapter needs an exportable provisional session.")); return false; }
    CampaignId = Initial.CampaignId;
    bConfigured = true;
    FCoastalCampaignSnapshot Baseline;
    FString Error;
    if (!Capture(Baseline, Error))
    { bConfigured = false; Notice(ECoastalSaveResult::InvalidSnapshot, Error); return false; }
    return true;
}
bool UCoastalSaveCoordinator::OwnsObject(const ACoastalWorldObject* Object) const
{
    return IsValid(Object) && Objects.ContainsByPredicate([Object](const auto& Entry) { return Entry.Get() == Object; });
}
bool UCoastalSaveCoordinator::BeginMutation()
{
    return IsInGameThread() && Ready() && HasActiveCampaign() && Gate.BeginMutation();
}
void UCoastalSaveCoordinator::EndMutation() { Gate.EndMutation(); }
void UCoastalSaveCoordinator::RequestSave()
{
    // Restore notifications must not create an autosave of a partially restored world.
    if (IsInGameThread() && !Gate.IsIO() && HasActiveCampaign()) Gate.RequestSave();
}
bool UCoastalSaveCoordinator::UpdateDryCheckpoint(FTransform Transform)
{
    if (!IsInGameThread() || !Ready() || !HasActiveCampaign() || Gate.Busy()
        || !SafeDestination(Transform) || !Gate.BeginMutation()) return false;
    DryCheckpoint = Transform;
    Gate.RequestSave(); Gate.EndMutation(); return true;
}
bool UCoastalSaveCoordinator::ValidSaveSet(FName SaveSet)
{
    const FString Name = SaveSet.ToString();
    return !SaveSet.IsNone() && Name.Len() <= 64
        && coastal::ValidLogicalId(TCHAR_TO_UTF8(*Name));
}
FString UCoastalSaveCoordinator::SlotName(FName SaveSet, int32 Index)
{
    return FString(TEXT("Coastal_")) + SaveSet.ToString() + (Index == 0 ? TEXT("_A") : TEXT("_B"));
}

void UCoastalSaveCoordinator::StopForIntegrationFailure(const FString& Detail)
{
    Gate.Poison();
    Notice(ECoastalSaveResult::RecoveryRequired, TEXT("Startup integration failed; exit and relaunch. ") + Detail);
}
