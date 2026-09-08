#pragma once
#include <cstdint>
#include <limits>

namespace coastal
{
    // Game-owned input permissions, not a focus scanner or inventory authority.
    enum class WorldPermission { Allowed, NotReady, Recovery, Busy, Menu, Paused, WrongPawn };
    struct InteractionContext
    {
        bool configured = false, campaign = false, recovery = false, busy = false;
        bool menu = false, paused = false, samePawn = true;
    };
    inline WorldPermission WorldPermissionFor(const InteractionContext& c)
    {
        if (c.recovery) return WorldPermission::Recovery;
        if (!c.configured || !c.campaign) return WorldPermission::NotReady;
        if (!c.samePawn) return WorldPermission::WrongPawn;
        if (c.menu) return WorldPermission::Menu;
        if (c.paused) return WorldPermission::Paused;
        if (c.busy) return WorldPermission::Busy;
        return WorldPermission::Allowed;
    }
    // Persists through new/load and all action callbacks, including post-mutation notices.
    class InteractionDispatchGate
    {
        std::uint64_t last_ = 0;
        bool seen_ = false, running_ = false;
    public:
        bool Begin(std::uint64_t frame)
        {
            if (running_ || (seen_ && frame <= last_)) return false;
            running_ = seen_ = true; last_ = frame; return true;
        }
        void End() { running_ = false; }
    };
    class InteractionIntentGate
    {
        std::uint64_t epoch_ = 0, revision_ = 0, boundary_ = 0, lastPress_ = 0;
        bool initialized_ = false, enabled_ = false, held_ = false;
        bool releaseRequired_ = true, pressed_ = false;
    public:
        // Every permission transition invalidates focus and requires a released input.
        bool Synchronize(std::uint64_t epoch, std::uint64_t revision, bool enabled, std::uint64_t frame)
        {
            if (initialized_ && epoch == epoch_ && revision == revision_ && enabled == enabled_) return false;
            initialized_ = true; epoch_ = epoch; revision_ = revision; enabled_ = enabled;
            boundary_ = frame; releaseRequired_ = true; return true;
        }
        void Released() { held_ = false; if (enabled_) releaseRequired_ = false; }
        void Cancel() { releaseRequired_ = true; }
        bool Press(std::uint64_t frame)
        {
            const bool accept = initialized_ && enabled_ && !held_ && !releaseRequired_
                && frame > boundary_ && (!pressed_ || frame > lastPress_);
            held_ = true; releaseRequired_ = true;
            if (accept) { lastPress_ = frame; pressed_ = true; }
            return accept;
        }
        bool Enabled() const { return initialized_ && enabled_; }
    };
    // Caller forwards Hyper's actual selected actor each frame. No target discovery here.
    class InteractionFocusLease
    {
        std::uint64_t target_ = 0, epoch_ = 0, revision_ = 0, frame_ = 0;
    public:
        static constexpr std::uint64_t MaxAgeFrames = 2;
        bool Set(std::uint64_t target, std::uint64_t epoch, std::uint64_t revision, std::uint64_t frame)
        {
            if (target == 0 || (target_ != 0 && frame < frame_)) return false;
            target_ = target; epoch_ = epoch; revision_ = revision; frame_ = frame; return true;
        }
        void Clear() { target_ = 0; }
        bool ClearExpected(std::uint64_t target)
        {
            if (target == 0 || target != target_) return false;
            Clear(); return true;
        }
        bool Valid(std::uint64_t target, std::uint64_t epoch, std::uint64_t revision, std::uint64_t frame) const
        {
            return target != 0 && target == target_ && epoch == epoch_ && revision == revision_
                && frame >= frame_ && frame - frame_ <= MaxAgeFrames;
        }
    };
    enum class OfferKind { Door, Pickup, Storage, Radio, Discovery, Note, Unknown };
    enum class OfferVerb { None, OpenDoor, CloseDoor, Take, OpenStorage, InspectRadio, RepairRadio,
        Listen, Replay, ReadDiscovery, RereadDiscovery, ReadNote, RereadNote };
    inline OfferVerb ChooseOffer(OfferKind kind, bool active, bool inspected, bool repaired, bool heard)
    {
        switch (kind)
        {
        case OfferKind::Door: return active ? OfferVerb::CloseDoor : OfferVerb::OpenDoor;
        case OfferKind::Pickup: return active ? OfferVerb::None : OfferVerb::Take;
        case OfferKind::Storage: return OfferVerb::OpenStorage;
        case OfferKind::Radio:
            if (!inspected) return OfferVerb::InspectRadio;
            if (!repaired) return OfferVerb::RepairRadio;
            return heard ? OfferVerb::Replay : OfferVerb::Listen;
        case OfferKind::Discovery: return active ? OfferVerb::RereadDiscovery : OfferVerb::ReadDiscovery;
        case OfferKind::Note: return active ? OfferVerb::RereadNote : OfferVerb::ReadNote;
        default: return OfferVerb::None;
        }
    }
}
