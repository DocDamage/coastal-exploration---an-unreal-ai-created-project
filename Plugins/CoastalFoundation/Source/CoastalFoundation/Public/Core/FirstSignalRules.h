#pragma once

// Original engine-independent rules. The inventory remains external and authoritative.
namespace coastal
{
    enum class Phase { InspectRadio, FindEquipment, ReturnToRadio, ListenToRadio, Complete };
    enum class CommitResult { Committed, AlreadyCommitted, MissingItems, NotConfigured, Failed };
    enum class ListenResult { Applied, AlreadyApplied, NotRepaired };

    struct Facts
    {
        bool cabinVisited = false;
        bool radioInspected = false;
        bool dockDiscovered = false;
        bool maintenanceNoteRead = false;
        bool radioRepaired = false;
        bool messageHeard = false;
    };

    constexpr bool IsValid(const Facts& facts)
    {
        return (!facts.radioRepaired || facts.radioInspected)
            && (!facts.messageHeard || facts.radioRepaired);
    }

    // Equipment may be found first. Notes and area triggers never gate progression.
    constexpr Phase CurrentPhase(const Facts& facts, bool hasRequiredEquipment)
    {
        if (facts.messageHeard) return Phase::Complete;
        if (facts.radioRepaired) return Phase::ListenToRadio;
        if (!facts.radioInspected) return Phase::InspectRadio;
        return hasRequiredEquipment ? Phase::ReturnToRadio : Phase::FindEquipment;
    }

    // Call only after the external, atomic, idempotent inventory operation returns.
    // This function itself does not consume items or claim durable persistence.
    inline bool ApplyRepairCommit(Facts& facts, CommitResult result)
    {
        if (facts.radioRepaired) return false;
        if (result != CommitResult::Committed && result != CommitResult::AlreadyCommitted)
            return false;
        facts.radioInspected = true;
        facts.radioRepaired = true;
        return true;
    }

    inline ListenResult FinishTransmission(Facts& facts)
    {
        if (!facts.radioRepaired) return ListenResult::NotRepaired;
        if (facts.messageHeard) return ListenResult::AlreadyApplied;
        facts.messageHeard = true;
        return ListenResult::Applied;
    }
}
