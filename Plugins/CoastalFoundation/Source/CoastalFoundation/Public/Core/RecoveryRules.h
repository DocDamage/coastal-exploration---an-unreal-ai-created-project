#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace coastal
{
    // Original project timings in game seconds; no async work or inventory ownership.
    constexpr double ReturnFadeOut = 0.18;
    constexpr double ReturnFadeIn = 0.22;
    constexpr double ReturnStableTime = 0.35;
    constexpr double ReturnSettleLimit = 2.0;
    constexpr double CheckpointDwell = 0.35;
    enum class ReturnPhase { Idle, FadingOut, Placement, FadingIn, Settling, Failed, Stopped };
    class ReturnFlow
    {
        ReturnPhase phase_ = ReturnPhase::Idle;
        std::uint64_t epoch_ = 0;
        double elapsed_ = 0, stable_ = 0;
    public:
        ReturnPhase Phase() const { return phase_; }
        bool Active() const
        { return phase_ != ReturnPhase::Idle && phase_ != ReturnPhase::Failed && phase_ != ReturnPhase::Stopped; }
        bool Begin(std::uint64_t epoch)
        {
            if (phase_ != ReturnPhase::Idle || epoch == 0) return false;
            epoch_ = epoch; elapsed_ = stable_ = 0; phase_ = ReturnPhase::FadingOut; return true;
        }
        bool Matches(std::uint64_t epoch) const { return Active() && epoch == epoch_; }
        bool Advance(double dt, bool grounded)
        {
            if (!Active() || !std::isfinite(dt) || dt < 0) return false;
            elapsed_ += dt;
            if (phase_ == ReturnPhase::FadingOut && elapsed_ >= ReturnFadeOut)
            { phase_ = ReturnPhase::Placement; elapsed_ = 0; }
            else if (phase_ == ReturnPhase::FadingIn && elapsed_ >= ReturnFadeIn)
            { phase_ = ReturnPhase::Settling; elapsed_ = stable_ = 0; }
            else if (phase_ == ReturnPhase::Settling)
            {
                stable_ = grounded ? stable_ + dt : 0;
                // A stalled frame cannot be mistaken for observed stable grounding.
                if (dt > 0.25) stable_ = 0;
                if (elapsed_ >= ReturnSettleLimit && stable_ < ReturnStableTime) Fail();
            }
            return true;
        }
        bool Placed()
        {
            if (phase_ != ReturnPhase::Placement) return false;
            phase_ = ReturnPhase::FadingIn; elapsed_ = 0; return true;
        }
        bool CanFinish() const { return phase_ == ReturnPhase::Settling && stable_ >= ReturnStableTime; }
        bool Finish()
        {
            if (!CanFinish()) return false;
            phase_ = ReturnPhase::Idle; elapsed_ = stable_ = 0; return true;
        }
        double Opacity() const
        {
            if (phase_ == ReturnPhase::FadingOut) return std::min(1.0, elapsed_ / ReturnFadeOut);
            if (phase_ == ReturnPhase::Placement) return 1.0;
            if (phase_ == ReturnPhase::FadingIn) return std::max(0.0, 1.0 - elapsed_ / ReturnFadeIn);
            return 0.0;
        }
        void Fail() { if (phase_ != ReturnPhase::Stopped) { phase_ = ReturnPhase::Failed; elapsed_ = stable_ = 0; } }
        void Stop() { phase_ = ReturnPhase::Stopped; }
    };
    // Callback must revalidate the candidate and the actual teleport result.
    // Returning an index is not an engine teleport; native source supplies that operation.
    template <typename Attempt>
    int TryReturnCandidates(bool distinctFallback, Attempt attempt)
    {
        if (attempt(0)) return 0;
        if (distinctFallback && attempt(1)) return 1;
        return -1;
    }
    class CheckpointVisit
    {
        std::uint64_t candidate_ = 0;
        std::uint64_t recordedCandidate_ = 0;
        double dwell_ = 0;
    public:
        // A session/real visit boundary permits the checkpoint to be recorded again.
        void Reset() { candidate_ = recordedCandidate_ = 0; dwell_ = 0; }
        // Menus, save ownership and hitches break dwell without forgetting a
        // checkpoint already recorded during the current visit.
        void Interrupted() { candidate_ = 0; dwell_ = 0; }
        bool Observe(std::uint64_t candidate, bool eligible, double dt)
        {
            if (!candidate || !eligible || !std::isfinite(dt) || dt < 0)
            { Reset(); return false; }
            if (dt > 0.25) { Interrupted(); return false; }
            if (candidate != candidate_) { candidate_ = candidate; dwell_ = 0; }
            dwell_ += dt;
            return candidate != recordedCandidate_ && dwell_ >= CheckpointDwell;
        }
        void Recorded() { recordedCandidate_ = candidate_; }
    };
    // Exact capsule/box overlap for an upright capsule and a yaw-only box, in cm.
    // The capsule segment extends halfHeight-radius above/below its center.
    inline bool ValidSafetyBox(double ex, double ey, double ez)
    {
        return std::isfinite(ex) && std::isfinite(ey) && std::isfinite(ez)
            && ex > 0 && ey > 0 && ez > 0 && ex <= 1000000 && ey <= 1000000 && ez <= 1000000;
    }
    inline bool SafetyCapsuleOverlap(double x, double y, double z,
        double radius, double halfHeight, double ex, double ey, double ez)
    {
        if (!ValidSafetyBox(ex, ey, ez) || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)
            || !std::isfinite(radius) || !std::isfinite(halfHeight) || radius <= 0 || halfHeight < radius
            || halfHeight > 1000000) return false;
        const double dx = std::max(0.0, std::abs(x) - ex);
        const double dy = std::max(0.0, std::abs(y) - ey);
        const double dz = std::max(0.0, std::abs(z) - ez - (halfHeight - radius));
        return dx * dx + dy * dy + dz * dz <= radius * radius;
    }
}
