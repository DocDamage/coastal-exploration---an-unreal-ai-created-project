#pragma once
#include "UIFlowRules.h"

namespace coastal
{
    constexpr double MaximumRadioSeconds = 900.0; // Authored-source guard, not a mission timer.
    struct PlaybackSourceInfo
    {
        bool playable = false, looping = false, playWhenSilent = false;
        double duration = 0;
    };
    inline bool ValidAmbienceSource(const PlaybackSourceInfo& source)
    { return source.playable && source.looping && source.playWhenSilent; }
    inline bool ValidRadioSource(const PlaybackSourceInfo& source)
    {
        return source.playable && !source.looping && source.playWhenSilent
            && std::isfinite(source.duration) && source.duration > 0 && source.duration <= MaximumRadioSeconds;
    }
    // Read-only projection from the ONE native UI. No save, item, audio-finished or quest input.
    struct AudioPlaybackContext
    {
        std::uint64_t epoch = 0;
        bool active = false, interrupted = true, ambienceAllowed = false;
        PanelTicket transcript;
        bool presented = false;
    };
    struct AudioPlaybackCommands
    {
        bool stopAmbience = false, startAmbience = false, pauseAmbience = false, resumeAmbience = false;
        bool stopRadio = false, startRadio = false;
        bool Empty() const
        { return !stopAmbience && !startAmbience && !pauseAmbience && !resumeAmbience && !stopRadio && !startRadio; }
    };
    // Tracks submitted intentions, NOT audible output. Never retries a natural finish,
    // dropped voice, allocation failure, or mute. UI ticket IDs increase across epoch resets.
    class AudioPlaybackSession
    {
        bool initialized_ = false, stopped_ = false, hasAmbience_ = false, hasRadio_ = false;
        bool ambienceAttempted_ = false, ambienceOwned_ = false, ambiencePaused_ = false;
        bool radioAttempted_ = false, radioOwned_ = false;
        std::uint64_t epoch_ = 0, ticket_ = 0, retiredTicket_ = 0;
        void RetireRadio(AudioPlaybackCommands& commands)
        {
            commands.stopRadio |= radioOwned_;
            retiredTicket_ = std::max(retiredTicket_, ticket_);
            ticket_ = 0; radioOwned_ = false; radioAttempted_ = false;
        }
        void StopAmbience(AudioPlaybackCommands& commands)
        {
            commands.stopAmbience |= ambienceOwned_;
            ambienceOwned_ = false; ambiencePaused_ = false;
        }
        void PauseAmbience(AudioPlaybackCommands& commands)
        {
            if (ambienceOwned_ && !ambiencePaused_)
            { commands.pauseAmbience = true; ambiencePaused_ = true; }
        }
    public:
        bool Begin(bool hasAmbience, bool hasRadio)
        {
            if (initialized_ || stopped_ || (!hasAmbience && !hasRadio)) return false;
            hasAmbience_ = hasAmbience; hasRadio_ = hasRadio; initialized_ = true; return true;
        }
        bool Active() const { return initialized_ && !stopped_; }
        AudioPlaybackCommands Suspend()
        {
            AudioPlaybackCommands commands;
            if (Active()) { RetireRadio(commands); PauseAmbience(commands); }
            return commands;
        }
        AudioPlaybackCommands Step(const AudioPlaybackContext& context)
        {
            AudioPlaybackCommands commands;
            if (!Active()) return commands;
            // A stale projection must not reset the current session or resurrect retired text.
            if (context.epoch < epoch_) return Suspend();
            if (context.epoch != epoch_)
            {
                RetireRadio(commands); StopAmbience(commands);
                epoch_ = context.epoch; ambienceAttempted_ = false;
            }
            if (!context.active || !context.epoch)
            { RetireRadio(commands); StopAmbience(commands); return commands; }
            if (context.interrupted)
            { RetireRadio(commands); PauseAmbience(commands); return commands; }
            if (context.ambienceAllowed && hasAmbience_)
            {
                if (!ambienceAttempted_)
                {
                    ambienceAttempted_ = true; ambienceOwned_ = true;
                    commands.startAmbience = true;
                }
                else if (ambienceOwned_ && ambiencePaused_)
                { ambiencePaused_ = false; commands.resumeAmbience = true; }
            }
            else PauseAmbience(commands);
            const auto& candidate = context.transcript;
            const bool valid = hasRadio_ && candidate.id && candidate.epoch == epoch_
                && candidate.kind == PanelKind::Transcript && candidate.id > retiredTicket_;
            const std::uint64_t next = valid ? candidate.id : 0;
            if (next != ticket_)
            {
                RetireRadio(commands);
                // Recheck after retiring a newer pending/playing ticket.
                if (next > retiredTicket_) ticket_ = next;
            }
            if (ticket_ && next == ticket_ && context.presented && !radioAttempted_)
            {
                radioAttempted_ = true; radioOwned_ = true;
                commands.startRadio = true;
            }
            return commands;
        }
        AudioPlaybackCommands Release()
        {
            AudioPlaybackCommands commands;
            if (Active()) { RetireRadio(commands); StopAmbience(commands); }
            stopped_ = true; initialized_ = false; return commands;
        }
    };
}
