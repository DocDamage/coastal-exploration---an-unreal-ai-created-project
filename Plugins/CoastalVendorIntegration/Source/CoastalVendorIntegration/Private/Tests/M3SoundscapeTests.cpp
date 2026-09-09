#include "Misc/AutomationTest.h"
#include "CoastalSoundscape.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalSoundscapeZones,"Coastal.M3.Soundscape.ZoneBoundaries",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoastalSoundscapeZones::RunTest(const FString&)
{
    const FVector Center(100,200,500);
    TestTrue(TEXT("Authored horizontal arrival admitted"),UCoastalSoundscape::WithinZone(Center+FVector(1000,0,0),Center,1000,false));
    TestFalse(TEXT("Outside entry radius remains coast"),UCoastalSoundscape::WithinZone(Center+FVector(1200,0,0),Center,1000,false));
    TestTrue(TEXT("Boundary hysteresis retains existing score"),UCoastalSoundscape::WithinZone(Center+FVector(1200,0,0),Center,1000,true));
    TestFalse(TEXT("Leaving hysteresis returns to coast"),UCoastalSoundscape::WithinZone(Center+FVector(1401,0,0),Center,1000,true));
    TestFalse(TEXT("Unrelated vertical level excluded"),UCoastalSoundscape::WithinZone(Center+FVector(0,0,2501),Center,1000,true));
    TestFalse(TEXT("Invalid radius excluded"),UCoastalSoundscape::WithinZone(Center,Center,-1,true));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalSoundscapeUnbound,"Coastal.M3.Soundscape.UnboundRefusal",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoastalSoundscapeUnbound::RunTest(const FString&)
{
    auto* Soundscape=NewObject<UCoastalSoundscape>();
    TestFalse(TEXT("No UI/route/world cannot initialize"),Soundscape->Initialize(nullptr,nullptr));
    TestNull(TEXT("No score allocated"),Soundscape->GetMusicVoice());
    TestNull(TEXT("No thunder allocated"),Soundscape->GetThunderVoice());
    return true;
}
#endif
