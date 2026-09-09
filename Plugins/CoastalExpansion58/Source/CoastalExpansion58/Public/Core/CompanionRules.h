#pragma once

#include <cmath>

namespace coastal
{
enum class CompanionDisposition
{
    Dormant,
    Hold,
    Follow,
    Regroup
};

enum class CompanionPace
{
    Idle,
    Walk,
    Run,
    Catchup
};

struct CompanionFollowBands
{
    double stopCm = 155.0;
    double runCm = 650.0;
    double catchupCm = 2400.0;

    bool Valid() const
    {
        return std::isfinite(stopCm) && std::isfinite(runCm) && std::isfinite(catchupCm)
            && stopCm >= 75.0 && runCm > stopCm && catchupCm > runCm;
    }
};

struct CompanionLifecycleSample
{
    bool configured = false;
    bool activeCampaign = false;
    bool followRequested = true;
    bool epochMatches = false;
    bool saveBusy = false;
    bool recoveryRequired = false;
    bool returning = false;
    bool paused = false;
    bool needsRegroup = false;
};

inline CompanionDisposition CompanionDecision(const CompanionLifecycleSample& sample)
{
    if (!sample.configured || !sample.activeCampaign) return CompanionDisposition::Dormant;
    if (!sample.epochMatches || sample.recoveryRequired || sample.returning)
        return CompanionDisposition::Regroup;
    if (!sample.followRequested || sample.saveBusy || sample.paused) return CompanionDisposition::Hold;
    if (sample.needsRegroup) return CompanionDisposition::Regroup;
    return CompanionDisposition::Follow;
}

inline CompanionPace CompanionPaceForDistance(double distanceCm, const CompanionFollowBands& bands)
{
    if (!bands.Valid() || !std::isfinite(distanceCm) || distanceCm < 0.0)
        return CompanionPace::Idle;
    if (distanceCm <= bands.stopCm) return CompanionPace::Idle;
    if (distanceCm >= bands.catchupCm) return CompanionPace::Catchup;
    if (distanceCm >= bands.runCm) return CompanionPace::Run;
    return CompanionPace::Walk;
}
}
