#pragma once

#include "CoreMinimal.h"
#include "CoastalCampaignTypes.h"

struct FCoastalDestinationStep
{
    FName WorldId;
    FName JournalId;
};

enum class ECoastalRestoreRecordResolution : uint8
{
    Saved,
    ResetOptional,
    Reject
};

// Fixed migration and progression policy for the seven destination discoveries.
// Progress remains derived from world records plus journal entries; no parallel quest state is stored.
class COASTALFOUNDATION_API FCoastalDestinationQuestRules
{
public:
    static constexpr int32 DestinationStepCount = 7;
    static bool IsFirstSignalMap(FName MapId);
    static bool IsRequiredWorldId(FName WorldId, ECoastalObjectKind* ExpectedKind = nullptr);
    static bool IsOptionalWorldId(FName WorldId, int32* StepIndex = nullptr);
    static bool IsDestinationJournalId(FName JournalId, int32* StepIndex = nullptr);
    static FCoastalDestinationStep Step(int32 Index);
    static bool ValidateLiveObject(FName WorldId, ECoastalObjectKind Kind, FName JournalId,
        FString& Error);
    static bool ValidateSavedManifest(const TArray<FCoastalWorldRecord>& World,
        const TArray<FName>& Journal, bool bFirstSignalComplete, FString& Error);
    static bool CanRead(FName WorldId, bool bFirstSignalComplete, const TArray<FName>& Journal);
    // -1 means First Signal is incomplete; DestinationStepCount means both chains are complete.
    static int32 NextStep(bool bFirstSignalComplete, const TArray<FName>& Journal);
    static ECoastalRestoreRecordResolution ResolveRestoreState(FName LiveWorldId,
        const TMap<FName, const FCoastalWorldRecord*>& SavedRecords, bool& bOutActive);
};
