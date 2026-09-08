#include "Misc/AutomationTest.h"
#include "CoastalSprintComponent.h"
#include "CoastalLocalOptions.h"
#include "Core/OptionsPersistence.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalSprintUnboundTest,
    "Coastal.M1Sprint.UnboundFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalSprintUnboundTest::RunTest(const FString& Parameters)
{
    auto* Sprint = NewObject<UCoastalSprintComponent>();
    auto* Profile = NewObject<UCoastalLocalOptions>();
    TestFalse(TEXT("No implicit host opt-in"), Sprint->bUseHostSprintEvents);
    TestFalse(TEXT("No character guessed"), Sprint->InitializeOptions(Profile, nullptr));
    TestFalse(TEXT("No binding"), Sprint->IsSprintReady());
    TestFalse(TEXT("No accepted input before binding"), Sprint->SubmitSprintInput(true));
    TestFalse(TEXT("No synthetic sprint"), Sprint->IsSprintRequested());
    Sprint->ReleaseOptions(); TestFalse(TEXT("Cannot restart stopped consumer"), Sprint->InitializeOptions(Profile, nullptr));
    // No profile initialization or platform IO; no substitute engine/AGIS supplied.
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalSprintTransitionTest,
    "Coastal.M1Sprint.TransitionRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalSprintTransitionTest::RunTest(const FString& Parameters)
{
    coastal::SprintGate Gate;
    Gate.Synchronize(true,1,1,true,0); Gate.Sample(false,1); Gate.Sample(true,2);
    TestTrue(TEXT("Deliberate toggle"), Gate.Requested(2));
    Gate.Synchronize(true,1,3,true,3);
    TestFalse(TEXT("Menu round trip cancels intention"), Gate.Requested(3));
    Gate.Sample(true,4); TestFalse(TEXT("Held key cannot resume"), Gate.Requested(4));
    Gate.Sample(false,5); Gate.Sample(true,6); TestTrue(TEXT("Release then press"), Gate.Requested(6));
    TestFalse(TEXT("Stale host input expires"), Gate.Requested(9));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalSprintSpeedTest,
    "Coastal.M1Sprint.SpeedOwnershipRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalSprintSpeedTest::RunTest(const FString& Parameters)
{
    coastal::WalkSpeedLease Lease; double Next=600;
    TestTrue(TEXT("Acquire initial speed"), Lease.Acquire(Next,{}));
    TestTrue(TEXT("Apply sprint"), Lease.Apply(Next,true,Next)); TestTrue(TEXT("Sprint tuning"), Next==520);
    TestFalse(TEXT("Foreign speed ends ownership"), Lease.Apply(700,false,Next));
    TestTrue(TEXT("Foreign value preserved"), Next==700);
    TestFalse(TEXT("Do not restore over foreign value"), Lease.Release(700,Next));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalSprintOptionTest,
    "Coastal.M1Sprint.OptionCodec", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalSprintOptionTest::RunTest(const FString& Parameters)
{
    coastal::PlayerOptions Value;
    TestFalse(TEXT("Default Hold"), Value.sprintToggle);
    TestTrue(TEXT("Toggle field"), coastal::AdjustOption(Value,coastal::OptionField::SprintMode,1));
    coastal::OptionsRecord Restored; std::vector<std::uint8_t> Bytes;
    TestTrue(TEXT("Encode current preference schema"), coastal::EncodeOptions({Value,1},Bytes));
    TestTrue(TEXT("Decode current preference schema"), coastal::DecodeOptions(Bytes.data(),Bytes.size(),Restored)==coastal::OptionsStatus::Valid);
    TestTrue(TEXT("All options retained"), Restored.values==Value && Restored.sourceSchema==coastal::OptionsSchema);
    return true;
}
#endif
