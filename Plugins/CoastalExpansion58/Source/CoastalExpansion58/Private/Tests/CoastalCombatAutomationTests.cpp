#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/CombatRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalCombatGateTest,
    "Coastal.M3.Combat.GatesRejectUnsafeActions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalCombatGateTest::RunTest(const FString&)
{
    using namespace coastal;
    CombatGateSample S;
    S.configured = S.activeCampaign = S.encounter = S.worldInput = S.epochMatches = true;
    S.cooldownReady = true; S.clip = 6; S.clipCapacity = 12; S.reserve = 24;
    TestEqual(TEXT("Safe fire is allowed"), FireDecision(S), CombatDecision::Allowed);
    S.worldInput = false;
    TestEqual(TEXT("Menu/input ownership blocks fire"), FireDecision(S), CombatDecision::InputBlocked);
    S.worldInput = true; S.returning = true;
    TestEqual(TEXT("Recovery blocks fire"), FireDecision(S), CombatDecision::Recovering);
    S.returning = false; S.camping = true;
    TestEqual(TEXT("Camping blocks fire"), FireDecision(S), CombatDecision::Camping);
    S.camping = false; S.swimming = true;
    TestEqual(TEXT("Swimming blocks fire"), FireDecision(S), CombatDecision::Swimming);
    S.swimming = false; S.foreignAnimation = true;
    TestEqual(TEXT("Foreign montage blocks fire"), FireDecision(S), CombatDecision::ForeignAnimation);
    S.foreignAnimation = false; S.encounter = false;
    TestEqual(TEXT("Outside the encounter blocks fire"), FireDecision(S), CombatDecision::OutsideEncounter);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalCombatAmmoTest,
    "Coastal.M3.Combat.AuthoritativeAmmoAndReload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalCombatAmmoTest::RunTest(const FString&)
{
    using namespace coastal;
    TestTrue(TEXT("One actual decrement is accepted"), AcceptedAuthoritativeShot(7, 6));
    TestFalse(TEXT("Predicted extra decrement is rejected"), AcceptedAuthoritativeShot(7, 5));
    TestFalse(TEXT("No mutation is rejected"), AcceptedAuthoritativeShot(7, 7));
    CombatGateSample S;
    S.configured = S.activeCampaign = S.encounter = S.worldInput = S.epochMatches = true;
    S.cooldownReady = true; S.clip = 12; S.clipCapacity = 12; S.reserve = 20;
    TestEqual(TEXT("Full magazine cannot reload"), ReloadDecision(S), CombatDecision::MagazineFull);
    S.clip = 2; S.reserve = 0;
    TestEqual(TEXT("Empty transient reserve cannot reload"), ReloadDecision(S), CombatDecision::NoReserve);
    S.reserve = 20;
    TestEqual(TEXT("Partial magazine can reload"), ReloadDecision(S), CombatDecision::Allowed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalCombatDamageTest,
    "Coastal.M3.Combat.ShieldHealthAndDefeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalCombatDamageTest::RunTest(const FString&)
{
    using namespace coastal;
    DamageState State{100.f, 30.f, false};
    State = ApplyDamage(State, 20.f);
    TestEqual(TEXT("Shield absorbs first damage"), State.shield, 10.f);
    TestEqual(TEXT("Health remains intact"), State.health, 100.f);
    State = ApplyDamage(State, 35.f);
    TestEqual(TEXT("Overflow reaches health"), State.health, 75.f);
    TestEqual(TEXT("Shield is exhausted"), State.shield, 0.f);
    State = ApplyDamage(State, 100.f);
    TestTrue(TEXT("Lethal damage defeats once"), State.defeated);
    TestEqual(TEXT("Health clamps to zero"), State.health, 0.f);
    const DamageState Repeated = ApplyDamage(State, 50.f);
    TestEqual(TEXT("Defeated state does not mutate again"), Repeated.health, State.health);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalCombatEpochTest,
    "Coastal.M3.Combat.CampaignEpochResetsTransientState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalCombatEpochTest::RunTest(const FString&)
{
    coastal::EncounterEpoch State{4, true, true};
    TestFalse(TEXT("Same epoch keeps transient state"), State.Observe(4));
    TestTrue(TEXT("New epoch reports reset"), State.Observe(5));
    TestFalse(TEXT("Weapon ownership resets"), State.armed);
    TestFalse(TEXT("Defeat resets"), State.defeated);
    TestEqual(TEXT("New epoch retained"), State.epoch, uint64(5));
    return true;
}
#endif
