#pragma once
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <limits>
#include <vector>

namespace coastal
{
    enum class PanelKind { Session, Pause, Inventory, Storage, Journal, Transcript, ItemDetails, ConfirmSession, ConfirmExit, Recovery, Settings, Display, ConfirmDisplay, CharacterCreator };
    struct PanelTicket
    {
        std::uint64_t id = 0, epoch = 0;
        PanelKind kind = PanelKind::Session;
    };
    struct PanelFrame
    {
        PanelTicket ticket;
        std::uint64_t openedFrame = 0;
        int focus = 0;
    };
    // Pure permission/stack rules used by the native UI. No items or mission facts.
    class UIFlow
    {
        std::vector<PanelFrame> frames_;
        std::uint64_t next_ = 0, epoch_ = 0, lastCommand_ = 0;
        bool commandSeen_ = false, recovery_ = false;
    public:
        static constexpr std::size_t MaxDepth = 8;
        void Reset(std::uint64_t epoch)
        {
            frames_.clear(); epoch_ = epoch;
            // Never reset the ticket counter or same-frame debounce at a session boundary.
            // Recovery is sticky for this component's lifetime.
        }
        bool Contains(PanelKind kind) const
        {
            return std::any_of(frames_.begin(), frames_.end(), [kind](const auto& f) { return f.ticket.kind == kind; });
        }
        PanelTicket Push(PanelKind kind, std::uint64_t frame)
        {
            if ((recovery_ && kind != PanelKind::Recovery) || frames_.size() >= MaxDepth
                || Contains(kind) || next_ == std::numeric_limits<std::uint64_t>::max()) return {};
            PanelTicket ticket{++next_, epoch_, kind};
            frames_.push_back({ticket, frame, 0});
            return ticket;
        }
        PanelTicket RequireRecovery(std::uint64_t frame)
        {
            recovery_ = true; frames_.clear();
            return Push(PanelKind::Recovery, frame);
        }
        bool IsTop(PanelTicket ticket) const
        {
            return ticket.id != 0 && !frames_.empty() && ticket.epoch == epoch_
                && frames_.back().ticket.id == ticket.id && frames_.back().ticket.kind == ticket.kind;
        }
        bool CanCommand(PanelTicket ticket, std::uint64_t frame) const
        {
            return IsTop(ticket) && frame > frames_.back().openedFrame
                && (!commandSeen_ || frame > lastCommand_);
        }
        bool ClaimCommand(PanelTicket ticket, std::uint64_t frame)
        {
            if (!CanCommand(ticket, frame)) return false;
            commandSeen_ = true; lastCommand_ = frame; return true;
        }
        bool Pop(PanelTicket ticket, bool campaignActive)
        {
            if (!IsTop(ticket) || recovery_ || (!campaignActive && frames_.size() == 1)) return false;
            frames_.pop_back(); return true;
        }
        void RememberFocus(PanelTicket ticket, int focus)
        {
            if (IsTop(ticket)) frames_.back().focus = std::max(0, focus);
        }
        const PanelFrame* Top() const { return frames_.empty() ? nullptr : &frames_.back(); }
        std::size_t Depth() const { return frames_.size(); }
        bool RecoveryRequired() const { return recovery_; }
    };
    struct UIRect { double left = 0, top = 0, right = 0, bottom = 0; };
    inline bool VisibleUIIntersection(UIRect a, UIRect b)
    {
        const auto valid = [](UIRect r) { return std::isfinite(r.left) && std::isfinite(r.top)
            && std::isfinite(r.right) && std::isfinite(r.bottom) && r.right > r.left && r.bottom > r.top; };
        return valid(a) && valid(b) && std::min(a.right, b.right) - std::max(a.left, b.left) > 1.0
            && std::min(a.bottom, b.bottom) - std::max(a.top, b.top) > 1.0;
    }
    class PresentationGate
    {
        bool painted_ = false;
        std::uint64_t frame_ = 0;
    public:
        void MarkPainted(std::uint64_t frame) { if (!painted_) { painted_ = true; frame_ = frame; } }
        bool Ready(std::uint64_t frame) const { return painted_ && frame > frame_; }
    };
    inline int CycleSelection(int current, int direction, int count)
    {
        if (count <= 0) return -1;
        if (current < 0 || current >= count) return 0;
        if (direction > 0) return current == count - 1 ? 0 : current + 1;
        if (direction < 0) return current == 0 ? count - 1 : current - 1;
        return current;
    }
}
