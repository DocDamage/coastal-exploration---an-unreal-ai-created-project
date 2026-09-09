#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace coastal
{
    // UI-detail presentation only. These values are degrees, not input axes or
    // world transforms. The caller owns input scaling and fresh item validation.
    constexpr float PreviewPitchMinimum = -70.0f;
    constexpr float PreviewPitchMaximum = 70.0f;

    inline bool ValidPreviewRevision(std::int64_t revision)
    { return revision >= 0; }

    inline bool ValidPreviewDelta(float yawDegrees, float pitchDegrees)
    { return std::isfinite(yawDegrees) && std::isfinite(pitchDegrees); }

    inline float WrapPreviewYaw(float value)
    {
        if (!std::isfinite(value)) return 0.0f;
        value = std::fmod(value, 360.0f);
        return value < 0.0f ? value + 360.0f : value;
    }

    struct ItemPreviewPose
    {
        float yaw = 0.0f;
        float pitch = 0.0f;
        bool Apply(float yawDegrees, float pitchDegrees)
        {
            if (!ValidPreviewDelta(yawDegrees, pitchDegrees)) return false;
            yaw = WrapPreviewYaw(yaw + yawDegrees);
            pitch = std::clamp(pitch + pitchDegrees, PreviewPitchMinimum, PreviewPitchMaximum);
            return true;
        }
    };
}
