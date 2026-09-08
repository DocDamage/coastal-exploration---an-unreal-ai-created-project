#include "Misc/AutomationTest.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "CoastalSaveCoordinator.h"
#include "Core/RecoveryRules.h"
#include "Core/CampaignRules.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalRecoveryOwnerTest,
    "Coastal.M1Recovery.UnboundOwnerFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalRecoveryOwnerTest::RunTest(const FString& Parameters)
{
    auto* Recovery = NewObject<UCoastalPlayerRecoveryComponent>();
    TestFalse(TEXT("No fabricated host binding"), Recovery->InitializeRecovery(nullptr, nullptr, FTransform::Identity));
    TestFalse(TEXT("Not initialized"), Recovery->IsInitialized());
    TestFalse(TEXT("Not returning"), Recovery->IsReturning());
    auto* Saves = NewObject<UCoastalSaveCoordinator>();
    TestFalse(TEXT("Checkpoint update needs a campaign"), Saves->UpdateDryCheckpoint(FTransform::Identity));
    TestFalse(TEXT("No return reservation"), Saves->IsPlayerReturnActive());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalRecoveryGateTest,
    "Coastal.M1Recovery.ExclusiveCampaignGate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalRecoveryGateTest::RunTest(const FString& Parameters)
{
    coastal::OperationGate Gate;
    Gate.RequestSave(); TestTrue(TEXT("Start relocation"), Gate.BeginRecovery());
    TestFalse(TEXT("No capture mid-return"), Gate.TakeSaveRequest());
    TestFalse(TEXT("No load/new/save IO"), Gate.BeginIO());
    TestFalse(TEXT("No inventory mutation"), Gate.BeginMutation());
    Gate.EndRecovery(); TestTrue(TEXT("Queued save retained"), Gate.TakeSaveRequest()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalRecoveryGeometryTest,
    "Coastal.M1Recovery.SharedSafetyGeometry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalRecoveryGeometryTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Capsule touches box face"), coastal::SafetyCapsuleOverlap(134, 0, 0, 34, 88, 100, 100, 100));
    TestFalse(TEXT("Capsule outside rounded corner"), coastal::SafetyCapsuleOverlap(134, 134, 0, 34, 88, 100, 100, 100));
    TestFalse(TEXT("Invalid shape"), coastal::SafetyCapsuleOverlap(0, 0, 0, -1, 88, 100, 100, 100)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalRecoveryFlowTest,
    "Coastal.M1Recovery.SettleAndSessionRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalRecoveryFlowTest::RunTest(const FString& Parameters)
{
    coastal::ReturnFlow Flow; TestTrue(TEXT("Start current session"), Flow.Begin(7));
    TestFalse(TEXT("Reject changed session"), Flow.Matches(8));
    Flow.Advance(coastal::ReturnFadeOut, false); TestTrue(TEXT("Place once"), Flow.Placed());
    Flow.Advance(coastal::ReturnFadeIn, false);
    for (int I = 0; I < 4; ++I) Flow.Advance(0.1, true);
    TestTrue(TEXT("Stable finish"), Flow.Finish());
    Flow.Fail(); TestFalse(TEXT("No looping retry after failure"), Flow.Begin(7)); return true;
}
#endif
