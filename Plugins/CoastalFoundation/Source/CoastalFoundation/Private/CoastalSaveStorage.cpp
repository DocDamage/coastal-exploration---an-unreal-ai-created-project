#include "CoastalSaveCoordinator.h"
#include "CoastalCampaignSave.h"
#include "Core/SaveEnvelope.h"
#include "Kismet/GameplayStatics.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#endif

coastal::SlotInfo UCoastalSaveCoordinator::ReadSlot(FName SaveSet, int32 Index,
    FCoastalCampaignSnapshot& Out) const
{
    using coastal::SlotStatus;
    Out = {};
    const FString Name = SlotName(SaveSet, Index);
    if (!UGameplayStatics::DoesSaveGameExist(Name, 0)) return {SlotStatus::Missing, 0};
    TArray<uint8> Bytes;
    if (!UGameplayStatics::LoadDataFromSlot(Bytes, Name, 0)) return {SlotStatus::Corrupt, 0};
    std::vector<std::uint8_t> Payload;
    const auto Decoded = coastal::DecodeSave(Bytes.GetData(), Bytes.Num(), Payload);
    if (Decoded == coastal::EnvelopeStatus::Unsupported) return {SlotStatus::Incompatible, 0};
    if (Decoded != coastal::EnvelopeStatus::Valid) return {SlotStatus::Corrupt, 0};
    TArray<uint8> Data;
    Data.Append(Payload.data(), static_cast<int32>(Payload.size()));
    const auto* Saved = Cast<UCoastalCampaignSave>(UGameplayStatics::LoadGameFromMemory(Data));
    if (!Saved) return {SlotStatus::Corrupt, 0};
    FString Error;
    const auto Validation = Validate(Saved->Snapshot, true, Error);
    if (Validation == ECoastalProviderResult::Incompatible || Validation == ECoastalProviderResult::NotConfigured)
        return {SlotStatus::Incompatible, 0};
    if (Validation != ECoastalProviderResult::Ready) return {SlotStatus::Corrupt, 0};
    Out = Saved->Snapshot;
    return {SlotStatus::Valid, Out.Generation};
}
bool UCoastalSaveCoordinator::WriteVerified(FName SaveSet, int32 Index,
    const FCoastalCampaignSnapshot& Saved)
{
    auto* Object = NewObject<UCoastalCampaignSave>(this);
    Object->Snapshot = Saved;
    TArray<uint8> Payload;
    if (!UGameplayStatics::SaveGameToMemory(Object, Payload)) return false;
    std::vector<std::uint8_t> Encoded;
    if (!coastal::EncodeSave(Payload.GetData(), Payload.Num(), Encoded)) return false;
    TArray<uint8> Bytes;
    Bytes.Append(Encoded.data(), static_cast<int32>(Encoded.size()));
    if (!UGameplayStatics::SaveDataToSlot(Bytes, SlotName(SaveSet, Index), 0)) return false;
#if WITH_DEV_AUTOMATION_TESTS
    FString TestMode;
    FParse::Value(FCommandLine::Get(),TEXT("CoastalAcceptance="),TestMode);
    if(TestMode==TEXT("campaign_ambiguous") && SaveSet.ToString().StartsWith(TEXT("coastal_test_"))
        && FParse::Param(FCommandLine::Get(),TEXT("CoastalCampaignWriteAfterFailure"))
        && IFileManager::Get().FileExists(*(FPaths::ProjectSavedDir()/TEXT("CoastalAcceptance")/(SaveSet.ToString()+TEXT("-write-after.arm")))))
    {
        UE_LOG(LogTemp,Display,TEXT("COASTAL_TEST_CAMPAIGN_VALID_WRITE_FALSE generation=%lld slot=%d"),Saved.Generation,Index);return false;
    }
#endif
    // Verify exact bytes AND complete semantic decoding, not only a successful API return.
    TArray<uint8> ReadBack;
    if (!UGameplayStatics::LoadDataFromSlot(ReadBack, SlotName(SaveSet, Index), 0) || Bytes != ReadBack) return false;
    FCoastalCampaignSnapshot Verified;
    const auto Info = ReadSlot(SaveSet, Index, Verified);
    return Info.status == coastal::SlotStatus::Valid && Verified.Generation == Saved.Generation
        && Verified.CampaignId == Saved.CampaignId;
}
