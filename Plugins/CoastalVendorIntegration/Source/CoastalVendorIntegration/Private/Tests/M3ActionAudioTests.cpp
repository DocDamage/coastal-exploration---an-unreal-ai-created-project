#include "Misc/AutomationTest.h"
#include "CoastalActionAudio.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalFeedbackResults,"Coastal.M3.Audio.ResultSemantics",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoastalFeedbackResults::RunTest(const FString&)
{
    using R=ECoastalActionResult;
    TestEqual(TEXT("Only applied operation keeps its authored cue"),UCoastalActionAudio::ResultCue(R::Applied,TEXT("pickup")),FName(TEXT("pickup")));
    for(auto Result:{R::AlreadyApplied,R::SuppressedInput,R::StaleFocus,R::BlockedByUI,R::Busy,R::InvalidTarget})
        TestTrue(TEXT("Duplicate/stale/blocked requests are quiet"),UCoastalActionAudio::ResultCue(Result,TEXT("pickup")).IsNone());
    for(auto Result:{R::MissingItems,R::NoSpace,R::TooFar,R::Occluded,R::Failed,R::NotConfigured})
        TestEqual(TEXT("Failures cannot sound successful"),UCoastalActionAudio::ResultCue(Result,TEXT("pickup")),FName(TEXT("error")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalFeedbackUnbound,"Coastal.M3.Audio.UnboundRefusal",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoastalFeedbackUnbound::RunTest(const FString&)
{
    auto* Audio=NewObject<UCoastalActionAudio>();
    TestFalse(TEXT("Missing real controller/routing/assets refuses initialization"),Audio->Initialize(nullptr,nullptr,nullptr,{}));
    TestNull(TEXT("No fabricated voice"),Audio->GetFeedbackVoice());
    TestEqual(TEXT("No submitted playback"),Audio->SubmissionCount,0);
    return true;
}
#endif
