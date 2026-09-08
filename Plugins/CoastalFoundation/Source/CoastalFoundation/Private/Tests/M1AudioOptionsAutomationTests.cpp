#include "Misc/AutomationTest.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalLocalOptions.h"
#include "Core/OptionsPersistence.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalAudioUnboundTest,
    "Coastal.M1AudioOptions.UnboundFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalAudioUnboundTest::RunTest(const FString& Parameters)
{
    auto* Audio=NewObject<UCoastalAudioOptionsComponent>(); auto* Profile=NewObject<UCoastalLocalOptions>();
    TestFalse(TEXT("No implicit audio routing"),Audio->bUseDedicatedSoundClasses);
    TestFalse(TEXT("No guessed controller/device/classes"),Audio->InitializeOptions(Profile));
    TestFalse(TEXT("Unavailable"),Audio->IsAudioReady()); TestFalse(TEXT("Unbound apply refused"),Audio->ApplyAudioOptions());
    Audio->ReleaseOptions();Audio->ReleaseOptions();
    TestFalse(TEXT("Stopped cannot restart"),Audio->InitializeOptions(Profile));
    // No real user preference initialization, audio playback, or platform IO.
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalAudioGainTest,
    "Coastal.M1AudioOptions.GainRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalAudioGainTest::RunTest(const FString& Parameters)
{
    coastal::PlayerOptions Values;Values.masterPercent=50;Values.ambiencePercent=20;Values.radioPercent=0;
    coastal::AudioGains Gains;TestTrue(TEXT("Valid gains"),coastal::BuildAudioGains(Values,Gains));
    TestTrue(TEXT("Master exactly once, muted radio"),Gains==coastal::AudioGains{.1,.5,0});
    Values.masterPercent=101;TestFalse(TEXT("Bad data"),coastal::BuildAudioGains(Values,Gains));
    TestTrue(TEXT("No partial data"),Gains==coastal::AudioGains{});return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalAudioOwnershipTest,
    "Coastal.M1AudioOptions.RoutingAndLifecycleRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalAudioOwnershipTest::RunTest(const FString& Parameters)
{
    std::array<coastal::AudioClassInfo,3> Classes{{{1,true,1,1},{2,true,1,1},{3,true,1,1}}};
    TestTrue(TEXT("Three isolated distinct classes"),coastal::ValidAudioClasses(Classes));
    Classes[1].identity=1;TestFalse(TEXT("No aliased buses"),coastal::ValidAudioClasses(Classes));
    coastal::AudioMixSession State;TestTrue(TEXT("One activation"),State.Begin({0,.5,1}));
    TestFalse(TEXT("No double push"),State.Begin({1,1,1}));
    TestTrue(TEXT("Release only once"),State.Release());TestFalse(TEXT("No extra pop"),State.Release());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalAudioCodecTest,
    "Coastal.M1AudioOptions.CurrentCodec", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalAudioCodecTest::RunTest(const FString& Parameters)
{
    coastal::PlayerOptions Values;Values.sprintToggle=true;Values.masterPercent=0;Values.radioPercent=65;
    coastal::OptionsRecord Read;std::vector<std::uint8_t> Bytes;
    TestTrue(TEXT("Encode"),coastal::EncodeOptions({Values,1},Bytes));TestTrue(TEXT("Exact size"),Bytes.size()==80);
    TestTrue(TEXT("Decode"),coastal::DecodeOptions(Bytes.data(),Bytes.size(),Read)==coastal::OptionsStatus::Valid);
    TestTrue(TEXT("Retain full options"),Read.values==Values && Read.sourceSchema==3);return true;
}
#endif
