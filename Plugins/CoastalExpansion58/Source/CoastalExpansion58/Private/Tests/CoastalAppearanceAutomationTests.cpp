#include "CoastalAppearanceTypes.h"
#include "CoastalCampaignSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalAppearanceRoundTrip,
    "Coastal.M3Character.AppearanceSerialization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalAppearanceRoundTrip::RunTest(const FString& Parameters)
{
    auto* Appearance = NewObject<UCoastalAppearanceSave>();
    Appearance->Campaign = FGuid::NewGuid();
    Appearance->AppearanceVersion = TEXT("coastal.human.v1");
    Appearance->Generation = 43;
    Appearance->Selections = {0, 2, 5, 1, 0, 4, 2};
    TArray<uint8> Bytes;
    TestTrue(TEXT("Appearance serializes independently"), UGameplayStatics::SaveGameToMemory(Appearance, Bytes));
    auto* Loaded = Cast<UCoastalAppearanceSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Appearance class survives serialization"), Loaded)) return false;
    TestEqual(TEXT("Schema survives"), Loaded->Schema, 1);
    TestEqual(TEXT("Campaign identity survives"), Loaded->Campaign, Appearance->Campaign);
    TestEqual(TEXT("Generation survives"), Loaded->Generation, Appearance->Generation);
    TestEqual(TEXT("Definition identity survives"), Loaded->AppearanceVersion, Appearance->AppearanceVersion);
    TestTrue(TEXT("All selection indices survive"), Loaded->Selections == Appearance->Selections);
    TestNull(TEXT("An appearance cannot be treated as a campaign save"), Cast<UCoastalCampaignSave>(Loaded));
    auto* Campaign = NewObject<UCoastalCampaignSave>();
    TestTrue(TEXT("Legacy campaign remains serializable"), UGameplayStatics::SaveGameToMemory(Campaign, Bytes));
    TestNull(TEXT("Campaign cannot be treated as an appearance"), Cast<UCoastalAppearanceSave>(UGameplayStatics::LoadGameFromMemory(Bytes)));
    return true;
}
#endif
