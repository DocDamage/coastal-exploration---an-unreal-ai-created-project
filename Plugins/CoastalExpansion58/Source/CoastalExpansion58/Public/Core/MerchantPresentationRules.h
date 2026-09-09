#pragma once

#include <cstdint>

namespace coastal
{
enum class MerchantPhase : std::uint8_t { Dormant, Idle, AwaitJournalClose, Pitching };

struct MerchantSample
{
    bool configured = false;
    bool campaign = false;
    bool epochMatches = false;
    bool paused = false;
    bool recovery = false;
    bool returning = false;
    bool busy = false;
    bool modal = false;
    bool unexpectedModal = false;
    bool worldInput = false;
    bool expectedModalSeen = false;
    bool clipFinished = false;
    bool queueExpired = false;
};

inline MerchantPhase AdvanceMerchant(MerchantPhase phase, const MerchantSample& sample)
{
    if (!sample.configured || !sample.campaign || !sample.epochMatches
        || sample.recovery || sample.returning)
        return MerchantPhase::Dormant;
    if (phase == MerchantPhase::Dormant) return MerchantPhase::Idle;
    if (phase == MerchantPhase::AwaitJournalClose)
    {
        if (sample.queueExpired || sample.unexpectedModal || (sample.paused && !sample.modal)) return MerchantPhase::Idle;
        if (!sample.expectedModalSeen || sample.modal) return MerchantPhase::AwaitJournalClose;
        return !sample.paused && !sample.busy && sample.worldInput
            ? MerchantPhase::Pitching : MerchantPhase::Idle;
    }
    if (phase == MerchantPhase::Pitching
        && (sample.paused || sample.busy || sample.modal || !sample.worldInput || sample.clipFinished))
        return MerchantPhase::Idle;
    return phase;
}
}
