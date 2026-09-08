#include "Misc/AutomationTest.h"
#include "CoastalAudioPlaybackComponent.h"
#include "CoastalAudioOptionsComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalPlaybackUnboundTest,
    "Coastal.M1AudioPlayback.UnboundFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalPlaybackUnboundTest::RunTest(const FString& Parameters)
{
    auto* Playback = NewObject<UCoastalAudioPlaybackComponent>();
    TestFalse(TEXT("Playback opt-in defaults false"), Playback->bEnablePlayback);
    TestNull(TEXT("No fabricated ambience"), Playback->AmbienceLoop.Get());
    TestNull(TEXT("No fabricated radio"), Playback->RadioTransmission.Get());
    TestFalse(TEXT("No implicit owner or routing"), Playback->InitializePlayback(nullptr, nullptr));
    TestFalse(TEXT("Unbound not ready"), Playback->IsPlaybackReady());
    Playback->RefreshPlayback(); Playback->SuspendPlayback(); Playback->ReleasePlayback(); Playback->ReleasePlayback();
    TestFalse(TEXT("Released owner cannot restart"), Playback->InitializePlayback(nullptr, nullptr));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalPlaybackPresentationTest,
    "Coastal.M1AudioPlayback.PresentationRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalPlaybackPresentationTest::RunTest(const FString& Parameters)
{
    coastal::AudioPlaybackSession Session;
    TestTrue(TEXT("Explicit session"), Session.Begin(true, true));
    coastal::AudioPlaybackContext Context;
    Context.epoch = 1; Context.active = true; Context.interrupted = false; Context.ambienceAllowed = true;
    TestTrue(TEXT("Start bed once"), Session.Step(Context).startAmbience);
    Context.ambienceAllowed = false; Context.transcript = {1, 1, coastal::PanelKind::Transcript};
    auto Commands = Session.Step(Context);
    TestTrue(TEXT("Pause bed, wait for paint"), Commands.pauseAmbience && !Commands.startRadio);
    Context.presented = true;
    TestTrue(TEXT("Painted transcript may play"), Session.Step(Context).startRadio);
    TestTrue(TEXT("No repeated play"), Session.Step(Context).Empty());
    TestTrue(TEXT("Interruption stops presentation"), Session.Suspend().stopRadio);
    TestFalse(TEXT("Old ticket does not resume"), Session.Step(Context).startRadio);
    Context.transcript.id = 2;
    TestTrue(TEXT("Explicit new panel may replay"), Session.Step(Context).startRadio);
    // No audio device, component Play, preference file, provider or mission call in this test.
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalPlaybackSourcesTest,
    "Coastal.M1AudioPlayback.SourceRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalPlaybackSourcesTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Loop with silent continuation"), coastal::ValidAmbienceSource({true,true,true,0}));
    TestFalse(TEXT("Finite source is not ambience"), coastal::ValidAmbienceSource({true,false,true,20}));
    TestTrue(TEXT("Finite transmission"), coastal::ValidRadioSource({true,false,true,20}));
    TestFalse(TEXT("Loop cannot be a transmission"), coastal::ValidRadioSource({true,true,true,20}));
    TestFalse(TEXT("Reject missing duration"), coastal::ValidRadioSource({true,false,true,0}));
    TestFalse(TEXT("No unverified silent-voice policy"), coastal::ValidRadioSource({true,false,false,20}));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalPlaybackRoutingReleaseTest,
    "Coastal.M1AudioPlayback.RoutingReleaseReentry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalPlaybackRoutingReleaseTest::RunTest(const FString& Parameters)
{
    auto* Routing = NewObject<UCoastalAudioOptionsComponent>();
    int32 Calls = 0;
    Routing->OnRoutingReleased.AddLambda([&]() { ++Calls; Routing->ReleaseOptions(); });
    Routing->ReleaseOptions(); Routing->ReleaseOptions();
    TestEqual(TEXT("One notification, recursive release cannot repop or rebroadcast"), Calls, 1);
    TestNull(TEXT("Stopped ambience route unavailable"), Routing->GetAmbienceClassForPlayback());
    TestNull(TEXT("Stopped radio route unavailable"), Routing->GetRadioClassForPlayback());
    Routing->OnRoutingReleased.Clear();
    // This unbound test never pushes/pops a real mix or writes the user's preferences.
    return true;
}
#endif
