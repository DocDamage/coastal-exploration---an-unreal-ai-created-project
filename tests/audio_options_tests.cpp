#include "Core/AudioOptionsRules.h"
#include "Core/UIFlowRules.h"
#include "m1_test_support.h"
#include <limits>
using namespace coastal;
int main()
{
    TestRun t;
    const PlayerOptions defaults;
    t.Expect(OptionFieldCount == 10, "ten fields; prior indices preserved");
    t.Expect(static_cast<int>(OptionField::SprintMode) == 5 && static_cast<int>(OptionField::Master) == 6, "append-only fields");
    t.Expect(defaults.masterPercent == 100 && defaults.ambiencePercent == 100
        && defaults.effectsPercent == 100 && defaults.radioPercent == 100, "legacy/default volume neutral");
    for (auto field : {OptionField::Master, OptionField::Ambience, OptionField::Effects, OptionField::Radio})
    {
        PlayerOptions p;
        t.Expect(IsAudioOption(field), "audio field availability classification");
        t.Expect(!AdjustOption(p, field, 1), "upper end unchanged");
        for (int step = 0; step < 20; ++step)
        { t.Expect(AdjustOption(p, field, -1) && ValidOptions(p), "5-percent step toward exact zero"); }
        t.Expect(!AdjustOption(p, field, -1), "lower end unchanged");
        t.Expect(p.mousePercent == 100 && p.stickPercent == 100 && p.fieldOfView == 85
            && p.textPercent == 100 && !p.sprintToggle && !p.invertY, "other option domains untouched");
        for (int step = 0; step < 20; ++step) AdjustOption(p, field, 1);
        t.Expect(p == defaults, "back to defaults without accumulated drift");
    }
    for (auto field : {OptionField::Mouse, OptionField::Text, OptionField::SprintMode, static_cast<OptionField>(99)})
        t.Expect(!IsAudioOption(field), "no other field enables audio");
    for (int bad : {-2147483647, -5, -1, 1, 4, 6, 99, 101, 2147483647})
    for (int field = 0; field < 4; ++field)
    {
        auto p = defaults;
        int* fields[] = {&p.masterPercent, &p.ambiencePercent, &p.effectsPercent, &p.radioPercent};
        *fields[field] = bad; AudioGains gains{1,1,1};
        t.Expect(!ValidOptions(p), "bad percent cannot enter codec or live preferences");
        t.Expect(!BuildAudioGains(p, gains) && gains == AudioGains{}, "invalid data clears gain output");
        t.Expect(!AdjustOption(p, OptionField::Text, 1), "no partial repair of invalid profile");
    }
    // Representative gain matrix verifies independent channel and master multiplication, not perceived loudness.
    for (int master : {0,5,25,50,75,100})
    for (int channel : {0,5,25,50,75,100})
    {
        PlayerOptions p; p.masterPercent = master; p.ambiencePercent = channel;
        p.effectsPercent = 50; p.radioPercent = 25;
        AudioGains gains;
        t.Expect(BuildAudioGains(p, gains), "valid mix");
        t.Expect(std::abs(gains[0] - master/100.0 * channel/100.0) < 1e-12, "master applied exactly once");
        t.Expect(gains[1] == master*50/10000.0 && gains[2] == master*25/10000.0, "channel isolation");
        t.Expect(ValidAudioGains(gains), "no amplification beyond authored sound level");
    }
    PlayerOptions p; p.masterPercent=50; p.radioPercent=0; AudioGains gains;
    BuildAudioGains(p,gains); t.Expect(gains == AudioGains{.5,.5,0}, "zero is real mute request, not epsilon");
    p.masterPercent=0; BuildAudioGains(p,gains); t.Expect(gains == AudioGains{}, "master mute includes every bus");
    auto copy=defaults; copy.mousePercent=175; copy.sprintToggle=true;
    CopyAudioOptions(copy,p); t.Expect(SameAudioOptions(copy,p) && copy.mousePercent==175 && copy.sprintToggle, "defaults/copy affects audio only");
    t.Expect(copy != defaults, "equality includes audio and retained fields");
    p=defaults; p.textPercent=140; t.Expect(SameAudioOptions(p,defaults), "text changes do not resend audio");
    std::array<AudioClassInfo,3> classes{{{1,true,1,1},{2,true,.5,1},{3,true,0,1}}};
    t.Expect(ValidAudioClasses(classes), "three distinct independent routed classes");
    for (int i=0;i<3;++i)
    {
        auto bad=classes; bad[i].identity=0; t.Expect(!ValidAudioClasses(bad), "null class rejected");
        bad=classes; bad[i].identity=classes[(i+1)%3].identity; t.Expect(!ValidAudioClasses(bad), "alias/duplicate rejected");
        bad=classes; bad[i].isolated=false; t.Expect(!ValidAudioClasses(bad), "hierarchy or passive mix rejected");
        for(double value : {-1.0, 1.01, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
        { bad=classes;bad[i].authoredVolume=value;t.Expect(!ValidAudioClasses(bad), "invalid authored gain"); }
        for(double value : {-1.0, 0.0, 4.01, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
        { bad=classes;bad[i].authoredPitch=value;t.Expect(!ValidAudioClasses(bad), "invalid authored pitch"); }
    }
    AudioMixSession session;
    t.Expect(!session.Active() && !session.NeedsUpdate({1,1,1}) && !session.Submitted({1,1,1}), "unbound never updates");
    t.Expect(!session.Begin({1,2,1}), "bad initial gain rejected before push");
    t.Expect(session.Begin({0,.5,1}) && session.Active(), "one valid activation");
    t.Expect(!session.Begin({1,1,1}), "duplicate activation cannot push twice");
    t.Expect(!session.NeedsUpdate({0,.5,1}), "no redundant update");
    for(double bad : {-1.0,1.1,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})
    { t.Expect(!session.NeedsUpdate({bad,0,0}) && !session.Submitted({bad,0,0}), "invalid update leaves last command intact"); }
    t.Expect(!session.NeedsUpdate({0,.5,1}), "invalid attempt did not alter last submitted gains");
    for (int frame=0;frame<100;++frame) t.Expect(!session.NeedsUpdate({0,.5,1}), "unchanged refresh does not resend or repush");
    t.Expect(session.NeedsUpdate({1,0,.25}) && session.Submitted({1,0,.25}), "apply changed effective gains");
    t.Expect(!session.NeedsUpdate({1,0,.25}), "submitted values retained");
    t.Expect(session.Release() && !session.Active(), "one owned mix pop");
    t.Expect(!session.Release() && !session.Begin({1,1,1}) && !session.Submitted({1,1,1}), "teardown idempotent and terminal");
    AudioMixSession unused; t.Expect(!unused.Release() && !unused.Begin({1,1,1}), "release-before-init cannot touch a device");
    // Audio fields use the retained modal dispatcher; no new focus owner.
    UIFlow ui;ui.Reset(7);auto parent=ui.Push(PanelKind::Pause,1);ui.RememberFocus(parent,3);
    auto panel=ui.Push(PanelKind::Settings,2);
    t.Expect(!ui.ClaimCommand(panel,2) && ui.ClaimCommand(panel,3) && !ui.ClaimCommand(panel,3), "same frame/held duplicate blocked");
    t.Expect(ui.Pop(panel,false) && ui.Top()->focus==3, "back restores parent focus");
    ui.Reset(8);t.Expect(!ui.ClaimCommand(panel,4), "old options cannot apply to a new session");
    return t.Finish("Audio gain/routing/lifecycle core");
}
