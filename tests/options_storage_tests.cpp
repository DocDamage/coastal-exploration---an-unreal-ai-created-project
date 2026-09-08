#include "Core/OptionsSession.h"
#include <iostream>
using namespace coastal;
static int checks = 0, failures = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::cerr << "line " << __LINE__ << ": " << #x << '\n'; } } while (false)
// In-memory callback fixture for the REAL shared protocol, not a fake Unreal/AGIS runtime.
struct Disk
{
    bool exists[2]{false, false}, readError[2]{false, false};
    std::vector<std::uint8_t> data[2];
    int reads = 0, writes = 0, lastTarget = -1;
    bool failBefore = false, failAfter = false, corruptAfter = false, unreadableAfter = false;
    OptionsImage Read(int i) { ++reads; return ReadOptionsImage(exists[i], !readError[i], data[i]); }
    bool Write(int i, const std::vector<std::uint8_t>& bytes)
    {
        ++writes; lastTarget = i; if (failBefore) return false;
        exists[i] = true; data[i] = bytes;
        if (corruptAfter) data[i].back() ^= 1;
        if (unreadableAfter) readError[i] = true;
        return !failAfter;
    }
    void Seed(int i, PlayerOptions p, std::uint64_t generation)
    { exists[i] = true; CHECK(EncodeOptions({p, generation}, data[i])); }
};
static bool Init(OptionsSession& s, Disk& d) { return s.Initialize([&](int i) { return d.Read(i); }); }
static bool Save(OptionsSession& s, Disk& d, const PlayerOptions& p)
{ return s.SaveAndApply(p, [&](int i) { return d.Read(i); }, [&](int i, const auto& b) { return d.Write(i, b); }); }
int main()
{
    const PlayerOptions defaults;
    PlayerOptions edited{200, 150, 95, 140, true};
    Disk d; OptionsSession s;
    CHECK(!s.ApplySession(edited)); CHECK(!Save(s, d, edited)); CHECK(d.reads == 0 && d.writes == 0);
    CHECK(Init(s, d)); CHECK(d.reads == 2 && d.writes == 0); CHECK(s.CanWrite()); CHECK(s.Notice() == OptionsNotice::Defaults);
    CHECK(!Init(s, d)); CHECK(d.reads == 2);
    CHECK(s.ApplySession(edited)); CHECK(s.Get() == edited); CHECK(d.writes == 0);
    CHECK(Save(s, d, edited)); CHECK(d.lastTarget == 0); CHECK(s.Notice() == OptionsNotice::Saved);
    const auto first = d.data[0]; CHECK(Save(s, d, defaults)); CHECK(d.lastTarget == 1); CHECK(d.data[0] == first);
    OptionsSession restarted; CHECK(Init(restarted, d)); CHECK(restarted.Get() == defaults);
    CHECK(restarted.ApplySession(edited)); OptionsSession unsavedRestart; CHECK(Init(unsavedRestart, d)); CHECK(unsavedRestart.Get() == defaults);
    auto invalid = edited; invalid.textPercent = 151;
    const int oldWrites = d.writes; CHECK(!Save(s, d, invalid)); CHECK(!s.ApplySession(invalid)); CHECK(d.writes == oldWrites);
    // Explicit apply after one damaged slot retains the remaining good slot until verification.
    d.data[1].back() ^= 1; OptionsSession recovered; CHECK(Init(recovered, d));
    CHECK(recovered.Notice() == OptionsNotice::Recovered); CHECK(recovered.Get() == edited);
    CHECK(Save(recovered, d, defaults)); CHECK(d.lastTarget == 1 && d.data[0] == first);
    // No safe selection never becomes an implicit reset of user files.
    for (int mode = 0; mode < 4; ++mode)
    {
        Disk bad; bad.Seed(0, edited, 7); bad.Seed(1, defaults, 8);
        if (mode == 0) { bad.data[0].back() ^= 1; bad.data[1].back() ^= 1; }
        if (mode == 1) bad.readError[1] = true;
        if (mode == 2) bad.Seed(1, defaults, 7); // ambiguous equal generation
        if (mode == 3) bad.data[1][8] = 2; // unsupported outer version
        const auto a = bad.data[0], b = bad.data[1]; OptionsSession blocked;
        CHECK(Init(blocked, bad)); CHECK(!blocked.CanWrite()); CHECK(blocked.Notice() == OptionsNotice::Blocked);
        CHECK(!Save(blocked, bad, edited)); CHECK(bad.writes == 0); CHECK(blocked.ApplySession(edited));
        CHECK(blocked.Get() == edited); CHECK(!blocked.CanWrite()); CHECK(bad.data[0] == a && bad.data[1] == b);
    }
    // All ambiguous write outcomes retain previous live settings and prevent a second write.
    for (int mode = 0; mode < 4; ++mode)
    {
        Disk fault; fault.Seed(0, defaults, 1); OptionsSession state; CHECK(Init(state, fault));
        const auto good = fault.data[0];
        fault.failBefore = mode == 0; fault.failAfter = mode == 1; fault.corruptAfter = mode == 2; fault.unreadableAfter = mode == 3;
        CHECK(!Save(state, fault, edited)); CHECK(state.Notice() == OptionsNotice::WriteUnverified);
        CHECK(state.Get() == defaults); CHECK(fault.data[0] == good); CHECK(fault.lastTarget == 1);
        CHECK(!state.CanWrite()); CHECK(!Save(state, fault, edited)); CHECK(fault.writes == 1);
        CHECK(state.ApplySession(edited)); CHECK(state.Get() == edited); CHECK(!state.CanWrite());
        if (mode == 1) { OptionsSession after; CHECK(Init(after, fault)); CHECK(after.Get() == edited); }
    }
    // An external writer, removal, or read error blocks BEFORE attempting a write.
    for (int mode = 0; mode < 3; ++mode)
    {
        Disk changed; changed.Seed(0, defaults, 1); OptionsSession state; CHECK(Init(state, changed));
        if (mode == 0) changed.Seed(1, edited, 2);
        if (mode == 1) changed.exists[0] = false;
        if (mode == 2) changed.readError[0] = true;
        CHECK(!Save(state, changed, edited)); CHECK(state.Notice() == OptionsNotice::DiskChanged);
        CHECK(changed.writes == 0); CHECK(state.Get() == defaults);
    }
    // Callback reentry cannot apply session values, save, or initialize a half-owned transaction.
    Disk recursive; OptionsSession guarded;
    CHECK(guarded.Initialize([&](int i) {
        CHECK(!guarded.ApplySession(edited)); CHECK(!Save(guarded, recursive, edited));
        CHECK(!Init(guarded, recursive)); return recursive.Read(i);
    }));
    CHECK(guarded.SaveAndApply(edited, [&](int i) { CHECK(!guarded.ApplySession(defaults)); return recursive.Read(i); },
        [&](int i, const auto& bytes) {
            CHECK(!guarded.CanWrite()); CHECK(!Save(guarded, recursive, defaults)); return recursive.Write(i, bytes);
        }));
    CHECK(guarded.Get() == edited); CHECK(recursive.writes == 1);
    std::cout << "Options storage/session core: " << checks << " checks; " << failures << " failures\n";
    return failures ? 1 : 0;
}
