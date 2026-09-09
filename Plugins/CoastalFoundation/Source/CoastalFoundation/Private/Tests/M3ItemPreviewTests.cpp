#include "Misc/AutomationTest.h"
#include "CoastalItemPreviewComponent.h"
#include "Core/ItemPreviewRules.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalItemPreviewRulesTest,
    "Coastal.M3ItemPreview.RotationRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalItemPreviewRulesTest::RunTest(const FString& Parameters)
{
    coastal::ItemPreviewPose Pose;
    TestTrue(TEXT("Finite degree deltas are accepted"), Pose.Apply(735.0f, 80.0f));
    TestEqual(TEXT("Yaw wraps without changing world state"), Pose.yaw, 15.0f);
    TestEqual(TEXT("Pitch clamps"), Pose.pitch, coastal::PreviewPitchMaximum);
    TestTrue(TEXT("Reverse delta is accepted"), Pose.Apply(-30.0f, -200.0f));
    TestEqual(TEXT("Yaw wraps below zero"), Pose.yaw, 345.0f);
    TestEqual(TEXT("Pitch clamps at lower bound"), Pose.pitch, coastal::PreviewPitchMinimum);
    TestFalse(TEXT("Nonfinite yaw fails closed"), Pose.Apply(std::numeric_limits<float>::quiet_NaN(), 0.0f));
    TestFalse(TEXT("Negative inventory revision is stale"), coastal::ValidPreviewRevision(-1));
    TestTrue(TEXT("Zero inventory revision is valid"), coastal::ValidPreviewRevision(0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalItemPreviewUnboundTest,
    "Coastal.M3ItemPreview.UnboundFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalItemPreviewUnboundTest::RunTest(const FString& Parameters)
{
    auto* Preview = NewObject<UCoastalItemPreviewComponent>();
    const FGuid Instance = FGuid::NewGuid();
    TestFalse(TEXT("No controller creates no preview"), Preview->InitializePreview(nullptr));
    TestFalse(TEXT("Unbound preview cannot open"), Preview->OpenPreview(Instance, 0, nullptr));
    TestFalse(TEXT("Unbound preview cannot rotate"), Preview->RotatePreview(Instance, 0, 15.0f, 0.0f));
    Preview->ClosePreview(); Preview->ClosePreview();
    TestFalse(TEXT("Close remains idempotent"), Preview->IsPreviewOpen());
    TestNull(TEXT("No render target survives unbound close"), Preview->GetPreviewTexture());
    return true;
}
#endif
