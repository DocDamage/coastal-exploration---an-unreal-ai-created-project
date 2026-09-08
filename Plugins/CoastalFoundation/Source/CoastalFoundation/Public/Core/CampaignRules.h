#pragma once
#include "FirstSignalRules.h"
#include <cstdint>
#include <limits>

namespace coastal
{
    enum class SlotStatus { Missing, Valid, Corrupt, Incompatible };
    struct SlotInfo
    {
        SlotStatus status = SlotStatus::Missing;
        std::int64_t generation = 0;
    };
    struct SlotChoice
    {
        int index = -1;
        bool recovered = false;
        bool blocked = false;
    };
    // Do not silently overwrite a future schema or resolve a tied generation.
    inline SlotChoice ChooseSlot(SlotInfo a, SlotInfo b)
    {
        if (a.status == SlotStatus::Incompatible || b.status == SlotStatus::Incompatible)
            return {-1, false, true};
        if ((a.status == SlotStatus::Valid && a.generation <= 0)
            || (b.status == SlotStatus::Valid && b.generation <= 0)) return {-1, false, true};
        const bool av = a.status == SlotStatus::Valid;
        const bool bv = b.status == SlotStatus::Valid;
        if (av && bv)
        {
            if (a.generation == b.generation) return {-1, false, true};
            return {a.generation > b.generation ? 0 : 1, false, false};
        }
        if (av) return {0, b.status == SlotStatus::Corrupt, false};
        if (bv) return {1, a.status == SlotStatus::Corrupt, false};
        return {-1, false, false};
    }
    inline bool NextGeneration(std::int64_t current, std::int64_t& next)
    {
        if (current < 0 || current == std::numeric_limits<std::int64_t>::max()) return false;
        next = current + 1;
        return true;
    }
    // Shared by runtime and standalone tests. Non-nestable game-thread boundary.
    class OperationGate
    {
        bool mutation_ = false, io_ = false, recovery_ = false, poisoned_ = false, pending_ = false;
    public:
        bool BeginMutation()
        {
            if (Busy()) return false;
            mutation_ = true;
            return true;
        }
        void EndMutation() { mutation_ = false; }
        bool BeginIO()
        {
            if (Busy()) return false;
            io_ = true;
            return true;
        }
        void EndIO() { io_ = false; }
        bool BeginRecovery()
        {
            if (Busy()) return false;
            recovery_ = true; return true;
        }
        void EndRecovery() { recovery_ = false; }
        bool IsRecovery() const { return recovery_; }
        void RequestSave() { if (!poisoned_) pending_ = true; }
        bool TakeSaveRequest()
        {
            if (Busy() || !pending_) return false;
            pending_ = false;
            return true;
        }
        bool Busy() const { return mutation_ || io_ || recovery_ || poisoned_; }
        bool IsIO() const { return io_; }
        bool Poisoned() const { return poisoned_; }
        void Poison() { poisoned_ = true; pending_ = false; }
        void ClearPending() { pending_ = false; }
    };
    inline bool CoherentMission(const Facts& facts, bool repairReceipt,
        bool transmissionEntry, bool leadEntry, bool maintenanceEntry)
    {
        return IsValid(facts) && facts.radioRepaired == repairReceipt
            && facts.messageHeard == transmissionEntry && facts.messageHeard == leadEntry
            && facts.maintenanceNoteRead == maintenanceEntry;
    }
}
