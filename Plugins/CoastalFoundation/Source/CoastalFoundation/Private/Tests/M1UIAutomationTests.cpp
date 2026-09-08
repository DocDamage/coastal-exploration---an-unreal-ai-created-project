#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalInventoryView.h"
#include "Core/UIFlowRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalUIViewProviderTest,
    "Coastal.M1UI.ProviderViewFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalUIViewProviderTest::RunTest(const FString& Parameters)
{
    auto* Provider = NewObject<UCoastalInventoryAdapter>();
    FCoastalContainerView View; View.ContainerId = TEXT("stale"); View.Revision = 100;
    TestTrue(TEXT("Unwired view is an explicit error"),
        Provider->ReadContainerView(TEXT("container.player"), View) == ECoastalProviderResult::NotConfigured);
    TestTrue(TEXT("No stale display survives failure"), View.ContainerId.IsNone() && View.Items.IsEmpty() && View.Revision < 0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalUIViewValidationTest,
    "Coastal.M1UI.ViewValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalUIViewValidationTest::RunTest(const FString& Parameters)
{
    FCoastalContainerView View; View.ContainerId = TEXT("container.player"); View.Revision = 1; View.Grid = FIntPoint(6, 4);
    FCoastalInventoryViewItem Item; Item.InstanceId = FGuid::NewGuid(); Item.ItemId = TEXT("item.radio_battery");
    Item.DisplayName = FText::FromString(TEXT("Radio battery")); Item.Quantity = 1; Item.Size = FIntPoint(1, 2);
    View.Items.Add(Item);
    TestTrue(TEXT("Native projection accepted"), View.IsValidFor(TEXT("container.player")));
    TestFalse(TEXT("Wrong container rejected"), View.IsValidFor(TEXT("world.test.storage")));
    View.Items.Add(Item); TestFalse(TEXT("Duplicate GUID rejected"), View.IsValidFor(View.ContainerId));
    View.Items.Pop(); View.Items[0].InstanceId.Invalidate();
    TestFalse(TEXT("Invalid GUID rejected before core projection"), View.IsValidFor(View.ContainerId));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalUIFlowTest,
    "Coastal.M1UI.FlowPermissions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalUIFlowTest::RunTest(const FString& Parameters)
{
    coastal::UIFlow Flow; Flow.Reset(1);
    const auto Pause = Flow.Push(coastal::PanelKind::Pause, 10);
    const auto Journal = Flow.Push(coastal::PanelKind::Journal, 10);
    TestFalse(TEXT("Opening event rejected"), Flow.ClaimCommand(Journal, 10));
    TestTrue(TEXT("Next-frame back accepted"), Flow.ClaimCommand(Journal, 11) && Flow.Pop(Journal, true));
    TestFalse(TEXT("Cannot close two menus in one frame"), Flow.ClaimCommand(Pause, 11));
    Flow.Reset(2);
    TestFalse(TEXT("Old session ticket rejected"), Flow.ClaimCommand(Pause, 12));
    const auto Fatal = Flow.RequireRecovery(13);
    TestFalse(TEXT("Recovery is not dismissible"), Flow.Pop(Fatal, true));
    return true;
}
#endif
