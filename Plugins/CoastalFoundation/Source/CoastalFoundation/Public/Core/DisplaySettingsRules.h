#pragma once
#include "Core/UIFlowRules.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace coastal
{
    enum class DisplayWindowMode { Fullscreen, Borderless, Windowed };
    struct DisplayMode
    {
        int width = 0, height = 0;
        DisplayWindowMode window = DisplayWindowMode::Windowed;
    };
    inline bool operator==(DisplayMode a, DisplayMode b)
    { return a.width == b.width && a.height == b.height && a.window == b.window; }
    inline bool operator!=(DisplayMode a, DisplayMode b) { return !(a == b); }
    inline bool ValidDisplaySnapshot(DisplayMode m)
    {
        return m.width > 0 && m.height > 0 && m.width <= 32768 && m.height <= 32768
            && static_cast<unsigned>(m.window) <= static_cast<unsigned>(DisplayWindowMode::Windowed);
    }
    inline bool SelectableDisplayMode(DisplayMode m)
    { return ValidDisplaySnapshot(m) && m.width >= 1280 && m.height >= 720 && m.width <= 7680 && m.height <= 4320; }
    inline std::vector<DisplayMode> DisplayCatalogue(std::vector<DisplayMode> modes)
    {
        modes.erase(std::remove_if(modes.begin(), modes.end(), [](auto m) { return !SelectableDisplayMode(m); }), modes.end());
        std::sort(modes.begin(), modes.end(), [](auto a, auto b) {
            if (a.window != b.window) return a.window < b.window;
            if (a.width != b.width) return a.width < b.width;
            return a.height < b.height;
        });
        modes.erase(std::unique(modes.begin(), modes.end()), modes.end());
        // Refuse unreasonable native enumeration; never truncate into an apparently complete list.
        return modes.size() <= 512 ? modes : std::vector<DisplayMode>{};
    }
    enum class DisplayPhase { Idle, Testing, Reverting, Failed };
    enum class DisplayCommand { None, RequestCandidate, RequestRestore };
    struct DisplayObservation
    {
        double seconds = 0;
        std::uint64_t frame = 0;
        PanelTicket top;
        DisplayMode actual;
        bool ownerReady = false, foreground = false, presented = false;
    };
    // Only this exact state machine decides when native requests and confirmation are legal.
    // It cannot verify a monitor, a disk write, or a frozen/non-running host.
    class DisplayTrial
    {
        DisplayPhase phase_ = DisplayPhase::Idle;
        DisplayMode before_, proposed_;
        PanelTicket ticket_;
        std::uint64_t lastTicket_ = 0, begunFrame_ = 0, lastFrame_ = 0, settledFrame_ = 0;
        std::uint64_t presentationRevision_ = 0, restoreFrame_ = 0;
        double lastSeconds_ = 0, deadline_ = 0, restoreDeadline_ = 0;
        bool settled_ = false, restoreClockPending_ = false;
        bool Context(const DisplayObservation& o) const
        {
            return o.ownerReady && o.foreground && o.top.id == ticket_.id && o.top.epoch == ticket_.epoch
                && o.top.kind == PanelKind::ConfirmDisplay && std::isfinite(o.seconds)
                && o.seconds >= lastSeconds_ && o.frame >= lastFrame_;
        }
    public:
        static constexpr double TrialSeconds = 15.0, RestoreSeconds = 5.0;
        DisplayPhase Phase() const { return phase_; }
        bool Busy() const { return phase_ == DisplayPhase::Testing || phase_ == DisplayPhase::Reverting; }
        bool Settled() const { return settled_; }
        std::uint64_t PresentationRevision() const { return presentationRevision_; }
        DisplayMode Before() const { return before_; }
        DisplayMode Proposed() const { return proposed_; }
        PanelTicket Ticket() const { return ticket_; }
        double SecondsLeft(double now) const
        { return phase_ == DisplayPhase::Testing && std::isfinite(now) ? std::max(0.0, deadline_ - now) : 0; }
        DisplayCommand Begin(DisplayMode before, DisplayMode candidate, const std::vector<DisplayMode>& catalogue,
                             const DisplayObservation& o)
        {
            if (phase_ != DisplayPhase::Idle || !ValidDisplaySnapshot(before) || before == candidate
                || !SelectableDisplayMode(candidate) || std::find(catalogue.begin(), catalogue.end(), candidate) == catalogue.end()
                || !o.ownerReady || !o.foreground || o.actual != before || !o.top.id || o.top.id <= lastTicket_
                || o.top.kind != PanelKind::ConfirmDisplay || !std::isfinite(o.seconds) || o.seconds < 0
                || !std::isfinite(o.seconds + TrialSeconds)) return DisplayCommand::None;
            before_ = before; proposed_ = candidate; ticket_ = o.top; lastTicket_ = o.top.id;
            begunFrame_ = lastFrame_ = o.frame; lastSeconds_ = o.seconds; deadline_ = o.seconds + TrialSeconds;
            settled_ = false; restoreClockPending_ = false; phase_ = DisplayPhase::Testing;
            ++presentationRevision_; return DisplayCommand::RequestCandidate;
        }
        DisplayCommand Cancel(double now, std::uint64_t frame)
        {
            if (phase_ != DisplayPhase::Testing) return DisplayCommand::None;
            phase_ = DisplayPhase::Reverting; settled_ = false; restoreFrame_ = frame;
            // A corrupt/regressing clock still issues one restore; the next valid poll starts its observation deadline.
            restoreClockPending_ = !std::isfinite(now) || now < lastSeconds_ || !std::isfinite(now + RestoreSeconds);
            restoreDeadline_ = restoreClockPending_ ? 0 : now + RestoreSeconds;
            if (!restoreClockPending_) lastSeconds_ = now;
            return DisplayCommand::RequestRestore;
        }
        DisplayCommand Step(const DisplayObservation& o)
        {
            if (phase_ == DisplayPhase::Testing)
            {
                if (!Context(o) || o.seconds >= deadline_) return Cancel(o.seconds, o.frame);
                lastSeconds_ = o.seconds; lastFrame_ = o.frame;
                if (o.actual != proposed_ || o.frame <= begunFrame_) { settled_ = false; return DisplayCommand::None; }
                if (!settled_) { settled_ = true; settledFrame_ = o.frame; ++presentationRevision_; }
            }
            else if (phase_ == DisplayPhase::Reverting)
            {
                // Restoration is an observed viewport result, not the void native request's return value.
                if (o.ownerReady && o.frame > restoreFrame_ && o.actual == before_) { phase_ = DisplayPhase::Idle; return DisplayCommand::None; }
                if (!std::isfinite(o.seconds) || o.seconds < lastSeconds_)
                { phase_ = DisplayPhase::Failed; return DisplayCommand::None; }
                if (restoreClockPending_) { restoreDeadline_ = o.seconds + RestoreSeconds; restoreClockPending_ = false; }
                lastSeconds_ = o.seconds;
                if (!std::isfinite(restoreDeadline_) || o.seconds >= restoreDeadline_) phase_ = DisplayPhase::Failed;
            }
            return DisplayCommand::None;
        }
        bool CanKeep(const DisplayObservation& o) const
        {
            return phase_ == DisplayPhase::Testing && settled_ && Context(o) && o.seconds < deadline_
                && o.actual == proposed_ && o.frame > settledFrame_ && o.presented;
        }
        bool Keep(const DisplayObservation& o)
        {
            if (!CanKeep(o)) return false;
            phase_ = DisplayPhase::Idle; settled_ = false; return true;
        }
        // Unrecoverable binding loss or failed restoration is sticky for this owner lifetime.
        void Fail() { phase_ = DisplayPhase::Failed; settled_ = false; }
    };
}
