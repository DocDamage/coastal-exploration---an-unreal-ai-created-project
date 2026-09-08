#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalContractLibrary.h"
#include "CoastalCampaignSave.h"
#include "Kismet/GameplayStatics.h"
#include "Core/SaveEnvelope.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalM1ProviderTest,
    "Coastal.M1.ProviderFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalM1ProviderTest::RunTest(const FString& Parameters)
{
    auto* Adapter = NewObject<UCoastalInventoryAdapter>();
    FCoastalInventorySnapshot Snapshot;
    TestTrue(TEXT("Status fails closed"), Adapter->GetProviderStatus() == ECoastalProviderResult::NotConfigured);
    TestTrue(TEXT("Export fails closed"), Adapter->ExportInventory(Snapshot) == ECoastalProviderResult::NotConfigured);
    TestTrue(TEXT("Restore fails closed"), Adapter->RestoreInventory(Snapshot) == ECoastalProviderResult::NotConfigured);
    TestTrue(TEXT("New game fails closed"), Adapter->BuildNewInventory(FGuid::NewGuid(), Snapshot) == ECoastalProviderResult::NotConfigured);
    TestTrue(TEXT("Pickup fails closed"), Adapter->TryCollectWorldItem(TEXT("world.test.battery"), {}) == ECoastalInventoryCommit::NotConfigured);
    TestTrue(TEXT("Transfer fails closed"), Adapter->TryTransfer({}) == ECoastalInventoryCommit::NotConfigured);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalM1MemorySerializationTest,
    "Coastal.M1.SaveMemoryRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalM1MemorySerializationTest::RunTest(const FString& Parameters)
{
    // Serialization plumbing only. This is NOT a semantically valid AGIS campaign.
    auto* Saved = NewObject<UCoastalCampaignSave>();
    Saved->Snapshot.CampaignId = FGuid::NewGuid();
    Saved->Snapshot.Generation = 12;
    Saved->Snapshot.MapId = TEXT("level.systems_test");
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Serialize USaveGame"), UGameplayStatics::SaveGameToMemory(Saved, Bytes))) return false;
    std::vector<std::uint8_t> Envelope, Payload;
    TestTrue(TEXT("Envelope"), coastal::EncodeSave(Bytes.GetData(), Bytes.Num(), Envelope));
    TestTrue(TEXT("Decode"), coastal::DecodeSave(Envelope.data(), Envelope.size(), Payload) == coastal::EnvelopeStatus::Valid);
    TArray<uint8> Decoded;
    Decoded.Append(Payload.data(), static_cast<int32>(Payload.size()));
    auto* Loaded = Cast<UCoastalCampaignSave>(UGameplayStatics::LoadGameFromMemory(Decoded));
    if (!TestNotNull(TEXT("Deserialize USaveGame"), Loaded)) return false;
    TestTrue(TEXT("Campaign preserved"), Loaded->Snapshot.CampaignId == Saved->Snapshot.CampaignId);
    TestEqual(TEXT("Generation preserved"), Loaded->Snapshot.Generation, int64(12));
    return true;
}
#endif
