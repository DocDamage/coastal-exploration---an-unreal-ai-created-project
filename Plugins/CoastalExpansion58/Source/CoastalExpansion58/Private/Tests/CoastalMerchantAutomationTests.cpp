#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Core/MerchantPresentationRules.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalMerchantLifecycleTest,
    "Coastal.M3Merchant.PresentationLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalMerchantLifecycleTest::RunTest(const FString& Parameters)
{
    using namespace coastal;
    MerchantSample S{true, true, true, false, false, false, false, false, false, true, false, false, false};
    TestEqual(TEXT("Healthy presenter wakes into idle"), AdvanceMerchant(MerchantPhase::Dormant, S), MerchantPhase::Idle);
    TestEqual(TEXT("Idle remains idle"), AdvanceMerchant(MerchantPhase::Idle, S), MerchantPhase::Idle);
    S.modal = true; S.paused = true; S.worldInput = false;
    S.expectedModalSeen = true;
    TestEqual(TEXT("Expected journal modal retains queued presentation"), AdvanceMerchant(MerchantPhase::AwaitJournalClose, S), MerchantPhase::AwaitJournalClose);
    S.modal = false; S.paused = false; S.worldInput = true;
    TestEqual(TEXT("Journal close starts one bounded offer pitch"), AdvanceMerchant(MerchantPhase::AwaitJournalClose, S), MerchantPhase::Pitching);
    S.clipFinished = true;
    TestEqual(TEXT("Pitch completion restores idle"), AdvanceMerchant(MerchantPhase::Pitching, S), MerchantPhase::Idle);
    S.clipFinished = false; S.paused = true;
    TestEqual(TEXT("Unrelated pause cancels active pitch"), AdvanceMerchant(MerchantPhase::Pitching, S), MerchantPhase::Idle);
    S.paused = false; S.recovery = true;
    TestEqual(TEXT("Recovery makes presentation dormant"), AdvanceMerchant(MerchantPhase::Pitching, S), MerchantPhase::Dormant);
    S.recovery = false; S.epochMatches = false;
    TestEqual(TEXT("Campaign replacement retires queued work"), AdvanceMerchant(MerchantPhase::AwaitJournalClose, S), MerchantPhase::Dormant);
    S.epochMatches = true; S.queueExpired = true;
    TestEqual(TEXT("Unclosed journal cannot retain a queue forever"), AdvanceMerchant(MerchantPhase::AwaitJournalClose, S), MerchantPhase::Idle);
    return true;
}
#endif
