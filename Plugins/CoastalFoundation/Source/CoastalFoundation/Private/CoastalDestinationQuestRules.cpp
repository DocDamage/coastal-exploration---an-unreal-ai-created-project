#include "CoastalDestinationQuestRules.h"

namespace
{
const FName TransmissionEntry(TEXT("journal.first_signal.transmission"));
const FName NorthReachLeadEntry(TEXT("journal.north_reach.lead"));

bool HasCoherentFirstSignalAcknowledgement(const TArray<FName>& Journal)
{
    return Journal.Contains(TransmissionEntry) && Journal.Contains(NorthReachLeadEntry);
}

const TArray<FCoastalDestinationStep>& DestinationSteps()
{
    static const TArray<FCoastalDestinationStep> Steps = {
        {TEXT("world.north_reach.harbour_log"), TEXT("journal.north_reach.harbour_log")},
        {TEXT("world.north_reach.platform_signal"), TEXT("journal.north_reach.platform_signal")},
        {TEXT("world.north_reach.powell_order"), TEXT("journal.north_reach.powell_order")},
        {TEXT("world.coastal_records.hallsands"), TEXT("journal.coastal_records.hallsands")},
        {TEXT("world.coastal_records.village"), TEXT("journal.coastal_records.village")},
        {TEXT("world.coastal_records.baelo"), TEXT("journal.coastal_records.baelo")},
        {TEXT("world.coastal_records.prison"), TEXT("journal.coastal_records.prison")}
    };
    return Steps;
}

struct FRequiredRecord
{
    FName WorldId;
    ECoastalObjectKind Kind;
    FName JournalId;
};

const TArray<FRequiredRecord>& RequiredRecords()
{
    static const TArray<FRequiredRecord> Records = {
        {TEXT("world.test.radio"), ECoastalObjectKind::Radio, NAME_None},
        {TEXT("world.test.storage"), ECoastalObjectKind::Storage, NAME_None},
        {TEXT("world.test.door"), ECoastalObjectKind::Door, NAME_None},
        {TEXT("world.test.battery"), ECoastalObjectKind::Pickup, NAME_None},
        {TEXT("world.test.fuse"), ECoastalObjectKind::Pickup, NAME_None},
        {TEXT("world.test.note"), ECoastalObjectKind::MaintenanceNote, NAME_None},
        {TEXT("world.test.postcard"), ECoastalObjectKind::Discovery, TEXT("journal.first_signal.postcard")}
    };
    return Records;
}
}

bool FCoastalDestinationQuestRules::IsFirstSignalMap(FName MapId)
{
    return MapId == TEXT("level.first_signal");
}

bool FCoastalDestinationQuestRules::IsRequiredWorldId(FName WorldId, ECoastalObjectKind* ExpectedKind)
{
    const FRequiredRecord* Match = RequiredRecords().FindByPredicate(
        [WorldId](const FRequiredRecord& Record) { return Record.WorldId == WorldId; });
    if (!Match) return false;
    if (ExpectedKind) *ExpectedKind = Match->Kind;
    return true;
}

bool FCoastalDestinationQuestRules::IsOptionalWorldId(FName WorldId, int32* StepIndex)
{
    const int32 Index = DestinationSteps().IndexOfByPredicate(
        [WorldId](const FCoastalDestinationStep& Step) { return Step.WorldId == WorldId; });
    if (Index == INDEX_NONE) return false;
    if (StepIndex) *StepIndex = Index;
    return true;
}

bool FCoastalDestinationQuestRules::IsDestinationJournalId(FName JournalId, int32* StepIndex)
{
    const int32 Index = DestinationSteps().IndexOfByPredicate(
        [JournalId](const FCoastalDestinationStep& Step) { return Step.JournalId == JournalId; });
    if (Index == INDEX_NONE) return false;
    if (StepIndex) *StepIndex = Index;
    return true;
}

FCoastalDestinationStep FCoastalDestinationQuestRules::Step(int32 Index)
{
    return DestinationSteps().IsValidIndex(Index) ? DestinationSteps()[Index] : FCoastalDestinationStep{};
}

bool FCoastalDestinationQuestRules::ValidateLiveObject(FName WorldId, ECoastalObjectKind Kind,
    FName JournalId, FString& Error)
{
    ECoastalObjectKind ExpectedKind;
    if (IsRequiredWorldId(WorldId, &ExpectedKind))
    {
        if (Kind != ExpectedKind)
        { Error = TEXT("Required First Signal world object has the wrong kind."); return false; }
        const FRequiredRecord* Match = RequiredRecords().FindByPredicate(
            [WorldId](const FRequiredRecord& Record) { return Record.WorldId == WorldId; });
        if (Match && Match->JournalId != JournalId)
        { Error = TEXT("Required First Signal world object has the wrong journal binding."); return false; }
        return true;
    }
    int32 Index = INDEX_NONE;
    if (!IsOptionalWorldId(WorldId, &Index))
    { Error = TEXT("World object is outside the approved First Signal manifest."); return false; }
    if (Kind != ECoastalObjectKind::Discovery || JournalId != DestinationSteps()[Index].JournalId)
    { Error = TEXT("Destination discovery has the wrong kind or journal binding."); return false; }
    return true;
}

