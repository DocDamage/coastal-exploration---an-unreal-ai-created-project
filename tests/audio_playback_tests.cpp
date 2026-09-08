#include "Core/AudioPlaybackRules.h"
#include <iostream>
#include <limits>
#include <array>
using namespace coastal;
namespace
{
    int checks = 0, failures = 0;
    void Check(bool condition, const char* label)
    { ++checks; if (!condition) { ++failures; std::cerr << "FAIL: " << label << '\n'; } }
    AudioPlaybackContext World(std::uint64_t epoch = 1)
    { AudioPlaybackContext c; c.epoch = epoch; c.active = true; c.interrupted = false; c.ambienceAllowed = true; return c; }
    AudioPlaybackContext Text(std::uint64_t ticket = 1, bool painted = true, std::uint64_t epoch = 1)
    { auto c = World(epoch); c.ambienceAllowed = false; c.transcript = {ticket, epoch, PanelKind::Transcript}; c.presented = painted; return c; }
    // Exact command masks expose accidental extra start/resume/stop requests.
    unsigned Mask(const AudioPlaybackCommands& c)
    {
        return (c.stopAmbience ? 1u : 0u) | (c.startAmbience ? 2u : 0u) | (c.pauseAmbience ? 4u : 0u)
            | (c.resumeAmbience ? 8u : 0u) | (c.stopRadio ? 16u : 0u) | (c.startRadio ? 32u : 0u);
    }
    void Expect(AudioPlaybackSession& s, const AudioPlaybackContext& c, unsigned mask, const char* label)
    { Check(Mask(s.Step(c)) == mask, label); }
    void SourceContracts()
    {
        Check(ValidAmbienceSource({true,true,true,0}), "Loop needs no finite duration estimate");
        Check(!ValidAmbienceSource({false,true,true,1}), "Unplayable loop");
        Check(!ValidAmbienceSource({true,false,true,1}), "Not a loop");
        Check(!ValidAmbienceSource({true,true,false,1}), "No silent-loop continuation contract");
        Check(ValidRadioSource({true,false,true,1}), "Finite radio");
        Check(ValidRadioSource({true,false,true,900}), "Maximum admitted duration");
        Check(!ValidRadioSource({true,false,true,900.01}), "Overlong radio");
        Check(!ValidRadioSource({true,true,true,1}), "Loop cannot be radio");
        Check(!ValidRadioSource({false,false,true,1}), "Unplayable radio");
        Check(!ValidRadioSource({true,false,false,1}), "No silent-radio continuation contract");
        for (double d : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
            Check(!ValidRadioSource({true,false,true,d}), "Invalid radio duration");
    }
    void Lifetime()
    {
        AudioPlaybackSession s;
        Check(s.Step(World()).Empty(), "Uninitialized is silent");
        Check(!s.Begin(false,false), "No phantom source");
        Check(s.Begin(true,true), "Explicit configuration");
        Check(!s.Begin(true,true), "No duplicate begin");
        Expect(s, {}, 0, "No title-screen autoplay");
        auto c=World(); c.ambienceAllowed=false;
        Expect(s,c,0,"Active campaign under initial modal stays silent");
        Expect(s,World(),2,"One initial bed");
        Expect(s,World(),0,"No repeated play");
        c=World(); c.ambienceAllowed=false;
        Expect(s,c,4,"Pause under menu"); Expect(s,c,0,"Pause idempotent");
        Expect(s,World(),8,"Resume same bed"); Expect(s,World(),0,"Resume idempotent");
        Check(Mask(s.Release())==1,"Release owns one bed");
        Check(s.Release().Empty(),"Release idempotent");
        Check(!s.Begin(true,true),"No rebind after release");
        Check(s.Step(Text()).Empty(),"No playback after teardown");
    }
    void Transcript()
    {
        AudioPlaybackSession s; Check(s.Begin(true,true),"Transcript configuration");
        Expect(s,World(),2,"Start bed");
        Expect(s,Text(1,false),4,"Pause bed before painted transcript");
        Expect(s,Text(1,false),0,"No audio for unpainted text");
        Expect(s,Text(),32,"Painted transcript plays once while modal");
        // There is intentionally no audio-finished input. Repeated frames after completion
        // or native failure do not retry. This is command logic, not actual device playback.
        for (int i=0;i<20;++i) Expect(s,Text(),0,"No loop/retry after natural finish or failed Play");
        Expect(s,World(),24,"Back stops voice and resumes bed");
        Expect(s,Text(),4,"Closed ticket cannot replay");
        Expect(s,Text(2,false),0,"New panel waits for paint");
        Expect(s,Text(2),32,"Explicit reinteraction permits one replay");
        Expect(s,Text(3),48,"Replace voice: stop old before start new");
        Expect(s,Text(2),16,"Stale lower ticket cannot replace current voice");
        Expect(s,Text(3),0,"Newer retired voice does not resurrect");
        Expect(s,Text(4),32,"Later deliberate panel is still valid");
        Check(Mask(s.Release())==17,"Release stops both owned sources");
    }
    void Interruptions()
    {
        AudioPlaybackSession s; s.Begin(true,true); s.Step(World()); s.Step(Text());
        auto blocked=Text(); blocked.interrupted=true;
        Expect(s,blocked,16,"Return/load/IO/recovery stops radio");
        Expect(s,blocked,0,"Repeated interruption is silent");
        Expect(s,Text(),0,"Ending interruption cannot restart old transcript");
        Expect(s,World(),8,"Bed resumes after interruption");
        Check(Mask(s.Suspend())==4,"Immediate menu-clearing suspension pauses bed");
        Check(s.Suspend().Empty(),"Repeated synchronous suspension safe");
        Expect(s,Text(2,false),0,"Track new unpainted panel");
        Check(s.Suspend().Empty(),"Cancel before painting does not invent a Stop");
        Expect(s,Text(2),0,"Canceled unpainted panel cannot later auto-play");
        Expect(s,Text(3),32,"Fresh panel after cancellation works");
        auto inactive=World(); inactive.active=false;
        Expect(s,inactive,17,"Inactive campaign stops rather than pauses");
        Expect(s,inactive,0,"Inactive repeats do nothing");
        Expect(s,World(),0,"Same session cannot silently restart a retired bed");
    }
    void EpochsAndMalformedPanels()
    {
        AudioPlaybackSession s; s.Begin(true,true); s.Step(World()); s.Step(Text(5));
        Expect(s,World(2),19,"New campaign stops both old sources then starts new bed");
        Expect(s,Text(6,true,1),4,"Old epoch only suspends current bed");
        Expect(s,World(2),8,"Current epoch resumes without duplicate start");
        auto wrong=Text(6,true,2); wrong.transcript.epoch=1;
        Expect(s,wrong,4,"Wrong ticket epoch cannot play");
        wrong=Text(6,true,2); wrong.transcript.kind=PanelKind::Journal;
        Expect(s,wrong,0,"Journal is not a voice trigger");
        wrong=Text(0,true,2); Expect(s,wrong,0,"Zero ticket invalid");
        Expect(s,Text(5,true,2),0,"UI ticket IDs never reset across epochs");
        Expect(s,Text(6,true,2),32,"Fresh epoch and later ticket valid");
        Expect(s,Text(7,false,3),17,"New session retires old voice without playing unpainted text");
        Expect(s,Text(7,true,3),32,"New session may start radio before its bed");
        Expect(s,World(3),18,"Close radio then begin first gameplay bed");
        auto zero=World(0); Expect(s,zero,4,"Zero/stale projection does not roll back epoch");
        Expect(s,World(3),8,"Highest observed epoch retained");
    }
    void OptionalChannelsAndMuteIndependence()
    {
        // No volume or mission state enters this session. Channel settings are applied by
        // the separate mix; zero gain does not change commands or acknowledge a transcript.
        for (const auto channels : {std::array<bool,2>{true,false}, {false,true}, {true,true}})
        {
            AudioPlaybackSession s; Check(s.Begin(channels[0],channels[1]),"Partial source configuration");
            Expect(s,World(),channels[0]?2:0,"Only configured bed starts");
            Expect(s,Text(),(channels[0]?4:0)|(channels[1]?32:0),"Only configured radio starts");
            Expect(s,Text(),0,"Mute/unmute cannot cause play/replay commands");
            Expect(s,World(),(channels[0]?8:0)|(channels[1]?16:0),"Independent channel cleanup");
            Check(Mask(s.Release())==(channels[0]?1u:0u),"No unowned stop");
        }
        AudioPlaybackSession s; s.Release();
        Check(!s.Begin(true,false),"Release-before-initialize is terminal");
        Check(s.Suspend().Empty(),"Uninitialized suspension is harmless");
    }
}
int main()
{
    SourceContracts(); Lifetime(); Transcript(); Interruptions(); EpochsAndMalformedPanels(); OptionalChannelsAndMuteIndependence();
    std::cout << "Audio playback/presentation core: " << checks << " checks; " << failures << " failures\n";
    return failures ? 1 : 0;
}
