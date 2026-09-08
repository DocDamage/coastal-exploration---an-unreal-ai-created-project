#pragma once

namespace coastal
{
enum class SwimmingDecision
{
    None,
    Enter,
    Maintain,
    ExitWater,
    CancelInterrupted,
    CancelUnsafe
};

struct SwimmingSample
{
    bool initialized = false;
    bool binding_valid = false;
    bool zone_valid = false;
    bool within_recovery_depth = false;
    bool movement_is_swimming = false;
    bool campaign_ready = false;
    bool world_input_allowed = false;
    bool world_paused = false;
    bool same_session = true;
};

inline SwimmingDecision EvaluateSwimming(bool active, const SwimmingSample& sample)
{
    if (active)
    {
        if (!sample.initialized || !sample.binding_valid || !sample.zone_valid
            || !sample.within_recovery_depth || !sample.same_session)
            return SwimmingDecision::CancelUnsafe;
        if (!sample.campaign_ready || !sample.world_input_allowed || sample.world_paused)
            return SwimmingDecision::CancelInterrupted;
        if (!sample.movement_is_swimming) return SwimmingDecision::ExitWater;
        return SwimmingDecision::Maintain;
    }
    if (sample.initialized && sample.binding_valid && sample.zone_valid
        && sample.within_recovery_depth && sample.movement_is_swimming
        && sample.campaign_ready && sample.world_input_allowed
        && !sample.world_paused && sample.same_session)
        return SwimmingDecision::Enter;
    return SwimmingDecision::None;
}

inline bool AllowsDeepWaterRecoveryExemption(bool active, const SwimmingSample& sample)
{
    return active && sample.initialized && sample.binding_valid && sample.zone_valid
        && sample.within_recovery_depth && sample.movement_is_swimming
        && sample.campaign_ready && sample.world_input_allowed
        && !sample.world_paused && sample.same_session;
}

inline bool AllowsZoneHandoff(bool active, bool different_zone,
    const SwimmingSample& candidate)
{
    return active && different_zone
        && EvaluateSwimming(true, candidate) == SwimmingDecision::Maintain;
}

inline bool AllowsDefaultZoneFallback(bool active, bool current_is_default,
    bool candidate_valid, bool capsule_overlaps_candidate)
{
    return active && current_is_default && candidate_valid && capsule_overlaps_candidate;
}

inline bool HasRequiredVolumeCollision(bool overlap_all_dynamic, bool query_only,
    bool generate_overlap_events, bool pawn_overlaps)
{
    return overlap_all_dynamic && query_only && generate_overlap_events && pawn_overlaps;
}
}
