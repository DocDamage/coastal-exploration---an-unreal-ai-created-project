#include "Core/RecoveryRules.h"
#include "Core/CampaignRules.h"
#include "Core/InteractionRules.h"
#include "m1_test_support.h"
#include <limits>
using namespace coastal;
int main()
{
    TestRun t;
    ReturnFlow flow;
    t.Expect(flow.Phase() == ReturnPhase::Idle && !flow.Active(), "initially idle");
    t.Expect(!flow.Begin(0), "zero campaign epoch cannot return");
    t.Expect(!flow.Placed() && !flow.Finish(), "out-of-order completion rejected");
    t.Expect(flow.Begin(8), "begin current session");
    t.Expect(!flow.Begin(8) && !flow.Begin(9), "no nested or replacement return");
    t.Expect(flow.Matches(8) && !flow.Matches(9), "session-bound return");
    t.Expect(!flow.Advance(-0.1, false), "negative dt rejected");
    t.Expect(!flow.Advance(std::numeric_limits<double>::quiet_NaN(), false), "NaN dt rejected");
    t.Expect(!flow.Advance(std::numeric_limits<double>::infinity(), false), "infinite dt rejected");
    t.Expect(flow.Advance(0, false) && flow.Opacity() == 0, "zero time does not progress");
    flow.Advance(ReturnFadeOut / 2, false);
    t.Expect(flow.Opacity() > 0.49 && flow.Opacity() < 0.51, "fade midpoint");
    flow.Advance(ReturnFadeOut / 2, false);
    t.Expect(flow.Phase() == ReturnPhase::Placement && flow.Opacity() == 1, "place only when faded");
    t.Expect(flow.Placed() && !flow.Placed(), "place completion occurs once");
    flow.Advance(ReturnFadeIn, false);
    t.Expect(flow.Phase() == ReturnPhase::Settling && flow.Opacity() == 0, "visible while settling");
    for (int i=0;i<3;++i) flow.Advance(0.1,true);
    t.Expect(!flow.CanFinish(), "short ground observation is insufficient");
    flow.Advance(0.01,false);
    t.Expect(!flow.CanFinish(), "airborne sample resets ground dwell");
    for (int i=0;i<4;++i) flow.Advance(0.1,true);
    t.Expect(flow.CanFinish() && flow.Finish() && !flow.Active(), "stable return completes");
    t.Expect(!flow.Finish(), "cannot double release recovery");
    t.Expect(flow.Begin(8), "later deliberate return allowed");
    flow.Fail();
    t.Expect(flow.Phase()==ReturnPhase::Failed && flow.Opacity()==0, "failure never leaves opaque screen in flow");
    t.Expect(!flow.Begin(8) && !flow.Begin(9), "failure sticky across sessions");
    flow.Stop(); flow.Fail();
    t.Expect(flow.Phase()==ReturnPhase::Stopped && !flow.Begin(8), "teardown cannot be revived");
    ReturnFlow timeout; timeout.Begin(1); timeout.Advance(ReturnFadeOut,false); timeout.Placed();
    timeout.Advance(ReturnFadeIn,false);
    for(int i=0;i<21;++i) timeout.Advance(0.1,false);
    t.Expect(timeout.Phase()==ReturnPhase::Failed && !timeout.CanFinish(), "never-grounded return fails rather than loops");
    ReturnFlow hitch; hitch.Begin(1); hitch.Advance(10,false); hitch.Placed(); hitch.Advance(10,false);
    t.Expect(hitch.Phase()==ReturnPhase::Settling, "hitch cannot skip settling");
    hitch.Advance(3,true);
    t.Expect(hitch.Phase()==ReturnPhase::Failed, "single huge sample is not evidence of stable grounding");
    // All active phases can fail closed; none autonomously retries a teleport.
    for (int phase=0;phase<4;++phase)
    {
        ReturnFlow f; f.Begin(42);
        if(phase>=1) f.Advance(ReturnFadeOut,false);
        if(phase>=2) f.Placed();
        if(phase>=3) f.Advance(ReturnFadeIn,false);
        t.Expect(f.Active(), "phase active before failure"); f.Fail();
        t.Expect(!f.Active() && !f.Begin(42) && !f.Advance(1,true), "phase failure is terminal");
    }
    for (int primary=0;primary<2;++primary)
    for (int fallback=0;fallback<2;++fallback)
    {
        int calls[2]={0,0};
        const int selected=TryReturnCandidates(true,[&](int index)
            { ++calls[index]; return index==0 ? primary!=0 : fallback!=0; });
        t.Expect(selected==(primary ? 0 : (fallback ? 1 : -1)), "primary then fallback selection");
        t.Expect(calls[0]==1 && calls[1]==(primary ? 0 : 1), "each distinct candidate attempted at most once");
    }
    int duplicateCalls=0;
    t.Expect(TryReturnCandidates(false,[&](int){++duplicateCalls;return false;})==-1 && duplicateCalls==1,
        "identical fallback cannot repeat failed teleport");
    CheckpointVisit visit;
    t.Expect(!visit.Observe(0,true,0.2), "no checkpoint means no capture");
    t.Expect(!visit.Observe(1,false,0.2), "unsafe ground not captured");
    t.Expect(!visit.Observe(1,true,0.2) && visit.Observe(1,true,0.2), "continuous dwell records candidate");
    visit.Recorded();
    t.Expect(!visit.Observe(1,true,0.2), "remaining in checkpoint does not spam saves");
    visit.Interrupted();
    t.Expect(!visit.Observe(1,true,0.2) && !visit.Observe(1,true,0.2),
        "menu or save interruption cannot record the same checkpoint twice");
    t.Expect(!visit.Observe(2,true,0.2) && visit.Observe(2,true,0.2), "different checkpoint has its own dwell");
    visit.Recorded();
    t.Expect(!visit.Observe(2,true,0.3) && !visit.Observe(2,true,0.2) && !visit.Observe(2,true,0.2),
        "hitch cannot record the same checkpoint twice after fresh dwell");
    visit.Reset(); t.Expect(!visit.Observe(2,true,0.2), "session reset removes dwell and recorded identity");
    t.Expect(!visit.Observe(2,true,0.3), "hitch resets checkpoint observation");
    t.Expect(!visit.Observe(2,true,0.2) && visit.Observe(2,true,0.2), "fresh dwell after hitch");
    t.Expect(!visit.Observe(2,true,-1), "bad dt cannot count");
    t.Expect(!visit.Observe(2,true,std::numeric_limits<double>::quiet_NaN()), "NaN dwell rejected");
    t.Expect(!visit.Observe(2,true,0.2), "bad sample reset previous candidate");
    t.Expect(!visit.Observe(0,true,0.1), "leaving region resets recorded visit");
    // The EXACT gate shared with native saves excludes mutation and IO while a return spans frames.
    OperationGate gate; gate.RequestSave();
    t.Expect(gate.BeginRecovery() && gate.Busy() && gate.IsRecovery(), "exclusive relocation reservation");
    t.Expect(!gate.BeginIO() && !gate.BeginMutation() && !gate.BeginRecovery(), "reject writes loads inventory and nested recovery");
    t.Expect(!gate.TakeSaveRequest(), "pending save not consumed halfway through relocation");
    gate.RequestSave(); gate.EndIO(); gate.EndMutation();
    t.Expect(gate.Busy() && gate.IsRecovery(), "other ownership releases cannot unlock return");
    gate.EndRecovery();
    t.Expect(!gate.Busy() && gate.TakeSaveRequest() && !gate.TakeSaveRequest(), "coalesced save survives and runs once");
    gate.BeginIO(); t.Expect(!gate.BeginRecovery(), "recovery cannot interrupt save/load"); gate.EndIO();
    gate.BeginMutation(); t.Expect(!gate.BeginRecovery(), "recovery cannot interrupt inventory commit"); gate.EndMutation();
    gate.BeginRecovery(); gate.RequestSave(); gate.Poison(); gate.EndRecovery();
    t.Expect(gate.Poisoned() && gate.Busy() && !gate.TakeSaveRequest(), "fatal return cannot write unsafe state");
    t.Expect(!gate.BeginIO() && !gate.BeginMutation() && !gate.BeginRecovery(), "poison remains terminal");
    // Existing relay invalidation must discard a held Interact across the return blocker transition.
    InteractionIntentGate input; input.Synchronize(1,0,true,1); input.Released();
    t.Expect(input.Press(2), "intentional world press");
    input.Synchronize(1,1,false,3); input.Synchronize(1,2,true,4);
    t.Expect(!input.Press(5), "held interaction cannot execute on return");
    input.Released(); t.Expect(input.Press(6), "release then fresh press works");
    // Exact box/capsule geometry: face, cap, rounded corner, separation, symmetry and finite inputs.
    t.Expect(ValidSafetyBox(1,1,1) && !ValidSafetyBox(0,1,1), "positive bounded extents");
    t.Expect(!ValidSafetyBox(1000001,1,1), "excessive extents rejected");
    t.Expect(!ValidSafetyBox(1,std::numeric_limits<double>::infinity(),1), "infinite box rejected");
    t.Expect(SafetyCapsuleOverlap(0,0,0,34,88,100,100,100), "capsule centered inside volume");
    t.Expect(SafetyCapsuleOverlap(134,0,0,34,88,100,100,100), "touching side counts");
    t.Expect(!SafetyCapsuleOverlap(134.01,0,0,34,88,100,100,100), "side clearance excludes");
    t.Expect(SafetyCapsuleOverlap(0,0,188,34,88,100,100,100), "touching top cap counts");
    t.Expect(!SafetyCapsuleOverlap(0,0,188.01,34,88,100,100,100), "cap clearance excludes");
    t.Expect(!SafetyCapsuleOverlap(134,134,0,34,88,100,100,100), "rounded corner not AABB false positive");
    t.Expect(SafetyCapsuleOverlap(120,120,0,34,88,100,100,100), "near corner intersects sphere");
    for(double x: {0.,100.,120.,134.,140.,300.})
    for(double y: {0.,100.,120.,134.,140.,300.})
    {
        const bool a=SafetyCapsuleOverlap(x,y,0,34,88,100,100,100);
        t.Expect(a==SafetyCapsuleOverlap(-x,-y,0,34,88,100,100,100), "opposite side symmetry");
        t.Expect(a==SafetyCapsuleOverlap(y,x,0,34,88,100,100,100), "square-box XY symmetry");
    }
    for(double bad: {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
    {
        t.Expect(!SafetyCapsuleOverlap(bad,0,0,34,88,100,100,100), "non-finite center");
        t.Expect(!SafetyCapsuleOverlap(0,bad,0,34,88,100,100,100), "non-finite Y");
        t.Expect(!SafetyCapsuleOverlap(0,0,bad,34,88,100,100,100), "non-finite Z");
        t.Expect(!SafetyCapsuleOverlap(0,0,0,bad,88,100,100,100), "non-finite radius");
        t.Expect(!SafetyCapsuleOverlap(0,0,0,34,bad,100,100,100), "non-finite height");
    }
    t.Expect(!SafetyCapsuleOverlap(0,0,0,0,88,100,100,100), "zero radius");
    t.Expect(!SafetyCapsuleOverlap(0,0,0,34,20,100,100,100), "height smaller than radius");
    return t.Finish("Safe-return/checkpoint core");
}
