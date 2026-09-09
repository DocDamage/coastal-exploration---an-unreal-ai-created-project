#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/CompanionRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalCompanionLifecycleTest,"Coastal.M3.Companion.CampaignRecoveryAndWaitGates",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoastalCompanionLifecycleTest::RunTest(const FString&)
{
    using namespace coastal;CompanionLifecycleSample S;TestEqual(TEXT("Unconfigured is dormant"),CompanionDecision(S),CompanionDisposition::Dormant);
    S.configured=true;TestEqual(TEXT("No campaign is dormant"),CompanionDecision(S),CompanionDisposition::Dormant);
    S.activeCampaign=S.followRequested=S.epochMatches=true;TestEqual(TEXT("Ready follows"),CompanionDecision(S),CompanionDisposition::Follow);
    S.followRequested=false;TestEqual(TEXT("Wait holds"),CompanionDecision(S),CompanionDisposition::Hold);S.followRequested=true;S.saveBusy=true;
    TestEqual(TEXT("Save holds"),CompanionDecision(S),CompanionDisposition::Hold);S.saveBusy=false;S.returning=true;
    TestEqual(TEXT("Return regroups"),CompanionDecision(S),CompanionDisposition::Regroup);S.returning=false;S.epochMatches=false;
    TestEqual(TEXT("Epoch change regroups"),CompanionDecision(S),CompanionDisposition::Regroup);S.epochMatches=true;S.recoveryRequired=true;
    TestEqual(TEXT("Recovery regroups"),CompanionDecision(S),CompanionDisposition::Regroup);S.recoveryRequired=false;S.needsRegroup=true;S.paused=true;
    TestEqual(TEXT("Pause holds before regroup"),CompanionDecision(S),CompanionDisposition::Hold);S.paused=false;
    TestEqual(TEXT("Dry regroup request regroups"),CompanionDecision(S),CompanionDisposition::Regroup);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalCompanionDistanceTest,"Coastal.M3.Companion.BoundedFollowPaces",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCoastalCompanionDistanceTest::RunTest(const FString&)
{
    using namespace coastal;const CompanionFollowBands B{155,650,2400};TestTrue(TEXT("Bands valid"),B.Valid());
    TestEqual(TEXT("Formation idles"),CompanionPaceForDistance(155,B),CompanionPace::Idle);TestEqual(TEXT("Near walks"),CompanionPaceForDistance(156,B),CompanionPace::Walk);
    TestEqual(TEXT("Far runs"),CompanionPaceForDistance(650,B),CompanionPace::Run);TestEqual(TEXT("Extreme catches up"),CompanionPaceForDistance(2400,B),CompanionPace::Catchup);
    const CompanionFollowBands Bad{650,155,2400};TestFalse(TEXT("Reversed bands rejected"),Bad.Valid());TestEqual(TEXT("Invalid never moves"),CompanionPaceForDistance(800,Bad),CompanionPace::Idle);return true;
}
#endif
