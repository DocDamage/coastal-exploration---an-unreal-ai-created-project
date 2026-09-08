#include "Core/InteractionRules.h"
#include "m1_test_support.h"
#include <array>
#include <limits>

int main()
{
    using namespace coastal;
    TestRun t;
    InteractionDispatchGate dispatch;
    t.Expect(dispatch.Begin(100), "first world action enters");
    t.Expect(!dispatch.Begin(100), "recursive same-frame action blocked");
    t.Expect(!dispatch.Begin(101), "active callback cannot reenter even with later frame");
    dispatch.End();
    t.Expect(!dispatch.Begin(100), "completed action cannot execute a second world action in the same frame");
    t.Expect(!dispatch.Begin(99), "time regression rejected");
    t.Expect(dispatch.Begin(101), "fresh action in later frame"); dispatch.End();
    t.Expect(!dispatch.Begin(101), "new session must not reset bridge dispatch debounce");

    InteractionIntentGate intent;
    t.Expect(!intent.Press(1), "uninitialized press suppressed");
    t.Expect(intent.Synchronize(1,0,true,2), "initial permission boundary");
    intent.Released();
    t.Expect(!intent.Press(2), "cannot act on same frame as initial enable");
    t.Expect(!intent.Press(3), "rejected press still latches held input");
    intent.Released();
    t.Expect(intent.Press(4), "release and later press accepted");
    t.Expect(!intent.Press(5), "holding across frames does not repeat");
    intent.Released();
    t.Expect(!intent.Press(4), "release does not allow duplicate in same input frame");
    intent.Released();
    t.Expect(intent.Press(6), "next physical press accepted");
    t.Expect(intent.Synchronize(1,1,false,7), "opening menu invalidates permission and focus");
    intent.Released();
    t.Expect(!intent.Press(8), "menu consumes held key without an action");
    t.Expect(intent.Synchronize(1,2,true,9), "closing menu requires a fresh release");
    t.Expect(!intent.Press(10), "menu-held input cannot leak into world");
    intent.Released();
    t.Expect(intent.Press(11), "release restores normal input");
    t.Expect(intent.Synchronize(2,2,true,12), "load invalidates prior session input");
    t.Expect(!intent.Press(13), "load-held press rejected");
    intent.Released();
    t.Expect(intent.Press(14), "new session fresh press accepted");
    intent.Cancel();
    t.Expect(!intent.Press(15), "canceled input is not a release");
    intent.Released(); t.Expect(intent.Press(16), "actual release recovers from cancel");
    t.Expect(intent.Synchronize(2,4,true,17), "open-close between ticks detected by revision");
    t.Expect(!intent.Press(18), "same enabled state with changed revision still needs release");
    intent.Released();
    t.Expect(intent.Press(19), "explicit release after missed menu transition");
    t.Expect(!intent.Synchronize(2,4,true,20), "unchanged permissions do not reset the input latch");
    t.Expect(!intent.Press(21), "unchanged permissions do not repeat held input");
    t.Expect(intent.Synchronize(2,4,false,22) && !intent.Enabled(), "pause or busy disables intent");
    t.Expect(!intent.Press(23), "disabled input remains suppressed");

    InteractionFocusLease focus;
    t.Expect(!focus.Valid(1,1,0,10), "no focus before real vendor sample");
    t.Expect(!focus.Set(0,1,0,10), "null actor cannot gain lease");
    t.Expect(focus.Set(41,1,0,10), "sample actual actor identity");
    for (std::uint64_t frame = 10; frame <= 12; ++frame)
        t.Expect(focus.Valid(41,1,0,frame), "bounded lease accepts current and two prior frames");
    t.Expect(!focus.Valid(41,1,0,13), "missing heartbeat expires target");
    t.Expect(!focus.Valid(41,1,0,9), "future sample cannot be used in past");
    t.Expect(!focus.Valid(42,1,0,10), "another actor cannot borrow focus");
    t.Expect(!focus.Valid(41,2,0,10), "new campaign invalidates old focus");
    t.Expect(!focus.Valid(41,1,1,10), "UI transition invalidates old focus");
    t.Expect(!focus.Set(42,1,0,9), "out-of-order focus update cannot replace newer one");
    t.Expect(focus.Set(42,1,0,11), "new current target replaces old one");
    t.Expect(!focus.ClearExpected(41) && focus.Valid(42,1,0,12), "late loss of A cannot clear B");
    t.Expect(focus.ClearExpected(42) && !focus.Valid(42,1,0,12), "loss of actual focus clears it");
    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    focus.Set(5,1,0,max-1);
    t.Expect(focus.Valid(5,1,0,max), "lease age avoids addition overflow");
    t.Expect(!focus.Valid(5,1,0,0), "frame wrap never extends a stale lease");
    focus.Clear(); t.Expect(!focus.Valid(5,1,0,max), "explicit clear invalidates even recent sample");

    // Exercise the same native permission predicate for every combination of seven flags.
    for (unsigned mask = 0; mask < 128; ++mask)
    {
        InteractionContext c{(mask&1)!=0,(mask&2)!=0,(mask&4)!=0,(mask&8)!=0,
            (mask&16)!=0,(mask&32)!=0,(mask&64)!=0};
        const bool expected = c.configured && c.campaign && !c.recovery && !c.busy && !c.menu && !c.paused && c.samePawn;
        t.Expect((WorldPermissionFor(c)==WorldPermission::Allowed)==expected,"world input requires all permissions");
    }
    t.Expect(WorldPermissionFor({true,true,true,true,true,true,false})==WorldPermission::Recovery,
        "recovery failure has highest priority");
    t.Expect(WorldPermissionFor({false,true,false,false,false,false,true})==WorldPermission::NotReady,
        "missing provider cannot produce usable input");
    t.Expect(WorldPermissionFor({true,true,false,false,true,false,true})==WorldPermission::Menu,
        "menu blocks interactions before reaching target");

    using K = OfferKind; using V = OfferVerb;
    t.Expect(ChooseOffer(K::Door,false,false,false,false)==V::OpenDoor,"closed door offer");
    t.Expect(ChooseOffer(K::Door,true,false,false,false)==V::CloseDoor,"open door offer");
    t.Expect(ChooseOffer(K::Pickup,false,false,false,false)==V::Take,"available pickup offer");
    t.Expect(ChooseOffer(K::Pickup,true,false,false,false)==V::None,"collected pickup is hidden");
    t.Expect(ChooseOffer(K::Storage,false,false,false,false)==V::OpenStorage,"storage offer");
    t.Expect(ChooseOffer(K::Discovery,false,false,false,false)==V::ReadDiscovery,"new postcard offer");
    t.Expect(ChooseOffer(K::Discovery,true,false,false,false)==V::RereadDiscovery,"reread postcard offer");
    t.Expect(ChooseOffer(K::Note,false,false,false,false)==V::ReadNote,"new note offer");
    t.Expect(ChooseOffer(K::Note,true,false,false,false)==V::RereadNote,"reread note offer");
    t.Expect(ChooseOffer(K::Unknown,false,false,false,false)==V::None,"unknown kind never becomes actionable");
    t.Expect(ChooseOffer(K::Radio,false,false,false,false)==V::InspectRadio,"inspect precedes repair");
    t.Expect(ChooseOffer(K::Radio,false,true,false,false)==V::RepairRadio,"inspected radio can request parts");
    t.Expect(ChooseOffer(K::Radio,false,true,true,false)==V::Listen,"repaired radio offers first listen");
    t.Expect(ChooseOffer(K::Radio,false,true,true,true)==V::Replay,"heard signal can replay");
    for (bool active : {false,true})
        for (bool repaired : {false,true})
            for (bool heard : {false,true})
                t.Expect(ChooseOffer(K::Radio,active,false,repaired,heard)==V::InspectRadio,
                    "display projection never invents inspection on malformed input");

    // Combined pure-input regression: two bindings and recursive notice cannot toggle twice.
    InteractionDispatchGate doorGate; bool open = false; int toggles = 0;
    auto toggle = [&](std::uint64_t f)
    {
        if (!doorGate.Begin(f)) return false;
        open = !open; ++toggles;
        t.Expect(!doorGate.Begin(f),"notice callback cannot enter another mutation");
        doorGate.End(); return true;
    };
    t.Expect(toggle(500),"first door intent commits in helper scenario");
    t.Expect(!toggle(500) && open && toggles == 1,"duplicate binding does not undo door action");
    t.Expect(toggle(501) && !open && toggles == 2,"next explicit door action still works");

    // A press without fresh focus is consumed, never retried when focus arrives later.
    InteractionIntentGate noTarget; noTarget.Synchronize(9,0,true,600); noTarget.Released();
    InteractionFocusLease absent;
    t.Expect(noTarget.Press(601) && !absent.Valid(10,9,0,601),"fresh input can have no selected target");
    absent.Set(10,9,0,602);
    t.Expect(!noTarget.Press(602),"new focus must not execute a queued held intent");
    noTarget.Released();
    t.Expect(noTarget.Press(603) && absent.Valid(10,9,0,603),"player can explicitly try again with fresh focus");
    return t.Finish("Interaction routing/offer core");
}
