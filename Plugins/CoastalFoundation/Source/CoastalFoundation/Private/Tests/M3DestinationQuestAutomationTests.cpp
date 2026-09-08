#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CoastalDestinationQuestRules.h"
#include "CoastalStoryLibrary.h"
#include "FirstSignalComponent.h"
#include "Core/TestRoomRules.h"
#include "UObject/UObjectGlobals.h"

namespace
{
FCoastalWorldRecord Record(FName Id, ECoastalObjectKind Kind, bool bActive = false)
{
    FCoastalWorldRecord Result;
    Result.WorldId = Id; Result.Kind = Kind; Result.bActive = bActive;
    return Result;
}
TArray<FCoastalWorldRecord> RequiredWorld()
{
    return {
        Record(TEXT("world.test.radio"), ECoastalObjectKind::Radio),
        Record(TEXT("world.test.storage"), ECoastalObjectKind::Storage),
        Record(TEXT("world.test.door"), ECoastalObjectKind::Door),
        Record(TEXT("world.test.battery"), ECoastalObjectKind::Pickup),
        Record(TEXT("world.test.fuse"), ECoastalObjectKind::Pickup),
        Record(TEXT("world.test.note"), ECoastalObjectKind::MaintenanceNote),
        Record(TEXT("world.test.postcard"), ECoastalObjectKind::Discovery)
    };
}

void AppendDestination(TArray<FCoastalWorldRecord>& World, bool bActive = false)
{
    for (int32 Index = 0; Index < FCoastalDestinationQuestRules::DestinationStepCount; ++Index)
        World.Add(Record(FCoastalDestinationQuestRules::Step(Index).WorldId,
            ECoastalObjectKind::Discovery, bActive));
}
TArray<FName> AcknowledgedFirstSignal()
{
    return {TEXT("journal.first_signal.transmission"), TEXT("journal.north_reach.lead")};
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalDestinationStartupManifestTest,
    "Coastal.M3Destination.AuthoredStartupManifest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalDestinationStartupManifestTest::RunTest(const FString& Parameters)
{
    const std::vector<coastal::RoomObject> Base(coastal::TestRoomManifest().begin(), coastal::TestRoomManifest().end());
    std::vector<coastal::RoomObject> Extension;
    for (int32 Index = 0; Index < FCoastalDestinationQuestRules::DestinationStepCount; ++Index)
    {
        const auto Step = FCoastalDestinationQuestRules::Step(Index);
        Extension.push_back({TCHAR_TO_UTF8(*Step.WorldId.ToString()), "DISCOVERY", "",
            TCHAR_TO_UTF8(*Step.JournalId.ToString()), 1, false});
    }
    auto Full = Base;
    Full.insert(Full.end(), Extension.begin(), Extension.end());
    TestTrue(TEXT("Expanded authored First Signal starts"), coastal::AuditTestRoom(Full, 1, false, Extension).empty());
    TestFalse(TEXT("Other map profiles reject destination actors"), coastal::AuditTestRoom(Full, 1).empty());
    TestFalse(TEXT("Expanded map requires all destination actors"), coastal::AuditTestRoom(Base, 1, false, Extension).empty());
    for (size_t Index = Base.size(); Index < Full.size(); ++Index)
    {
        auto Wrong = Full; Wrong[Index].journal = "journal.wrong";
        TestFalse(TEXT("Destination journal mismatch rejected"), coastal::AuditTestRoom(Wrong, 1, false, Extension).empty());
        Wrong = Full; Wrong[Index].kind = "PICKUP";
        TestFalse(TEXT("Destination kind mismatch rejected"), coastal::AuditTestRoom(Wrong, 1, false, Extension).empty());
        Wrong = Full; Wrong[Index].active = true;
        TestFalse(TEXT("Destination must begin unread"), coastal::AuditTestRoom(Wrong, 1, false, Extension).empty());
        Wrong = Full; const auto Duplicate = Wrong[Index]; Wrong.push_back(Duplicate);
        TestFalse(TEXT("Duplicate destination rejected"), coastal::AuditTestRoom(Wrong, 1, false, Extension).empty());
    }
    auto Unknown = Full; Unknown.push_back({"world.unknown", "DISCOVERY", "", "journal.unknown", 1, false});
    TestFalse(TEXT("Unknown records remain rejected"), coastal::AuditTestRoom(Unknown, 1, false, Extension).empty());
    TestFalse(TEXT("Capacity profile does not inherit expansion"), coastal::AuditTestRoom(Full, 1, true, Extension).empty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalDestinationManifestTest,
    "Coastal.M3Destination.OptionalSaveMigrationAndCoherence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalDestinationManifestTest::RunTest(const FString& Parameters)
{
    FString Error;
    TestFalse(TEXT("Optional migration is scoped away from systems/capacity maps"),
        FCoastalDestinationQuestRules::IsFirstSignalMap(TEXT("level.systems_test")));
    TestTrue(TEXT("Optional migration is scoped to First Signal"),
        FCoastalDestinationQuestRules::IsFirstSignalMap(TEXT("level.first_signal")));
    const TArray<FCoastalWorldRecord> OldWorld = RequiredWorld();
    TestTrue(TEXT("Old seven-record First Signal save remains valid"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(OldWorld, {}, true, Error));
    TestEqual(TEXT("Mission flag alone does not start destination progression"),
        FCoastalDestinationQuestRules::NextStep(true, {}), INDEX_NONE);
    TestFalse(TEXT("Mission flag alone cannot unlock first destination"),
        FCoastalDestinationQuestRules::CanRead(
            FCoastalDestinationQuestRules::Step(0).WorldId, true, {}));
    const TArray<FName> Acknowledged = AcknowledgedFirstSignal();
    TestEqual(TEXT("Coherent acknowledgment starts at first destination"),
        FCoastalDestinationQuestRules::NextStep(true, Acknowledged), 0);
    TestTrue(TEXT("Coherent acknowledgment unlocks first destination"),
        FCoastalDestinationQuestRules::CanRead(
            FCoastalDestinationQuestRules::Step(0).WorldId, true, Acknowledged));

    TArray<FCoastalWorldRecord> Expanded = RequiredWorld();
    AppendDestination(Expanded);
    Error.Empty();
    TestTrue(TEXT("Expanded unread manifest is coherent"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(Expanded, Acknowledged, true, Error));

    Expanded[7].bActive = true;
    Expanded[8].bActive = true;
    TArray<FName> Journal = Acknowledged;
    Journal.Add(FCoastalDestinationQuestRules::Step(0).JournalId);
    Journal.Add(FCoastalDestinationQuestRules::Step(1).JournalId);
    Swap(Expanded[7], Expanded[13]);
    Error.Empty();
    TestTrue(TEXT("Record serialization order does not define quest order"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(Expanded, Journal, true, Error));
    TestTrue(TEXT("Prior discovery unlocks the next authored record"),
        FCoastalDestinationQuestRules::CanRead(
            FCoastalDestinationQuestRules::Step(2).WorldId, true, Journal));
    TestFalse(TEXT("Later record remains locked"),
        FCoastalDestinationQuestRules::CanRead(
            FCoastalDestinationQuestRules::Step(3).WorldId, true, Journal));

    TArray<FCoastalWorldRecord> MissingRequired = RequiredWorld();
    MissingRequired.RemoveAt(0);
    Error.Empty();
    TestFalse(TEXT("Original required record cannot be migrated away"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(MissingRequired, {}, true, Error));

    TArray<FCoastalWorldRecord> Unknown = RequiredWorld();
    Unknown.Add(Record(TEXT("world.destination.forged"), ECoastalObjectKind::Discovery));
    Error.Empty();
    TestFalse(TEXT("Unknown expansion record is rejected"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(Unknown, {}, true, Error));
    Error.Empty();
    TestFalse(TEXT("Unknown destination journal ID is rejected"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(
            RequiredWorld(), {TEXT("journal.coastal_records.forged")}, true, Error));

    TArray<FCoastalWorldRecord> WrongKind = RequiredWorld();
    WrongKind.Add(Record(FCoastalDestinationQuestRules::Step(0).WorldId, ECoastalObjectKind::Door));
    Error.Empty();
    TestFalse(TEXT("Destination kind mismatch is rejected"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(WrongKind, {}, true, Error));

    TArray<FCoastalWorldRecord> Duplicate = RequiredWorld();
    const FCoastalWorldRecord DuplicatedRecord = Duplicate[0];
    Duplicate.Add(DuplicatedRecord);
    Error.Empty();
    TestFalse(TEXT("Duplicate record is rejected"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(Duplicate, {}, true, Error));

    TArray<FCoastalWorldRecord> Forged = RequiredWorld();
    Forged.Add(Record(FCoastalDestinationQuestRules::Step(1).WorldId, ECoastalObjectKind::Discovery, true));
    TArray<FName> ForgedJournal = {FCoastalDestinationQuestRules::Step(1).JournalId};
    Error.Empty();
    TestFalse(TEXT("Skipped prerequisite is rejected"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(Forged, ForgedJournal, true, Error));
    Error.Empty();
    TestFalse(TEXT("Destination progress before First Signal is rejected"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(Forged, ForgedJournal, false, Error));

    auto* PublicMission = NewObject<UFirstSignalComponent>();
    FFirstSignalSnapshot Repaired;
    Repaired.bRadioInspected = true;
    Repaired.bRadioRepaired = true;
    TestTrue(TEXT("Valid repaired state prepared"), PublicMission->RestoreSnapshot(Repaired));
    TestTrue(TEXT("Public finish call can set mission flag"),
        PublicMission->FinishRadioTransmission() == ECoastalListenResult::Applied);
    const TArray<FName> NoBridgeJournal;
    TestFalse(TEXT("Public finish bypass cannot authorize destination mutation"),
        FCoastalDestinationQuestRules::CanRead(FCoastalDestinationQuestRules::Step(0).WorldId,
            PublicMission->ExportSnapshot().bMessageHeard, NoBridgeJournal));
    TestTrue(TEXT("Bridge-equivalent coherent journal acknowledgment authorizes it"),
        FCoastalDestinationQuestRules::CanRead(FCoastalDestinationQuestRules::Step(0).WorldId,
            PublicMission->ExportSnapshot().bMessageHeard, Acknowledged));
    TArray<FCoastalWorldRecord> BypassWorld = RequiredWorld();
    BypassWorld.Add(Record(FCoastalDestinationQuestRules::Step(0).WorldId,
        ECoastalObjectKind::Discovery, true));
    Error.Empty();
    TestFalse(TEXT("Mission flag plus destination entry still cannot forge a coherent save"),
        FCoastalDestinationQuestRules::ValidateSavedManifest(BypassWorld,
            {FCoastalDestinationQuestRules::Step(0).JournalId}, true, Error));

    TMap<FName, const FCoastalWorldRecord*> OldLookup;
    for (const FCoastalWorldRecord& Record : OldWorld) OldLookup.Add(Record.WorldId, &Record);
    for (int32 Index = 0; Index < FCoastalDestinationQuestRules::DestinationStepCount; ++Index)
    {
        bool bRestoredActive = true; // Simulates state left by the campaign being replaced.
        TestTrue(TEXT("Old/cross-campaign restore explicitly resets missing approved discovery"),
            FCoastalDestinationQuestRules::ResolveRestoreState(
                FCoastalDestinationQuestRules::Step(Index).WorldId, OldLookup, bRestoredActive)
                == ECoastalRestoreRecordResolution::ResetOptional);
        TestFalse(TEXT("Missing approved discovery restores unread"), bRestoredActive);
    }
    bool bRequiredState = true;
    TestTrue(TEXT("Missing required record cannot be silently reset"),
        FCoastalDestinationQuestRules::ResolveRestoreState(
            TEXT("world.test.radio"), {}, bRequiredState) == ECoastalRestoreRecordResolution::Reject);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalDestinationPresentationTest,
    "Coastal.M3Destination.ProgressPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalDestinationPresentationTest::RunTest(const FString& Parameters)
{
    TArray<FName> Journal = AcknowledgedFirstSignal();
    TestEqual(TEXT("First destination title"),
        UCoastalStoryLibrary::CampaignTitle(EFirstSignalPhase::Complete, Journal).ToString(),
        FString(TEXT("NORTH REACH")));
    TestEqual(TEXT("First destination objective"),
        UCoastalStoryLibrary::CampaignObjective(EFirstSignalPhase::Complete, Journal).ToString(),
        FString(TEXT("Read the inspection log at the Industrial Harbour.")));
    for (int32 Index = 0; Index < 3; ++Index)
        Journal.Add(FCoastalDestinationQuestRules::Step(Index).JournalId);
    TestEqual(TEXT("Second chain title derives from journal progress"),
        UCoastalStoryLibrary::CampaignTitle(EFirstSignalPhase::Complete, Journal).ToString(),
        FString(TEXT("COASTAL RECORDS")));
    TestEqual(TEXT("Hallsands objective derives from journal progress"),
        UCoastalStoryLibrary::CampaignObjective(EFirstSignalPhase::Complete, Journal).ToString(),
        FString(TEXT("Inspect the evacuation marker at Hallsands.")));
    return true;
}
#endif
