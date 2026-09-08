#include "Misc/AutomationTest.h"
#include "CoastalDisplaySettings.h"
#include "CoastalUISessionComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalDisplayUnboundTest,
    "Coastal.M1Display.UnboundFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalDisplayUnboundTest::RunTest(const FString& Parameters)
{
    auto* Owner = NewObject<UCoastalDisplaySettings>();
    auto* UI = NewObject<UCoastalUISessionComponent>();
    TestFalse(TEXT("Explicit display ownership defaults off"), UI->bEnableDisplaySettings);
    TestFalse(TEXT("Cannot bind a missing player"), Owner->Initialize(nullptr, true));
    TestFalse(TEXT("Unbound not ready"), Owner->Ready());
    TestFalse(TEXT("Unbound no synthetic catalogue"), Owner->RefreshCatalogue());
    TestFalse(TEXT("Unbound cannot request display"), Owner->Begin({}, {}));
    Owner->Tick({}, false, false); Owner->Cancel(); Owner->Release(); Owner->Release();
    TestFalse(TEXT("Released cannot keep or save"), Owner->Keep({}, true, true));
    return true; // No real display change or file IO is performed by this test.
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalDisplayCatalogueTest,
    "Coastal.M1Display.CatalogueRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalDisplayCatalogueTest::RunTest(const FString& Parameters)
{
    using namespace coastal;
    const auto Modes = DisplayCatalogue({{}, {1920,1080,DisplayWindowMode::Windowed}, {1920,1080,DisplayWindowMode::Windowed}});
    TestTrue(TEXT("Only one valid unique candidate"), Modes.size() == 1);
    TestTrue(TEXT("Legacy small viewport can be a rollback snapshot"), ValidDisplaySnapshot({1024,600,DisplayWindowMode::Windowed}));
    TestFalse(TEXT("Small viewport not a new candidate"), SelectableDisplayMode({1024,600,DisplayWindowMode::Windowed}));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalDisplayDeadlineTest,
    "Coastal.M1Display.DeadlineRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalDisplayDeadlineTest::RunTest(const FString& Parameters)
{
    using namespace coastal;
    DisplayTrial Trial;
    DisplayMode Before{1600,900,DisplayWindowMode::Windowed}, After{1920,1080,DisplayWindowMode::Fullscreen};
    DisplayObservation O{10,1,{1,0,PanelKind::ConfirmDisplay},Before,true,true,false};
    TestTrue(TEXT("Single change request"), Trial.Begin(Before,After,{After},O)==DisplayCommand::RequestCandidate);
    O.seconds=25; O.frame=100; O.actual=After;
    TestTrue(TEXT("Paused time still expires"), Trial.Step(O)==DisplayCommand::RequestRestore);
    TestFalse(TEXT("Deadline cannot keep"), Trial.Keep(O));
    TestTrue(TEXT("Pending restoration is not success"), Trial.Phase()==DisplayPhase::Reverting);
    O.seconds=26; ++O.frame; O.actual=Before; Trial.Step(O);
    TestTrue(TEXT("Observed previous mode closes trial"), Trial.Phase()==DisplayPhase::Idle);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalDisplayModalTest,
    "Coastal.M1Display.ModalRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalDisplayModalTest::RunTest(const FString& Parameters)
{
    using namespace coastal;
    UIFlow Flow; Flow.Reset(1); Flow.Push(PanelKind::Session,1);
    const auto Parent=Flow.Push(PanelKind::Display,2), Child=Flow.Push(PanelKind::ConfirmDisplay,3);
    TestFalse(TEXT("No hidden-parent command"), Flow.CanCommand(Parent,4));
    TestFalse(TEXT("No opening-frame keep"), Flow.CanCommand(Child,3));
    TestTrue(TEXT("Top later-frame command"), Flow.ClaimCommand(Child,4));
    TestFalse(TEXT("No same-frame second command"), Flow.ClaimCommand(Child,4));
    TestTrue(TEXT("Inactive title can close only child"), Flow.Pop(Child,false));
    return true;
}
#endif
