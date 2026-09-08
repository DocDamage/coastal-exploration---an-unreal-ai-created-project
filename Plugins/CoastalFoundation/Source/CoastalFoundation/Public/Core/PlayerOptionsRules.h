#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace coastal
{
    // Integer percentages make on-disk values deterministic; no float/NaN profile fields.
    struct PlayerOptions
    {
        int mousePercent = 100, stickPercent = 100, fieldOfView = 85, textPercent = 100;
        bool invertY = false;
        bool sprintToggle = false;
        int masterPercent = 100, ambiencePercent = 100, effectsPercent = 100, radioPercent = 100;
    };
    inline bool operator==(const PlayerOptions& a, const PlayerOptions& b)
    {
        return a.mousePercent == b.mousePercent && a.stickPercent == b.stickPercent
            && a.fieldOfView == b.fieldOfView && a.textPercent == b.textPercent && a.invertY == b.invertY && a.sprintToggle == b.sprintToggle
            && a.masterPercent == b.masterPercent && a.ambiencePercent == b.ambiencePercent
            && a.effectsPercent == b.effectsPercent && a.radioPercent == b.radioPercent;
    }
    inline bool operator!=(const PlayerOptions& a, const PlayerOptions& b) { return !(a == b); }
    inline bool ValidVolumePercent(int value) { return value >= 0 && value <= 100 && value % 5 == 0; }
    inline bool ValidOptions(const PlayerOptions& p)
    {
        return p.mousePercent >= 25 && p.mousePercent <= 300 && p.mousePercent % 25 == 0
            && p.stickPercent >= 25 && p.stickPercent <= 300 && p.stickPercent % 25 == 0
            && p.fieldOfView >= 70 && p.fieldOfView <= 110 && p.fieldOfView % 5 == 0
            && p.textPercent >= 100 && p.textPercent <= 150 && p.textPercent % 10 == 0
            && ValidVolumePercent(p.masterPercent) && ValidVolumePercent(p.ambiencePercent)
            && ValidVolumePercent(p.effectsPercent) && ValidVolumePercent(p.radioPercent);
    }
    enum class OptionField { Mouse, Stick, FieldOfView, Text, InvertY, SprintMode, Master, Ambience, Effects, Radio };
    constexpr int OptionFieldCount = 10;
    inline bool AdjustOption(PlayerOptions& p, OptionField field, int direction)
    {
        if (!ValidOptions(p) || (direction != -1 && direction != 1)) return false;
        const auto before = p;
        switch (field)
        {
        case OptionField::Mouse: p.mousePercent = std::clamp(p.mousePercent + direction * 25, 25, 300); break;
        case OptionField::Stick: p.stickPercent = std::clamp(p.stickPercent + direction * 25, 25, 300); break;
        case OptionField::FieldOfView: p.fieldOfView = std::clamp(p.fieldOfView + direction * 5, 70, 110); break;
        case OptionField::Text: p.textPercent = std::clamp(p.textPercent + direction * 10, 100, 150); break;
        case OptionField::InvertY: p.invertY = !p.invertY; break;
        case OptionField::SprintMode: p.sprintToggle = !p.sprintToggle; break;
        case OptionField::Master: p.masterPercent = std::clamp(p.masterPercent + direction * 5, 0, 100); break;
        case OptionField::Ambience: p.ambiencePercent = std::clamp(p.ambiencePercent + direction * 5, 0, 100); break;
        case OptionField::Effects: p.effectsPercent = std::clamp(p.effectsPercent + direction * 5, 0, 100); break;
        case OptionField::Radio: p.radioPercent = std::clamp(p.radioPercent + direction * 5, 0, 100); break;
        default: return false;
        }
        return p != before;
    }
    inline int OptionFontSize(int base, int percent)
    {
        if (base < 1 || base > 100 || percent < 100 || percent > 150) return base;
        return (base * percent + 50) / 100;
    }
    struct LookDelta { double yaw = 0, pitch = 0; };
    inline bool ValidLook(double x, double y) { return std::isfinite(x) && std::isfinite(y); }
    inline LookDelta MouseLook(const PlayerOptions& p, double x, double y)
    {
        // Mouse is a delta, never multiplied by frame time. Bound unreasonable host samples.
        if (!ValidOptions(p) || !ValidLook(x, y) || std::abs(x) > 10000 || std::abs(y) > 10000) return {};
        const double scale = p.mousePercent / 100.0;
        return {x * scale, y * scale * (p.invertY ? -1 : 1)};
    }
    inline LookDelta StickLook(const PlayerOptions& p, double x, double y, double seconds)
    {
        // Host supplies mapped, dead-zoned normalized axes. No second dead zone or acceleration.
        if (!ValidOptions(p) || !ValidLook(x, y) || !std::isfinite(seconds) || seconds <= 0) return {};
        constexpr double BaseStickRate = 90.0; // original tuning, input units per second at 100%
        const double scale = BaseStickRate * p.stickPercent / 100.0 * std::min(seconds, 0.1);
        return {std::clamp(x, -1.0, 1.0) * scale,
            std::clamp(y, -1.0, 1.0) * scale * (p.invertY ? -1 : 1)};
    }
    // Host must supply actual neutral/released stick samples, not a synthetic release on menu close.
    class LookInputGate
    {
        std::uint64_t epoch_ = 0, revision_ = 0, enabledFrame_ = 0, mouseFrame_ = 0, stickFrame_ = 0;
        bool known_ = false, allowed_ = false, neutral_ = false, mouseSeen_ = false, stickSeen_ = false;
    public:
        void Synchronize(bool allowed, std::uint64_t epoch, std::uint64_t revision, std::uint64_t frame)
        {
            if (!known_ || allowed != allowed_ || epoch != epoch_ || revision != revision_)
            { known_ = true; allowed_ = allowed; epoch_ = epoch; revision_ = revision; enabledFrame_ = frame; neutral_ = false; }
        }
        bool Mouse(std::uint64_t frame)
        {
            if (!allowed_ || frame <= enabledFrame_ || (mouseSeen_ && frame <= mouseFrame_)) return false;
            mouseSeen_ = true; mouseFrame_ = frame; return true;
        }
        bool Stick(double x, double y, std::uint64_t frame)
        {
            if (!allowed_ || !ValidLook(x, y) || frame <= enabledFrame_
                || (stickSeen_ && frame <= stickFrame_)) return false;
            stickSeen_ = true; stickFrame_ = frame;
            // Only an actually neutral logical axis rearms. No aim drift is treated as released.
            if (std::abs(x) <= 0.0001 && std::abs(y) <= 0.0001) { neutral_ = true; return false; }
            return neutral_;
        }
    };
}