bool FCoastalDestinationQuestRules::ValidateSavedManifest(const TArray<FCoastalWorldRecord>& World,
    const TArray<FName>& Journal, bool bFirstSignalComplete, FString& Error)
{
    TSet<FName> Seen;
    TSet<FName> Entries;
    for (FName Entry : Journal)
    {
        const FString Id = Entry.ToString();
        if ((Id.StartsWith(TEXT("journal.north_reach.")) && Entry != TEXT("journal.north_reach.lead")
                && !IsDestinationJournalId(Entry))
            || (Id.StartsWith(TEXT("journal.coastal_records.")) && !IsDestinationJournalId(Entry)))
        { Error = TEXT("Unknown destination journal entry."); return false; }
        Entries.Add(Entry);
    }
    TArray<bool> OptionalPresent;
    OptionalPresent.Init(false, DestinationStepCount);
    for (const FCoastalWorldRecord& Record : World)
    {
        if (Seen.Contains(Record.WorldId))
        { Error = TEXT("Duplicate saved world ID."); return false; }
        Seen.Add(Record.WorldId);
        ECoastalObjectKind RequiredKind;
        if (IsRequiredWorldId(Record.WorldId, &RequiredKind))
        {
            if (Record.Kind != RequiredKind)
            { Error = TEXT("Required First Signal record has the wrong kind."); return false; }
            continue;
        }
        int32 Index = INDEX_NONE;
        if (!IsOptionalWorldId(Record.WorldId, &Index))
        { Error = TEXT("Saved world record is not in the approved manifest."); return false; }
        if (Record.Kind != ECoastalObjectKind::Discovery)
        { Error = TEXT("Destination record must remain a discovery."); return false; }
        OptionalPresent[Index] = true;
        if (Record.bActive != Entries.Contains(DestinationSteps()[Index].JournalId))
        { Error = TEXT("Destination discovery state and journal disagree."); return false; }
    }
    for (const FRequiredRecord& Required : RequiredRecords())
        if (!Seen.Contains(Required.WorldId))
        { Error = TEXT("A required First Signal world record is missing."); return false; }
    bool bPreviousComplete = bFirstSignalComplete && HasCoherentFirstSignalAcknowledgement(Journal);
    for (int32 Index = 0; Index < DestinationStepCount; ++Index)
    {
        const bool bInJournal = Entries.Contains(DestinationSteps()[Index].JournalId);
        if (bInJournal && !OptionalPresent[Index])
        { Error = TEXT("Destination journal progress has no matching saved record."); return false; }
        if (bInJournal && !bPreviousComplete)
        { Error = TEXT("Destination records were completed out of order."); return false; }
        bPreviousComplete = bInJournal;
    }
    return true;
}

bool FCoastalDestinationQuestRules::CanRead(FName WorldId, bool bFirstSignalComplete,
    const TArray<FName>& Journal)
{
    int32 Index = INDEX_NONE;
    if (!IsOptionalWorldId(WorldId, &Index)) return true;
    if (!bFirstSignalComplete || !HasCoherentFirstSignalAcknowledgement(Journal)) return false;
    return Index == 0 || Journal.Contains(DestinationSteps()[Index - 1].JournalId);
}

int32 FCoastalDestinationQuestRules::NextStep(bool bFirstSignalComplete, const TArray<FName>& Journal)
{
    if (!bFirstSignalComplete || !HasCoherentFirstSignalAcknowledgement(Journal)) return INDEX_NONE;
    for (int32 Index = 0; Index < DestinationStepCount; ++Index)
        if (!Journal.Contains(DestinationSteps()[Index].JournalId)) return Index;
    return DestinationStepCount;
}

ECoastalRestoreRecordResolution FCoastalDestinationQuestRules::ResolveRestoreState(FName LiveWorldId,
    const TMap<FName, const FCoastalWorldRecord*>& SavedRecords, bool& bOutActive)
{
    if (const FCoastalWorldRecord* const* Record = SavedRecords.Find(LiveWorldId))
    {
        bOutActive = (*Record)->bActive;
        return ECoastalRestoreRecordResolution::Saved;
    }
    if (IsOptionalWorldId(LiveWorldId))
    {
        bOutActive = false;
        return ECoastalRestoreRecordResolution::ResetOptional;
    }
    return ECoastalRestoreRecordResolution::Reject;
}
