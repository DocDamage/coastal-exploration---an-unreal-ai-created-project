#pragma once
#include <cmath>
#include <cstdint>

namespace coastal
{
    struct SprintTuning { double walk = 330.0, sprint = 520.0; };
    inline bool ValidSprintTuning(const SprintTuning& t)
    {
        return std::isfinite(t.walk) && std::isfinite(t.sprint)
            && t.walk >= 100 && t.walk <= 1000 && t.sprint >= t.walk && t.sprint <= 1500;
    }
    constexpr std::uint64_t SprintSampleLeaseFrames = 2;
    // The host supplies one real aggregate down/up sample per frame, never a fabricated release.
    // Toggle selects a speed cap; it never creates movement input or an auto-run direction.
    class SprintGate
    {
        std::uint64_t epoch_ = 0, revision_ = 0, enabledFrame_ = 0, sampleFrame_ = 0, syncFrame_ = 0;
        bool known_ = false, allowed_ = false, toggle_ = false, sampled_ = false;
        bool armed_ = false, wasDown_ = false, requested_ = false;
        void Reset(std::uint64_t frame)
        { enabledFrame_ = frame; sampled_ = false; armed_ = false; wasDown_ = false; requested_ = false; }
    public:
        void Synchronize(bool allowed, std::uint64_t epoch, std::uint64_t revision,
                         bool toggle, std::uint64_t frame)
        {
            const bool expired = sampled_ && (frame < sampleFrame_ || frame - sampleFrame_ > SprintSampleLeaseFrames);
            if (!known_ || allowed != allowed_ || epoch != epoch_ || revision != revision_
                || toggle != toggle_ || (known_ && frame < syncFrame_) || expired) Reset(frame);
            known_ = true; allowed_ = allowed; epoch_ = epoch; revision_ = revision;
            toggle_ = toggle; syncFrame_ = frame;
        }
        bool Sample(bool down, std::uint64_t frame)
        {
            if (!known_ || !allowed_ || frame <= enabledFrame_ || frame < syncFrame_
                || (sampled_ && frame <= sampleFrame_)) return false;
            sampleFrame_ = frame; sampled_ = true;
            if (!down)
            { armed_ = true; wasDown_ = false; if (!toggle_) requested_ = false; return true; }
            if (armed_ && !wasDown_) requested_ = toggle_ ? !requested_ : true;
            wasDown_ = true; return true;
        }
        bool Requested(std::uint64_t frame) const
        {
            return known_ && allowed_ && sampled_ && requested_ && frame >= sampleFrame_
                && frame - sampleFrame_ <= SprintSampleLeaseFrames;
        }
    };
    // Sole ownership of one native speed property. A foreign edit ends ownership, not a tug-of-war.
    class WalkSpeedLease
    {
        SprintTuning tuning_;
        double original_ = 0, applied_ = 0;
        bool owned_ = false;
        bool Matches(double current) const
        { return owned_ && std::isfinite(current) && std::abs(current - applied_) <= 0.0001; }
    public:
        bool Acquire(double current, const SprintTuning& tuning)
        {
            if (owned_ || !std::isfinite(current) || current < 0 || !ValidSprintTuning(tuning)) return false;
            tuning_ = tuning; original_ = applied_ = current; owned_ = true; return true;
        }
        bool Apply(double current, bool sprint, double& next)
        {
            next = current;
            if (!Matches(current)) { owned_ = false; return false; }
            next = sprint ? tuning_.sprint : tuning_.walk; applied_ = next; return true;
        }
        bool Release(double current, double& next)
        {
            next = current; const bool restore = Matches(current);
            if (restore) next = original_;
            owned_ = false; return restore;
        }
        bool Owns(double current) const { return Matches(current); }
    };
}
