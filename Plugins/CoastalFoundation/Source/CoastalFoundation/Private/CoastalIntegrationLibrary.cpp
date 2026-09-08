#include "CoastalIntegrationLibrary.h"
#include "CoastalMissionDirector.h"
#include "CoastalWorldObject.h"
#include "CoastalDestinationQuestRules.h"
#include "Core/TestRoomRules.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

FString FCoastalIntegrationReport::ToText() const
{
    if (bPassed) return TEXT("Startup prerequisites passed. This is not vendor transaction or gameplay certification.");
    FString Text = TEXT("COASTAL STARTUP BLOCKED\n");
    for (const auto& Issue : Issues)
        Text += Issue.Code.ToString() + TEXT(" | ") + Issue.Subject + TEXT(": ") + Issue.Detail + TEXT("\n");
    return Text;
}
namespace
{
    std::string KindName(ECoastalObjectKind Kind)
    {
        switch (Kind)
        {
        case ECoastalObjectKind::Door: return "DOOR";
        case ECoastalObjectKind::Pickup: return "PICKUP";
        case ECoastalObjectKind::Storage: return "STORAGE";
        case ECoastalObjectKind::Radio: return "RADIO";
        case ECoastalObjectKind::Discovery: return "DISCOVERY";
        case ECoastalObjectKind::MaintenanceNote: return "MAINTENANCE_NOTE";
        default: return "INVALID";
        }
    }
    std::string NameText(FName Name) { return Name.IsNone() ? "" : TCHAR_TO_UTF8(*Name.ToString()); }
}
bool UCoastalIntegrationLibrary::AuditTestRoom(const UObject* Context, FCoastalIntegrationReport& Report, bool CapacityFixture)
{
    Report = {};
    UWorld* World = IsValid(Context) && GEngine
        ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!IsInGameThread() || !World)
    { Report.Add(TEXT("room.no_world"), TEXT("room"), TEXT("A valid editor/game world on the game thread is required.")); return false; }
    int32 Directors = 0;
    for (TActorIterator<ACoastalMissionDirector> It(World); It; ++It) ++Directors;
    std::vector<coastal::RoomObject> Rows;
    for (TActorIterator<ACoastalWorldObject> It(World); It; ++It)
    {
        Rows.push_back({NameText(It->WorldId), KindName(It->Kind), NameText(It->PickupItem.ItemId),
            NameText(It->JournalEntry), It->PickupItem.Quantity, It->IsActive()});
        if (Rows.size() > 256) break;
    }
    std::vector<coastal::RoomObject> Extension;
    FString MapName = World->GetMapName();
    if (!World->StreamingLevelsPrefix.IsEmpty()) MapName.RemoveFromStart(World->StreamingLevelsPrefix);
    if (!CapacityFixture && MapName == TEXT("L_FirstSignal"))
        for (int32 Index = 0; Index < FCoastalDestinationQuestRules::DestinationStepCount; ++Index)
        {
            const auto Step = FCoastalDestinationQuestRules::Step(Index);
            Extension.push_back({NameText(Step.WorldId), "DISCOVERY", "", NameText(Step.JournalId), 1, false});
        }
    for (const auto& Issue : coastal::AuditTestRoom(Rows, Directors, CapacityFixture, Extension))
    {
        if (Report.Issues.Num() >= 255)
        {
            Report.Add(TEXT("room.issue_limit"), TEXT("room"), TEXT("Additional issues omitted. Fix the listed authoring errors and rerun."));
            break;
        }
        Report.Add(FName(UTF8_TO_TCHAR(Issue.code.c_str())), UTF8_TO_TCHAR(Issue.subject.c_str()), UTF8_TO_TCHAR(Issue.detail.c_str()));
    }
    Report.Finish(); return Report.bPassed;
}

FCoastalIntegrationReport UCoastalIntegrationLibrary::InspectTestRoom(const UObject* Context)
{
    FCoastalIntegrationReport Report; AuditTestRoom(Context, Report); return Report;
}
