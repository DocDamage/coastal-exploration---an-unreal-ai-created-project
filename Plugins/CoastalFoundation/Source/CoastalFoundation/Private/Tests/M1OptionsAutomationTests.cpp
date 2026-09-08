#include "Misc/AutomationTest.h"
#include "CoastalLookInputComponent.h"
#include "CoastalLocalOptions.h"
#include "Core/OptionsSession.h"
#include "Core/UIFlowRules.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalOptionsUnboundTest,
    "Coastal.M1Options.UnboundConsumersFailClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalOptionsUnboundTest::RunTest(const FString& Parameters)
{
    auto* Look = NewObject<UCoastalLookInputComponent>();
    auto* Profile = NewObject<UCoastalLocalOptions>();
    TestFalse(TEXT("No camera guessed without a character"), Look->InitializeOptions(Profile, nullptr));
    TestFalse(TEXT("No active binding"), Look->IsCameraReady());
    TestFalse(TEXT("No input route"), Look->SubmitMouseLook(FVector2D(1, 1)));
    TestFalse(TEXT("No stick route"), Look->SubmitStickLook(FVector2D(1, 1)));
    TestFalse(TEXT("Uninitialized preferences cannot write"), Profile->CanWrite());
    TestFalse(TEXT("No uninitialized apply"), Profile->ApplySession(coastal::PlayerOptions{}));
    // Does NOT initialize the profile or touch the developer's real local preference files.
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalOptionsCodecTest,
    "Coastal.M1Options.PreferenceCodec", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalOptionsCodecTest::RunTest(const FString& Parameters)
{
    std::vector<std::uint8_t> Bytes;
    coastal::OptionsRecord Record{{200, 150, 95, 140, true}, 3}, Restored;
    TestTrue(TEXT("Encode bounded preference data"), coastal::EncodeOptions(Record, Bytes));
    TestTrue(TEXT("Decode exact bytes"), coastal::DecodeOptions(Bytes.data(), Bytes.size(), Restored) == coastal::OptionsStatus::Valid);
    TestTrue(TEXT("All fields retained"), Restored.values == Record.values && Restored.generation == 3);
    Bytes.back() ^= 1;
    TestTrue(TEXT("Reject accidental corruption"), coastal::DecodeOptions(Bytes.data(), Bytes.size(), Restored) == coastal::OptionsStatus::Corrupt);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalOptionsFlowTest,
    "Coastal.M1Options.ModalAndPresentationRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalOptionsFlowTest::RunTest(const FString& Parameters)
{
    coastal::UIFlow Flow; Flow.Reset(1);
    const auto Parent = Flow.Push(coastal::PanelKind::Session, 1);
    const auto Options = Flow.Push(coastal::PanelKind::Settings, 2);
    TestFalse(TEXT("Parent cannot apply behind child"), Flow.ClaimCommand(Parent, 3));
    TestTrue(TEXT("Later deliberate options action"), Flow.ClaimCommand(Options, 3));
    TestFalse(TEXT("No second command in same frame"), Flow.ClaimCommand(Options, 3));
    TestTrue(TEXT("Back retains the initial session"), Flow.Pop(Options, false));
    TestFalse(TEXT("Offscreen body is not a display opportunity"), coastal::VisibleUIIntersection({0, 900, 400, 1000}, {0, 0, 400, 500}));
    TestTrue(TEXT("Visible body can count"), coastal::VisibleUIIntersection({0, 40, 400, 900}, {0, 0, 400, 500}));
    return true;
}
#endif
