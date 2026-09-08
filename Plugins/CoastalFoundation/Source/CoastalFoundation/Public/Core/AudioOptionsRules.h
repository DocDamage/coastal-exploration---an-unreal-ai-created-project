#pragma once
#include "PlayerOptionsRules.h"
#include <array>

namespace coastal
{
    constexpr double AudioOptionFadeSeconds = 0.1; // Original tuning; startup uses zero fade.
    inline bool IsAudioOption(OptionField field)
    { return field == OptionField::Master || field == OptionField::Ambience || field == OptionField::Effects || field == OptionField::Radio; }
    inline bool SameAudioOptions(const PlayerOptions& a, const PlayerOptions& b)
    {
        return a.masterPercent == b.masterPercent && a.ambiencePercent == b.ambiencePercent
            && a.effectsPercent == b.effectsPercent && a.radioPercent == b.radioPercent;
    }
    inline void CopyAudioOptions(PlayerOptions& destination, const PlayerOptions& source)
    {
        destination.masterPercent = source.masterPercent; destination.ambiencePercent = source.ambiencePercent;
        destination.effectsPercent = source.effectsPercent; destination.radioPercent = source.radioPercent;
    }
    using AudioGains = std::array<double, 3>; // Ambience, Effects, Radio. No parent-class multiplier.
    inline bool ValidAudioGains(const AudioGains& gains)
    {
        for (double gain : gains) if (!std::isfinite(gain) || gain < 0 || gain > 1) return false;
        return true;
    }
    inline bool BuildAudioGains(const PlayerOptions& p, AudioGains& out)
    {
        out = {};
        if (!ValidOptions(p)) return false;
        out = {p.masterPercent * p.ambiencePercent / 10000.0,
               p.masterPercent * p.effectsPercent / 10000.0,
               p.masterPercent * p.radioPercent / 10000.0};
        return true;
    }
    // Read-only metadata for three isolated, project-owned sound classes. No guessed asset paths.
    struct AudioClassInfo
    {
        std::uint64_t identity = 0;
        bool isolated = false; // no parent, children or passive modifiers
        double authoredVolume = 1, authoredPitch = 1;
    };
    inline bool ValidAudioClasses(const std::array<AudioClassInfo, 3>& classes)
    {
        for (std::size_t i = 0; i < classes.size(); ++i)
        {
            const auto& c = classes[i];
            if (!c.identity || !c.isolated || !std::isfinite(c.authoredVolume) || c.authoredVolume < 0
                || c.authoredVolume > 1 || !std::isfinite(c.authoredPitch) || c.authoredPitch <= 0
                || c.authoredPitch > 4) return false;
            for (std::size_t j = 0; j < i; ++j) if (c.identity == classes[j].identity) return false;
        }
        return true;
    }
    // Tracks commands submitted to the native device, NOT an audio-thread acknowledgement.
    // Runtime creates one private mix and never repushes it to change a volume.
    class AudioMixSession
    {
        enum class State { Idle, Active, Stopped } state_ = State::Idle;
        AudioGains last_{};
    public:
        bool Active() const { return state_ == State::Active; }
        bool Begin(const AudioGains& gains)
        {
            if (state_ != State::Idle || !ValidAudioGains(gains)) return false;
            last_ = gains; state_ = State::Active; return true;
        }
        bool NeedsUpdate(const AudioGains& gains) const
        { return Active() && ValidAudioGains(gains) && gains != last_; }
        bool Submitted(const AudioGains& gains)
        {
            if (!Active() || !ValidAudioGains(gains)) return false;
            last_ = gains; return true;
        }
        bool Release()
        {
            const bool owned = Active(); state_ = State::Stopped; return owned;
        }
    };
}
