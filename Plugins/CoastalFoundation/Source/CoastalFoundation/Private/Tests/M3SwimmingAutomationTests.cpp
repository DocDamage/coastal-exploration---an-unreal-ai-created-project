#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CoastalSwimmingComponent.h"
#include "Core/SwimmingRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalSwimmingDefaultsTest,
    "Coastal.M3Swimming.SafeDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalSwimmingDefaultsTest::RunTest(const FString& Parameters)
{
    const UCoastalSwimmingComponent* Swimming = NewObject<UCoastalSwimmingComponent>();
    TestTrue(TEXT("Finite bounded tuning"), Swimming->SettingsValid());
    TestFalse(TEXT("Not initialized without host binding"), Swimming->IsInitialized());
    TestFalse(TEXT("Not active without a real water zone"), Swimming->IsSurfaceSwimming());
    TestFalse(TEXT("No deep-water exemption without active swimming"), Swimming->IsRecoveryProtected());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalSwimmingTransitionsTest,
    "Coastal.M3Swimming.TransitionFailures",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalSwimmingTransitionsTest::RunTest(const FString& Parameters)
{
    coastal::SwimmingSample Sample;
    Sample.initialized = Sample.binding_valid = Sample.zone_valid = true;
    Sample.within_recovery_depth = Sample.movement_is_swimming = true;
    Sample.campaign_ready = Sample.world_input_allowed = Sample.same_session = true;
    TestEqual(TEXT("Eligible water entry"), coastal::EvaluateSwimming(false, Sample), coastal::SwimmingDecision::Enter);
    TestEqual(TEXT("Stable active swim"), coastal::EvaluateSwimming(true, Sample), coastal::SwimmingDecision::Maintain);
    TestTrue(TEXT("Bounded active swim exempts deep-water return"),
        coastal::AllowsDeepWaterRecoveryExemption(true, Sample));
    TestTrue(TEXT("Eligible adjacent water preserves the active presentation"),
        coastal::AllowsZoneHandoff(true, true, Sample));
    TestFalse(TEXT("The same zone is not a handoff"),
        coastal::AllowsZoneHandoff(true, false, Sample));
    TestTrue(TEXT("Active swim may resolve an overlapping authored zone from the default volume"),
        coastal::AllowsDefaultZoneFallback(true, true, true, true));
    TestFalse(TEXT("A foreign volume is never replaced by the fallback"),
        coastal::AllowsDefaultZoneFallback(true, false, true, true));
    TestFalse(TEXT("Dry land is never treated as a fallback zone"),
        coastal::AllowsDefaultZoneFallback(true, true, true, false));
    TestFalse(TEXT("An invalid brush is never a fallback zone"),
        coastal::AllowsDefaultZoneFallback(true, true, false, true));
    TestTrue(TEXT("Physics-volume overlap contract accepts the required collision"),
        coastal::HasRequiredVolumeCollision(true, true, true, true));
    TestFalse(TEXT("Physics-only volume collision is rejected"),
        coastal::HasRequiredVolumeCollision(true, false, true, true));
    TestFalse(TEXT("Disabled overlap generation is rejected"),
        coastal::HasRequiredVolumeCollision(true, true, false, true));
    TestFalse(TEXT("Non-overlap Pawn response is rejected"),
        coastal::HasRequiredVolumeCollision(true, true, true, false));

    Sample.world_input_allowed = false;
    TestFalse(TEXT("Interrupted input rejects an adjacent-zone handoff"),
        coastal::AllowsZoneHandoff(true, true, Sample));
    TestEqual(TEXT("Menu or recovery input gate cancels presentation"),
        coastal::EvaluateSwimming(true, Sample), coastal::SwimmingDecision::CancelInterrupted);
    TestFalse(TEXT("Input gate removes recovery exemption"),
        coastal::AllowsDeepWaterRecoveryExemption(true, Sample));
    Sample.world_input_allowed = true;
    Sample.within_recovery_depth = false;
    TestEqual(TEXT("Excess depth restores deep-water recovery"),
        coastal::EvaluateSwimming(true, Sample), coastal::SwimmingDecision::CancelUnsafe);
    Sample.within_recovery_depth = true;
    Sample.same_session = false;
    TestEqual(TEXT("Load/session change cancels old swim state"),
        coastal::EvaluateSwimming(true, Sample), coastal::SwimmingDecision::CancelUnsafe);
    Sample.same_session = true;
    Sample.movement_is_swimming = false;
    TestEqual(TEXT("Walking out exits without teleport"),
        coastal::EvaluateSwimming(true, Sample), coastal::SwimmingDecision::ExitWater);
    return true;
}
#endif
