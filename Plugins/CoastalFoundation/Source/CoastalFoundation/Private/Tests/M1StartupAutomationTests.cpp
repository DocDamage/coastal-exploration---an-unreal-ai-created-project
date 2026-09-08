#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CoastalIntegrationLibrary.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalSessionBootstrapComponent.h"
#include "CoastalPlacementLibrary.h"
#include "Core/TestRoomRules.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalProviderAuditTest,
    "Coastal.M1Startup.ProviderAuditFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalProviderAuditTest::RunTest(const FString& Parameters)
{
    FCoastalIntegrationReport Report; Report.bPassed = true;
    TestFalse(TEXT("Null provider rejected"), UCoastalIntegrationLibrary::AuditProvisionalProvider(nullptr, Report));
    TestFalse(TEXT("Stale pass cleared"), Report.bPassed);
    auto* Adapter = NewObject<UCoastalInventoryAdapter>();
    TestFalse(TEXT("Base provider cannot qualify"), UCoastalIntegrationLibrary::AuditProvisionalProvider(Adapter, Report));
    TestTrue(TEXT("Actual diagnostic retained"), Report.Issues.Num() == 1 && Report.Issues[0].Code == TEXT("provider.not_ready"));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalStartupOwnerTest,
    "Coastal.M1Startup.InvalidOwnerFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalStartupOwnerTest::RunTest(const FString& Parameters)
{
    auto* Bootstrap = NewObject<UCoastalSessionBootstrapComponent>();
    TestTrue(TEXT("No owner means blocked"), Bootstrap->StartTestRoom(nullptr, nullptr, FTransform::Identity) == ECoastalStartupResult::Blocked);
    TestTrue(TEXT("Only read-only failure retryable"), Bootstrap->GetPhase() == ECoastalStartupPhase::Blocked);
    TestTrue(TEXT("Diagnostics exist"), !Bootstrap->LastReport.Issues.IsEmpty());
    TestFalse(TEXT("Missing character cannot qualify dry placement"), UCoastalPlacementLibrary::IsDryDestination(nullptr, FTransform::Identity));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalStartupGateTest,
    "Coastal.M1Startup.LifecycleRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalStartupGateTest::RunTest(const FString& Parameters)
{
    coastal::StartupGate Gate;
    TestTrue(TEXT("Explicit startup only"), Gate.Begin() == coastal::StartupRequest::Proceed);
    TestTrue(TEXT("Reentrancy refused"), Gate.Begin() == coastal::StartupRequest::Busy);
    Gate.FinishChecks(true); Gate.FinishBinding(false);
    TestTrue(TEXT("Partial binding requires restart"), Gate.Begin() == coastal::StartupRequest::RestartRequired);
    TestFalse(TEXT("Stale success cannot unpoison"), Gate.FinishBinding(true));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalRoomAuditTest,
    "Coastal.M1Startup.RoomManifest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalRoomAuditTest::RunTest(const FString& Parameters)
{
    const auto& Manifest = coastal::TestRoomManifest();
    std::vector<coastal::RoomObject> Room(Manifest.begin(), Manifest.end());
    TestTrue(TEXT("Complete authoring manifest"), coastal::AuditTestRoom(Room,1).empty());
    Room[3].item = "item.marine_fuse";
    TestFalse(TEXT("Wrong part cannot pass startup"), coastal::AuditTestRoom(Room,1).empty());
    FCoastalIntegrationReport Report;
    TestFalse(TEXT("Native audit requires a real world"), UCoastalIntegrationLibrary::AuditTestRoom(nullptr, Report));
    return true;
}
#endif
