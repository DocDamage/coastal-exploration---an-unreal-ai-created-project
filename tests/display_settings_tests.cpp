#include "Core/DisplaySettingsRules.h"
#include <iostream>
#include <limits>
using namespace coastal;
namespace
{
    int checks = 0, failures = 0;
    void Check(bool ok, const char* label) { ++checks; if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; } }
    const DisplayMode Old{1600,900,DisplayWindowMode::Windowed}, New{1920,1080,DisplayWindowMode::Fullscreen};
    const std::vector<DisplayMode> Modes{Old,New,{1920,1080,DisplayWindowMode::Borderless}};
    DisplayObservation At(double seconds = 100, std::uint64_t frame = 10)
    { return {seconds,frame,{2,0,PanelKind::ConfirmDisplay},Old,true,true,false}; }
    void Start(DisplayTrial& t, DisplayObservation o = At())
    { Check(t.Begin(Old,New,Modes,o)==DisplayCommand::RequestCandidate,"One initial change request"); }
    DisplayObservation Settle(DisplayTrial& t)
    {
        auto o=At(101,11); o.actual=New;
        Check(t.Step(o)==DisplayCommand::None,"Observe changed mode without another request");
        Check(t.Settled(),"Changed mode observed"); return o;
    }
    void CatalogueTests()
    {
        Check(ValidDisplaySnapshot({1,1,DisplayWindowMode::Windowed}),"Snapshot supports small prior size");
        Check(!SelectableDisplayMode({1,1,DisplayWindowMode::Windowed}),"No unreadable candidate");
        Check(SelectableDisplayMode({1280,720,DisplayWindowMode::Windowed}),"Candidate lower bounds");
        Check(SelectableDisplayMode({7680,4320,DisplayWindowMode::Fullscreen}),"Candidate upper bounds");
        for (const auto& m : std::vector<DisplayMode>{{0,1080,DisplayWindowMode::Windowed}, {-1,900,DisplayWindowMode::Windowed},
            {32769,1080,DisplayWindowMode::Windowed}, {1920,0,DisplayWindowMode::Windowed},
            {1920,32769,DisplayWindowMode::Windowed}, {1920,1080,static_cast<DisplayWindowMode>(-1)},
            {1920,1080,static_cast<DisplayWindowMode>(3)}})
            Check(!ValidDisplaySnapshot(m),"Reject invalid size/mode snapshot");
        Check(!SelectableDisplayMode({1279,720,DisplayWindowMode::Windowed}),"Width minimum");
        Check(!SelectableDisplayMode({1280,719,DisplayWindowMode::Windowed}),"Height minimum");
        Check(!SelectableDisplayMode({7681,4320,DisplayWindowMode::Fullscreen}),"Width maximum");
        Check(!SelectableDisplayMode({7680,4321,DisplayWindowMode::Fullscreen}),"Height maximum");
        const auto c=DisplayCatalogue({Old,New,{},Old,New,Modes[2]});
        Check(c.size()==3,"Unique valid modes only");
        Check(c[0]==New && c[1]==Modes[2] && c[2]==Old,"Deterministic catalogue ordering");
        Check(DisplayCatalogue({}).empty(),"Missing enumeration is not invented");
        std::vector<DisplayMode> many;
        for(int n=0;n<513;++n) many.push_back({1280+n,720,DisplayWindowMode::Windowed});
        Check(DisplayCatalogue(many).empty(),"Refuse excessive enumeration");
        many.pop_back(); Check(DisplayCatalogue(many).size()==512,"Bounded enumeration admitted");
    }
    void BeginTests()
    {
        DisplayTrial t; auto o=At();
        Check(!t.Busy() && t.Phase()==DisplayPhase::Idle,"Idle initial state");
        Check(t.Cancel(o.seconds,o.frame)==DisplayCommand::None,"No restore without a trial");
        Check(t.Step(o)==DisplayCommand::None && !t.Keep(o),"No spontaneous change or keep");
        Check(t.Begin(Old,Old,Modes,o)==DisplayCommand::None,"Same mode no-op");
        Check(t.Begin({},New,Modes,o)==DisplayCommand::None,"Cannot restore unknown snapshot");
        Check(t.Begin(Old,New,{},o)==DisplayCommand::None,"Candidate must be enumerated");
        auto unlisted=New; ++unlisted.width;
        Check(t.Begin(Old,unlisted,Modes,o)==DisplayCommand::None,"No hand-typed unsupported mode");
        for(int n=0;n<7;++n)
        {
            auto bad=o;
            if(n==0) bad.ownerReady=false;
            if(n==1) bad.foreground=false;
            if(n==2) bad.top.id=0;
            if(n==3) bad.top.kind=PanelKind::Display;
            if(n==4) bad.actual=New;
            if(n==5) bad.seconds=-1;
            if(n==6) bad.seconds=std::numeric_limits<double>::quiet_NaN();
            Check(t.Begin(Old,New,Modes,bad)==DisplayCommand::None,"Invalid start context");
        }
        Start(t); Check(t.Busy(),"Trial active");
        Check(t.Begin(Old,New,Modes,o)==DisplayCommand::None,"No overlapping trial");
        Check(t.Before()==Old && t.Proposed()==New,"Exact snapshot and candidate retained");
        Check(t.SecondsLeft(100)==15 && t.SecondsLeft(114)==1,"Real-time countdown");
        Check(t.SecondsLeft(std::numeric_limits<double>::quiet_NaN())==0,"Invalid timer not rendered");
    }
    void ConfirmationTests()
    {
        DisplayTrial t; Start(t); auto o=At(); o.actual=New; o.presented=true;
        t.Step(o); Check(!t.Settled() && !t.CanKeep(o),"Never keep on enabling frame");
        o=Settle(t); o.presented=true;
        Check(!t.Keep(o),"Observation frame cannot also confirm");
        ++o.frame; o.seconds=102; o.presented=false;
        Check(!t.CanKeep(o),"Unpainted new-resolution panel cannot confirm");
        o.presented=true; Check(t.CanKeep(o),"Fresh painted later frame can keep");
        auto wrong=o; ++wrong.top.id; Check(!t.Keep(wrong),"Ticket mismatch");
        wrong=o; ++wrong.top.epoch; Check(!t.Keep(wrong),"Epoch mismatch");
        wrong=o; wrong.foreground=false; Check(!t.Keep(wrong),"Background cannot keep");
        wrong=o; wrong.ownerReady=false; Check(!t.Keep(wrong),"Lost owner cannot keep");
        wrong=o; wrong.actual=Old; Check(!t.Keep(wrong),"Live viewport rechecked at keep");
        wrong=o; wrong.seconds=115; Check(!t.Keep(wrong),"Keep exactly at deadline refused without a tick");
        Check(t.Keep(o) && !t.Busy(),"Keep retires the trial");
        Check(!t.Keep(o),"No repeated keep");
        Check(t.Cancel(o.seconds,o.frame)==DisplayCommand::None,"Closing a kept panel never restores");
        Check(t.Begin(Old,New,Modes,At())==DisplayCommand::None,"Old ticket cannot restart");
        auto next=At(200,200); next.top.id=3; next.actual=New;
        Check(t.Begin(New,Old,Modes,next)==DisplayCommand::RequestCandidate,"New trial after session-only keep");
        Check(t.Before()==New,"Next rollback uses actual kept mode, not old disk preference");
    }
    void TimeoutAndRestore()
    {
        DisplayTrial t; Start(t); auto o=Settle(t); o.seconds=114.999; o.frame=20;
        Check(t.Step(o)==DisplayCommand::None && t.Busy(),"No early timeout");
        o.seconds=115; o.frame=21;
        Check(t.Step(o)==DisplayCommand::RequestRestore,"At deadline issue one restore");
        Check(t.Phase()==DisplayPhase::Reverting,"Request is not restore success");
        Check(t.Cancel(115,21)==DisplayCommand::None,"Repeated cancel no duplicate request");
        o.actual=Old; Check(t.Step(o)==DisplayCommand::None && t.Busy(),"Same-frame old size not accepted as restore acknowledgement");
        o.frame=22; o.seconds=115.1;
        Check(t.Step(o)==DisplayCommand::None && t.Phase()==DisplayPhase::Idle,"Observe prior viewport on later frame");
        Check(t.Step(o)==DisplayCommand::None,"Idle poll does not restart");
        DisplayTrial stalled; Start(stalled); o=At(120,25); o.actual=New;
        Check(stalled.Step(o)==DisplayCommand::RequestRestore,"A long game hitch still expires trial");
        o.seconds=124.999; ++o.frame; stalled.Step(o);
        Check(stalled.Phase()==DisplayPhase::Reverting,"Wait bounded interval for native restoration");
        o.seconds=125; ++o.frame; stalled.Step(o);
        Check(stalled.Phase()==DisplayPhase::Failed,"Unobserved restore is failure, never success");
        o=At(130,50); o.top.id=99;
        Check(stalled.Begin(Old,New,Modes,o)==DisplayCommand::None,"Failed owner cannot silently retry");
        Check(stalled.Cancel(131,51)==DisplayCommand::None,"Failure cannot spam restore");
    }
    void LossAndInvalidClocks()
    {
        for(int fault=0;fault<10;++fault)
        {
            DisplayTrial t; Start(t); auto o=Settle(t); o.seconds=102; ++o.frame;
            if(fault==0) o.ownerReady=false;
            if(fault==1) o.foreground=false;
            if(fault==2) o.top={};
            if(fault==3) ++o.top.epoch;
            if(fault==4) ++o.top.id;
            if(fault==5) o.top.kind=PanelKind::Pause;
            if(fault==6) o.seconds=std::numeric_limits<double>::quiet_NaN();
            if(fault==7) o.seconds=std::numeric_limits<double>::infinity();
            if(fault==8) o.seconds=99;
            if(fault==9) o.frame=9;
            Check(!t.CanKeep(o),"Loss/corrupt context denies keep");
            Check(t.Step(o)==DisplayCommand::RequestRestore,"Loss/corrupt context cancels once");
            Check(t.Cancel(103,13)==DisplayCommand::None,"No repeated restore during fault");
            auto restored=At(104,14); restored.actual=Old; restored.top={};
            t.Step(restored); Check(t.Phase()==DisplayPhase::Idle,"Restore does not require the vanished modal or active campaign");
        }
        DisplayTrial t; Start(t); auto o=At(102,12); o.actual=New; t.Cancel(102,12);
        o.ownerReady=false; o.seconds=107; o.frame=17; t.Step(o);
        Check(t.Phase()==DisplayPhase::Failed,"Lost native binding cannot certify restoration");
        DisplayTrial small; auto s=At(); s.actual={1024,600,DisplayWindowMode::Windowed};
        Check(small.Begin(s.actual,New,Modes,s)==DisplayCommand::RequestCandidate,"Restore snapshot need not be a newly selectable size");
        Check(small.Before()==s.actual,"Small original viewport is preserved exactly");
    }
    void PaintInvalidationAndUI()
    {
        DisplayTrial t; Start(t); auto o=Settle(t); const auto revision=t.PresentationRevision();
        o.frame=12; o.seconds=102; o.actual=Old; t.Step(o);
        Check(!t.Settled() && !t.CanKeep(o),"Mismatch invalidates observed match");
        ++o.frame; ++o.seconds; o.actual=New; o.presented=true; t.Step(o);
        Check(t.PresentationRevision()>revision && !t.CanKeep(o),"Returning to target needs another presentation revision");
        UIFlow f; f.Reset(0); auto parent=f.Push(PanelKind::Session,1);
        auto display=f.Push(PanelKind::Display,2); auto trial=f.Push(PanelKind::ConfirmDisplay,3);
        Check(parent.id && display.id && trial.id,"Display has one existing modal owner");
        Check(!f.CanCommand(trial,3),"No opening-frame confirm");
        Check(!f.CanCommand(display,4),"Collapsed display parent cannot command trial");
        Check(f.ClaimCommand(trial,4),"Top later-frame command accepted");
        Check(!f.ClaimCommand(trial,4),"Duplicate confirm blocked");
        Check(f.Pop(trial,false) && f.IsTop(display),"Back restores display parent on inactive title session");
        Check(f.Pop(display,false) && f.IsTop(parent),"Return to title still works");
        Check(!f.Pop(parent,false),"No gameplay under inactive campaign");
        PresentationGate gate; gate.MarkPainted(10); Check(gate.Ready(11),"Initial paint");
        gate={}; Check(!gate.Ready(12),"Display presentation reset is real");
        gate.MarkPainted(12); Check(!gate.Ready(12) && gate.Ready(13),"Fresh post-mode paint requires later command frame");
    }
    void DeadlineMatrix()
    {
        for(double time : {101.0,114.9,115.0,115.1,200.0})
            for(bool actualMatches : {false,true}) for(bool painted : {false,true})
            {
                DisplayTrial t; Start(t); Settle(t); auto o=At(time,20); o.actual=actualMatches?New:Old; o.presented=painted;
                const bool expected=time<115 && actualMatches && painted;
                Check(t.CanKeep(o)==expected,"Independent keep predicate matrix");
                const auto command=t.Step(o);
                Check(command==(time>=115?DisplayCommand::RequestRestore:DisplayCommand::None),"Independent timeout predicate matrix");
                Check(t.Keep(o)==expected,"Command path agrees with deadline, viewport and paint matrix");
            }
    }
}
int main()
{
    CatalogueTests(); BeginTests(); ConfirmationTests(); TimeoutAndRestore(); LossAndInvalidClocks(); PaintInvalidationAndUI(); DeadlineMatrix();
    std::cout << "Display trial/catalogue/presentation core: " << checks << " checks; " << failures << " failures\n";
    return failures ? 1 : 0;
}
