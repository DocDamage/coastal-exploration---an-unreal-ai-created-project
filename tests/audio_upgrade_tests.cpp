#include "Core/OptionsSession.h"
#include "Core/AudioOptionsRules.h"
#include "fixtures/m1_7_options_fixture.h"
#include "fixtures/m1_6_options_fixture.h"
#include "m1_test_support.h"
#include <limits>
using namespace coastal;
using Bytes=std::vector<std::uint8_t>;
static void Set32(Bytes& b, std::size_t at, std::uint32_t value)
{ for(unsigned i=0;i<4;++i)b[at+i]=static_cast<std::uint8_t>(value>>(8*i)); }
struct DiskFixture
{
    OptionsImage slots[2]; int writes=0,target=-1,fault=0;
    OptionsImage Read(int i) const { return slots[i]; }
    bool Write(int i,const Bytes& b)
    {
        ++writes;target=i;
        if(fault==1)return false;
        slots[i]=ReadOptionsImage(true,true,b);
        if(fault==2)return false; // valid bytes reached disk despite failure return
        if(fault==3){auto bad=b;bad.back()^=1;slots[i]=ReadOptionsImage(true,true,bad);}
        if(fault==4)slots[i]=ReadOptionsImage(true,false,{});
        return true;
    }
};
int main()
{
    TestRun t;
    const Bytes legacy1(std::begin(LegacyM16Options),std::end(LegacyM16Options));
    const Bytes legacy2(M17OptionsFixture.begin(),M17OptionsFixture.end());
    const PlayerOptions expected2{175,225,95,140,true,true};
    OptionsRecord decoded;
    t.Expect(legacy2.size()==64 && DecodeOptions(legacy2.data(),legacy2.size(),decoded)==OptionsStatus::Valid, "actual frozen M1.7 encoder bytes load");
    t.Expect(decoded.sourceSchema==2 && decoded.generation==29 && decoded.values==expected2, "retain all six original fields and neutral volumes");
    auto values=expected2;values.masterPercent=75;values.ambiencePercent=20;values.effectsPercent=0;values.radioPercent=65;
    Bytes modern;t.Expect(EncodeOptions({values,30},modern) && modern.size()==80, "schema3 exact 60+20-byte envelope");
    t.Expect(DecodeOptions(modern.data(),modern.size(),decoded)==OptionsStatus::Valid && decoded.values==values && decoded.sourceSchema==3, "ten-field round trip");
    Bytes payload;DecodeSave(modern.data(),modern.size(),payload);
    for(std::size_t offset : {44u,48u,52u,56u})
    for(std::uint32_t invalid : {1u,4u,99u,101u,0xffffffffu})
    {
        auto p=payload;Set32(p,offset,invalid);Bytes bad;EncodeSave(p.data(),p.size(),bad);
        t.Expect(DecodeOptions(bad.data(),bad.size(),decoded)==OptionsStatus::Corrupt && decoded.generation==0, "CRC-valid invalid volume fails without partial output");
    }
    // Exact schema lengths, including CRC-valid truncation/extension and unsupported future versions.
    for(std::uint32_t schema : {1u,2u,3u})
    for(std::size_t size : {36u,40u,44u,48u,56u,60u,64u})
    {
        auto p=payload;p.resize(size);Set32(p,8,schema);Bytes data;EncodeSave(p.data(),p.size(),data);
        const bool valid=(schema==1&&size==40)||(schema==2&&size==44)||(schema==3&&size==60);
        t.Expect((DecodeOptions(data.data(),data.size(),decoded)==OptionsStatus::Valid)==valid, "schema size exact, no absent field reads");
    }
    for(const auto& old : {legacy1,legacy2})
    for(int source : {0,1})
    {
        DiskFixture disk;disk.slots[source]=ReadOptionsImage(true,true,old);OptionsSession session;
        auto read=[&](int i){return disk.Read(i);};auto write=[&](int i,const Bytes& b){return disk.Write(i,b);};
        t.Expect(session.Initialize(read) && session.NeedsFormatUpgrade(), "old record selected");
        const auto prior=session.Get();const auto generation=disk.slots[source].slot.record.generation;
        t.Expect(disk.writes==0 && SameAudioOptions(prior,PlayerOptions{}), "no auto-write and missing audio defaults 100 percent");
        auto draft=prior;CopyAudioOptions(draft,values);
        t.Expect(session.ApplySession(draft) && disk.writes==0 && disk.slots[source].bytes==old, "session-only does not upgrade file");
        OptionsSession restarted;t.Expect(restarted.Initialize(read) && restarted.Get()==prior, "relaunch discards session-only values");
        t.Expect(session.SaveAndApply(draft,read,write), "explicit verified upgrade");
        t.Expect(disk.writes==1 && disk.target==1-source && disk.slots[source].bytes==old, "inactive slot only; prior valid generation intact");
        t.Expect(disk.slots[1-source].slot.record.sourceSchema==3 && disk.slots[1-source].slot.record.generation==generation+1, "schema3 monotonic generation");
        t.Expect(session.Get()==draft && session.SourceSchema()==3 && !session.NeedsFormatUpgrade(), "publish live values only after verified save");
        OptionsSession after;t.Expect(after.Initialize(read) && after.Get()==draft, "relaunch selects upgraded record");
    }
    for(int fault=1;fault<=4;++fault)
    {
        DiskFixture disk;disk.slots[0]=ReadOptionsImage(true,true,legacy2);disk.fault=fault;OptionsSession session;
        auto read=[&](int i){return disk.Read(i);};auto write=[&](int i,const Bytes& b){return disk.Write(i,b);};
        t.Expect(session.Initialize(read), "failure fixture loads real old bytes");
        t.Expect(!session.SaveAndApply(values,read,write) && !session.CanWrite(), "unverified write locks further writes");
        t.Expect(session.Get()==expected2 && disk.slots[0].bytes==legacy2, "previous live options and good slot retained");
        t.Expect(!session.SaveAndApply(values,read,write) && disk.writes==1, "no retry overwrites ambiguous result");
        t.Expect(session.ApplySession(values) && disk.writes==1, "session-only remains usable");
        if(fault==2){OptionsSession relaunched;t.Expect(relaunched.Initialize(read) && relaunched.Get()==values, "failure does not imply disk unchanged");}
    }
    DiskFixture mixed;mixed.slots[0]=ReadOptionsImage(true,true,legacy1);mixed.slots[1]=ReadOptionsImage(true,true,legacy2);
    OptionsSession latest;t.Expect(latest.Initialize([&](int i){return mixed.Read(i);}) && latest.Get()==expected2 && latest.SourceSchema()==2, "v2 newer than v1 preserved");
    Bytes same;EncodeOptions({values,29},same);
    t.Expect(SelectOptions(ReadOptionsImage(true,true,legacy2).slot,ReadOptionsImage(true,true,same).slot).selected==-1, "equal different schemas never guessed");
    auto future=payload;Set32(future,8,4);Bytes futureBytes;EncodeSave(future.data(),future.size(),futureBytes);
    t.Expect(DecodeOptions(futureBytes.data(),futureBytes.size(),decoded)==OptionsStatus::Unsupported, "unknown schema4 blocked");
    t.Expect(!SelectOptions(ReadOptionsImage(true,true,legacy2).slot,ReadOptionsImage(true,true,futureBytes).slot).writable, "do not overwrite unsupported peer");
    DiskFixture recovery;recovery.slots[0]=ReadOptionsImage(true,true,legacy2);auto corrupt=modern;corrupt.back()^=1;recovery.slots[1]=ReadOptionsImage(true,true,corrupt);
    OptionsSession recovered;t.Expect(recovered.Initialize([&](int i){return recovery.Read(i);}) && recovered.Get()==expected2 && recovered.Notice()==OptionsNotice::Recovered, "damaged3 recovers actual schema2 with notice");
    DiskFixture exhausted;Bytes max;EncodeOptions({values,std::numeric_limits<std::uint64_t>::max()},max);exhausted.slots[0]=ReadOptionsImage(true,true,max);
    OptionsSession capped;t.Expect(capped.Initialize([&](int i){return exhausted.Read(i);}) && !capped.CanWrite() && capped.Get()==values, "generation exhaustion allows read not wrap");
    DiskFixture changed;changed.slots[0]=ReadOptionsImage(true,true,legacy2);OptionsSession guarded;guarded.Initialize([&](int i){return changed.Read(i);});changed.slots[1]=ReadOptionsImage(true,true,modern);
    t.Expect(!guarded.SaveAndApply(values,[&](int i){return changed.Read(i);},[&](int i,const Bytes& b){return changed.Write(i,b);})
        && changed.writes==0 && guarded.Get()==expected2, "external edits block before writing/applying");
    // An all-muted profile retains category choices; unmuting master is not a reset.
    auto muted=values;muted.masterPercent=0;EncodeOptions({muted,31},modern);DecodeOptions(modern.data(),modern.size(),decoded);
    t.Expect(decoded.values==muted, "master mute saved without replacing per-category values");
    decoded.values.masterPercent=100;AudioGains gains;BuildAudioGains(decoded.values,gains);
    t.Expect(gains==AudioGains{.2,0,.65}, "unmute restores category multipliers");
    return t.Finish("Audio preference v1-v2-v3 upgrade core");
}
