#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FirstSignalComponent.h"
#include "CoastalInventoryAdapter.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstSignalDefaultAdapterTest,
    "Coastal.FirstSignal.DefaultAdapterFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFirstSignalDefaultAdapterTest::RunTest(const FString& Parameters)
{
    UFirstSignalComponent* Quest = NewObject<UFirstSignalComponent>();
    UCoastalInventoryAdapter* Adapter = NewObject<UCoastalInventoryAdapter>();
    TestTrue(TEXT("Missing provider is explicit"),
        Quest->RequestRadioRepair(nullptr) == ECoastalInventoryCommit::NotConfigured);
    TestTrue(TEXT("Unwired adapter cannot repair"),
        Quest->RequestRadioRepair(Adapter) == ECoastalInventoryCommit::NotConfigured);
    TestFalse(TEXT("No fake repaired flag"), Quest->ExportSnapshot().bRadioRepaired);
    TestTrue(TEXT("Cannot finish before repair"),
        Quest->FinishRadioTransmission() == ECoastalListenResult::NotRepaired);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstSignalSnapshotTest,
    "Coastal.FirstSignal.SnapshotValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFirstSignalSnapshotTest::RunTest(const FString& Parameters)
{
    UFirstSignalComponent* Quest = NewObject<UFirstSignalComponent>();
    Quest->InspectRadio();
    Quest->ReadMaintenanceNote();
    FFirstSignalSnapshot Saved = Quest->ExportSnapshot();
    Quest->ResetForNewGame();
    TestTrue(TEXT("Valid snapshot restores"), Quest->RestoreSnapshot(Saved));
    TestTrue(TEXT("Inspection round trips"), Quest->ExportSnapshot().bRadioInspected);
    Saved.bMessageHeard = true;
    TestFalse(TEXT("Impossible message state rejected"), Quest->RestoreSnapshot(Saved));
    Saved.bMessageHeard = false;
    Saved.SchemaVersion = 99;
    TestFalse(TEXT("Future schema rejected"), Quest->RestoreSnapshot(Saved));
    return true;
}
#endif
