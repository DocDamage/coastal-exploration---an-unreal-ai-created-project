#include "Core/SprintRules.h"
#include "Core/PlayerOptionsRules.h"
#include "Core/UIFlowRules.h"
#include <iostream>
#include <limits>
using namespace coastal;
static int checks = 0, failures = 0;
#define CHECK(x) do { ++checks; const bool passed = static_cast<bool>(x); if (!passed) { ++failures; std::cerr << "line " << __LINE__ << ": " << #x << '\n'; } } while (false)
static void Sync(SprintGate& gate, std::uint64_t frame, bool toggle = false,
                 bool allowed = true, std::uint64_t epoch = 1, std::uint64_t revision = 1)
{ gate.Synchronize(allowed, epoch, revision, toggle, frame); }
static bool Sample(SprintGate& gate, std::uint64_t frame, bool down, bool toggle = false)
{ Sync(gate, frame, toggle); return gate.Sample(down, frame); }
int main()
{
    CHECK(ValidSprintTuning({})); CHECK(ValidSprintTuning({100,100})); CHECK(ValidSprintTuning({1000,1500}));
    const double nan = std::numeric_limits<double>::quiet_NaN(), inf = std::numeric_limits<double>::infinity();
    for (SprintTuning t : {SprintTuning{99,520}, {1001,1200}, {330,329}, {330,1501}, {nan,520}, {330,nan}, {inf,520}, {330,-inf}})
        CHECK(!ValidSprintTuning(t));
    PlayerOptions options, old = options;
    CHECK(!options.sprintToggle); CHECK(OptionFieldCount == 10);
    CHECK(AdjustOption(options, OptionField::SprintMode, 1)); CHECK(options.sprintToggle && options != old);
    CHECK(AdjustOption(options, OptionField::SprintMode, -1)); CHECK(options == old);
    CHECK(!AdjustOption(options, OptionField::SprintMode, 0));
    CHECK(!AdjustOption(options, static_cast<OptionField>(100), 1));
    CHECK(CycleSelection(OptionFieldCount-1,1,OptionFieldCount) == 0); CHECK(CycleSelection(0,-1,OptionFieldCount) == OptionFieldCount-1);
    // Initial permission never treats a held key as an intentional fresh press.
    SprintGate hold;
    CHECK(!hold.Sample(true,1)); Sync(hold,1);
    CHECK(!hold.Sample(false,1)); CHECK(!hold.Requested(1));
    CHECK(Sample(hold,2,true)); CHECK(!hold.Requested(2));
    CHECK(Sample(hold,3,false)); CHECK(!hold.Requested(3));
    CHECK(!hold.Sample(true,3)); // once-per-frame aggregate input
    CHECK(Sample(hold,4,true)); CHECK(hold.Requested(4));
    CHECK(!hold.Sample(false,4)); CHECK(hold.Requested(4));
    CHECK(Sample(hold,5,true)); CHECK(hold.Requested(5));
    CHECK(Sample(hold,6,false)); CHECK(!hold.Requested(6));
    // Toggle latches a requested speed, not a direction, and release does not cancel it.
    SprintGate toggle; Sync(toggle,10,true);
    CHECK(Sample(toggle,11,false,true)); CHECK(Sample(toggle,12,true,true)); CHECK(toggle.Requested(12));
    for (std::uint64_t f=13;f<30;++f)
    { CHECK(Sample(toggle,f,true,true)); CHECK(toggle.Requested(f)); }
    CHECK(Sample(toggle,30,false,true)); CHECK(toggle.Requested(30));
    CHECK(Sample(toggle,31,true,true)); CHECK(!toggle.Requested(31));
    CHECK(Sample(toggle,32,false,true)); CHECK(Sample(toggle,33,true,true)); CHECK(toggle.Requested(33));
    // Every interruption invalidates both the toggle and the physical rearm state.
    for (bool mode : {false,true})
    for (int cause=0;cause<5;++cause)
    {
        SprintGate g; Sync(g,0,mode); CHECK(Sample(g,1,false,mode)); CHECK(Sample(g,2,true,mode)); CHECK(g.Requested(2));
        std::uint64_t epoch=1, revision=1; bool currentMode=mode;
        if(cause==0) { Sync(g,3,mode,false); CHECK(!g.Sample(false,3)); Sync(g,4,mode,true); }
        if(cause==1) { epoch=2; Sync(g,4,mode,true,epoch); }
        if(cause==2) { revision=3; Sync(g,4,mode,true,epoch,revision); } // menu open+close between ticks
        if(cause==3) { currentMode=!mode; Sync(g,4,currentMode); }
        if(cause==4) { Sync(g,6,mode); } // heartbeat expired
        const std::uint64_t frame=cause==4?6:4;
        CHECK(!g.Requested(frame)); CHECK(!g.Sample(false,frame));
        g.Synchronize(true,epoch,revision,currentMode,frame+1); CHECK(g.Sample(true,frame+1)); CHECK(!g.Requested(frame+1));
        g.Synchronize(true,epoch,revision,currentMode,frame+2); CHECK(g.Sample(false,frame+2));
        g.Synchronize(true,epoch,revision,currentMode,frame+3); CHECK(g.Sample(true,frame+3)); CHECK(g.Requested(frame+3));
    }
    // Frame-lease boundary is inclusive; it cannot silently re-enable a held or toggled request.
    SprintGate lease; Sync(lease,1,true); Sample(lease,2,false,true); Sample(lease,3,true,true);
    CHECK(lease.Requested(3)); CHECK(lease.Requested(4)); CHECK(lease.Requested(5)); CHECK(!lease.Requested(6));
    CHECK(!lease.Requested(2)); Sync(lease,6,true); CHECK(!lease.Sample(true,6));
    CHECK(Sample(lease,7,true,true)); CHECK(!lease.Requested(7));
    // Regressed frame counters fail closed, even after uint64 max; a fresh release is required.
    SprintGate clock; const auto max=std::numeric_limits<std::uint64_t>::max();
    Sync(clock,max-3); Sample(clock,max-2,false); Sample(clock,max-1,true); CHECK(clock.Requested(max));
    Sync(clock,0); CHECK(!clock.Requested(0)); CHECK(!clock.Sample(true,0));
    CHECK(Sample(clock,1,true)); CHECK(!clock.Requested(1));
    CHECK(Sample(clock,2,false)); CHECK(Sample(clock,3,true)); CHECK(clock.Requested(3));
    // Enumerated press/release traces compare with a tiny independent reference edge model.
    for(bool mode : {false,true}) for(unsigned mask=0;mask<64;++mask)
    {
        SprintGate g; Sync(g,0,mode); CHECK(Sample(g,1,false,mode)); bool want=false, was=false;
        for(unsigned bit=0;bit<6;++bit)
        {
            const bool down=(mask & (1u<<bit))!=0;
            if(mode) { if(down && !was) want=!want; } else want=down;
            was=down; const auto frame=static_cast<std::uint64_t>(bit+2);
            CHECK(Sample(g,frame,down,mode)); CHECK(g.Requested(frame)==want);
        }
    }
    // A speed property is only restored while this owner still owns its last exact value.
    WalkSpeedLease speed; double next=0;
    CHECK(!speed.Apply(600,false,next)); CHECK(next==600);
    CHECK(speed.Acquire(600,{})); CHECK(!speed.Acquire(700,{})); CHECK(speed.Owns(600));
    CHECK(speed.Apply(600,false,next)); CHECK(next==330); CHECK(speed.Owns(330));
    CHECK(speed.Apply(330,true,next)); CHECK(next==520); CHECK(speed.Owns(520));
    CHECK(speed.Apply(520,false,next)); CHECK(next==330);
    CHECK(speed.Release(330,next)); CHECK(next==600); CHECK(!speed.Owns(600));
    CHECK(!speed.Release(600,next)); CHECK(next==600);
    for(double foreign : {0.0,200.0,700.0,nan,inf})
    {
        WalkSpeedLease own; CHECK(own.Acquire(600,{})); CHECK(own.Apply(600,true,next));
        CHECK(!own.Apply(foreign,false,next)); CHECK(!own.Owns(520));
        CHECK(!own.Release(foreign,next)); CHECK(std::isnan(foreign)?std::isnan(next):next==foreign);
    }
    CHECK(!speed.Acquire(nan,{})); CHECK(!speed.Acquire(-1,{})); CHECK(!speed.Acquire(600,{10,20}));
    // Combined flow: a menu cancels sprint; release and another press are needed after returning.
    SprintGate combined; WalkSpeedLease cap; double current=600;
    CHECK(cap.Acquire(current,{})); Sync(combined,0,true);
    Sample(combined,1,false,true); Sample(combined,2,true,true);
    CHECK(cap.Apply(current,combined.Requested(2),current)); CHECK(current==520);
    Sync(combined,3,true,false); CHECK(cap.Apply(current,combined.Requested(3),current)); CHECK(current==330);
    Sync(combined,4,true); Sample(combined,5,true,true); CHECK(!combined.Requested(5));
    Sample(combined,6,false,true); Sample(combined,7,true,true);
    CHECK(cap.Apply(current,combined.Requested(7),current)); CHECK(current==520);
    CHECK(cap.Release(current,current)); CHECK(current==600);
    std::cout << "Sprint/input/speed ownership core: " << checks << " checks; " << failures << " failures\n";
    return failures?1:0;
}
