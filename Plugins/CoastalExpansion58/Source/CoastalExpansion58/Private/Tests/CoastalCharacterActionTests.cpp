#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/CharacterActionRules.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalCharacterDirectionTest,
    "Coastal.M3.Character.FacingRelativeActionDirections",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalCharacterDirectionTest::RunTest(const FString&)
{
    using namespace coastal;
    using D = ActionDirection;
    TestEqual(TEXT("Front attacker"), CharacterActionDirection(100, 0), D::Front);
    TestEqual(TEXT("Rear attacker"), CharacterActionDirection(-100, 0), D::Back);
    TestEqual(TEXT("Right attacker"), CharacterActionDirection(0, 100), D::Right);
    TestEqual(TEXT("Left attacker"), CharacterActionDirection(0, -100), D::Left);
    TestEqual(TEXT("Diagonal tie is stable"), CharacterActionDirection(-50, 50), D::Back);
    TestEqual(TEXT("Dominant side"), CharacterActionDirection(-49, 50), D::Right);
    TestEqual(TEXT("Coincident damage fallback"), CharacterActionDirection(0, 0), D::Front);
    TestEqual(TEXT("Invalid bearing fallback"), CharacterActionDirection(std::numeric_limits<double>::quiet_NaN(), 10), D::Front);
    // World-front source becomes local-left when the character turns right.
    const FVector Local = FTransform(FRotator(0, 90, 0)).InverseTransformVectorNoScale(FVector(100, 0, 0));
    TestEqual(TEXT("Bearing follows character facing"), CharacterActionDirection(Local.X, Local.Y), D::Left);
    return true;
}
#endif
