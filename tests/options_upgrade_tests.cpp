#include "Core/OptionsSession.h"
#include "fixtures/m1_6_options_fixture.h"
#include <iostream>
#include <limits>
using namespace coastal;
static int checks = 0, failures = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::cerr << "line " << __LINE__ << ": " << #x << '\n'; } } while (false)
static void Set32(std::vector<std::uint8_t>& b,std::size_t at,std::uint32_t value)
{ for(unsigned i=0;i<4;++i)b[at+i]=static_cast<std::uint8_t>(value>>(i*8)); }
struct Disk
{
    OptionsImage files[2]; int writes=0,target=-1,mode=0;
    OptionsImage Read(int i) const { return files[i]; }
    bool Write(int i,const std::vector<std::uint8_t>& bytes)
    {
        ++writes;target=i;
        if(mode==1)return false;
        files[i]=ReadOptionsImage(true,true,bytes);
        if(mode==2)return false; // reached disk but reported failure
        if(mode==3){auto corrupt=bytes;corrupt.back()^=1;files[i]=ReadOptionsImage(true,true,corrupt);}
        if(mode==4)files[i]=ReadOptionsImage(true,false,{});
        return true;
    }
};
int main()
{
    const std::vector<std::uint8_t> legacy(std::begin(LegacyM16Options),std::end(LegacyM16Options));
    OptionsRecord restored; CHECK(legacy.size()==60);
    CHECK(DecodeOptions(legacy.data(),legacy.size(),restored)==OptionsStatus::Valid);
    CHECK(restored.generation==17 && restored.sourceSchema==1);
    const PlayerOptions oldValues{200,150,95,140,true};
    CHECK(restored.values==oldValues && !restored.values.sprintToggle);
    // Decode does not turn CRC-valid malformed legacy payloads into valid settings.
    std::vector<std::uint8_t> payload; CHECK(DecodeSave(legacy.data(),legacy.size(),payload)==EnvelopeStatus::Valid);
    for(std::size_t i=0;i<legacy.size();++i)
    { auto bad=legacy;bad[i]^=1; CHECK(DecodeOptions(bad.data(),bad.size(),restored)!=OptionsStatus::Valid); CHECK(restored.generation==0); }
    for(auto mutation : {std::pair<std::size_t,std::uint32_t>{20,0},{24,299},{28,69},{32,151},{36,2}})
    {
        auto p=payload;Set32(p,mutation.first,mutation.second);std::vector<std::uint8_t> b;
        CHECK(EncodeSave(p.data(),p.size(),b));CHECK(DecodeOptions(b.data(),b.size(),restored)==OptionsStatus::Corrupt);
    }
    // Explicit migration alone emits the current schema and leaves the previous legacy file intact.
    for(int slot : {0,1})
    {
        Disk disk;disk.files[slot]=ReadOptionsImage(true,true,legacy);OptionsSession state;
        CHECK(state.Initialize([&](int i){return disk.Read(i);}));CHECK(state.UsesLegacyRecord());
        CHECK(state.Get()==oldValues);CHECK(disk.writes==0);CHECK(disk.files[slot].bytes==legacy);
        auto toggle=oldValues;toggle.sprintToggle=true;
        CHECK(state.ApplySession(toggle));CHECK(state.UsesLegacyRecord());CHECK(disk.writes==0);
        OptionsSession before;CHECK(before.Initialize([&](int i){return disk.Read(i);}));CHECK(!before.Get().sprintToggle);
        CHECK(state.SaveAndApply(toggle,[&](int i){return disk.Read(i);},[&](int i,const auto& b){return disk.Write(i,b);}));
        CHECK(disk.writes==1 && disk.target==1-slot);CHECK(disk.files[slot].bytes==legacy);
        CHECK(disk.files[1-slot].bytes.size()==OptionsFileSize);CHECK(!state.UsesLegacyRecord());CHECK(state.Get()==toggle);
        CHECK(disk.files[1-slot].slot.record.sourceSchema==OptionsSchema && disk.files[1-slot].slot.record.generation==18);
        OptionsSession after;CHECK(after.Initialize([&](int i){return disk.Read(i);}));CHECK(after.Get()==toggle);CHECK(!after.UsesLegacyRecord());
    }
    // Every failure mode retains former LIVE options and the untouched old valid file.
    for(int failure=1;failure<=4;++failure)
    {
        Disk disk;disk.mode=failure;disk.files[0]=ReadOptionsImage(true,true,legacy);OptionsSession state;
        CHECK(state.Initialize([&](int i){return disk.Read(i);}));auto changed=oldValues;changed.sprintToggle=true;
        CHECK(!state.SaveAndApply(changed,[&](int i){return disk.Read(i);},[&](int i,const auto& b){return disk.Write(i,b);}));
        CHECK(!state.CanWrite());CHECK(state.Get()==oldValues);CHECK(disk.files[0].bytes==legacy);
        CHECK(state.Notice()==OptionsNotice::WriteUnverified);CHECK(disk.writes==1 && disk.target==1);
        if(failure==2){OptionsSession after;CHECK(after.Initialize([&](int i){return disk.Read(i);}));CHECK(after.Get()==changed);}
    }
    // Strict schema/size matching: a truncated current record cannot masquerade as a legacy record.
    std::vector<std::uint8_t> modern; auto values=oldValues;values.sprintToggle=true;
    CHECK(EncodeOptions({values,18},modern)); std::vector<std::uint8_t> p;
    CHECK(DecodeSave(modern.data(),modern.size(),p)==EnvelopeStatus::Valid);
    for(auto version : {0u,4u,0xffffffffu})
    {
        auto future=p;Set32(future,8,version);std::vector<std::uint8_t> bytes;CHECK(EncodeSave(future.data(),future.size(),bytes));
        CHECK(DecodeOptions(bytes.data(),bytes.size(),restored)==OptionsStatus::Unsupported);
        const auto selected=SelectOptions(ReadOptionsImage(true,true,legacy).slot,ReadOptionsImage(true,true,bytes).slot);
        CHECK(!selected.writable && selected.selected<0);
    }
    for(int shape=0;shape<4;++shape)
    {
        auto bad=p;
        if(shape==0)Set32(bad,8,1); // v1 with an appended field
        if(shape==1)bad.resize(40); // current record missing its added fields
        if(shape==2)Set32(bad,40,2);
        if(shape==3)Set32(bad,40,0xffffffffu);
        std::vector<std::uint8_t> bytes;CHECK(EncodeSave(bad.data(),bad.size(),bytes));
        CHECK(DecodeOptions(bytes.data(),bytes.size(),restored)==OptionsStatus::Corrupt);
    }
    // Unknown/newer generation, equality, corruption fallback and legacy generation exhaustion.
    CHECK(EncodeOptions({values,17},modern));
    CHECK(!SelectOptions(ReadOptionsImage(true,true,legacy).slot,ReadOptionsImage(true,true,modern).slot).writable);
    CHECK(EncodeOptions({values,18},modern));auto corrupt=modern;corrupt.back()^=1;
    Disk fallback;fallback.files[0]=ReadOptionsImage(true,true,legacy);fallback.files[1]=ReadOptionsImage(true,true,corrupt);
    OptionsSession recovered;CHECK(recovered.Initialize([&](int i){return fallback.Read(i);}));
    CHECK(recovered.UsesLegacyRecord() && recovered.Get()==oldValues && recovered.Notice()==OptionsNotice::Recovered);
    auto exhausted=payload;Set32(exhausted,12,0xffffffffu);Set32(exhausted,16,0xffffffffu);
    std::vector<std::uint8_t> bytes;CHECK(EncodeSave(exhausted.data(),exhausted.size(),bytes));
    Disk max;max.files[0]=ReadOptionsImage(true,true,bytes);OptionsSession capped;
    CHECK(capped.Initialize([&](int i){return max.Read(i);}));CHECK(capped.Get()==oldValues && capped.UsesLegacyRecord());CHECK(!capped.CanWrite());
    // External edits between initialization and explicit migration must block before writing.
    Disk changed;changed.files[0]=ReadOptionsImage(true,true,legacy);OptionsSession guarded;
    CHECK(guarded.Initialize([&](int i){return changed.Read(i);}));changed.files[1]=ReadOptionsImage(true,true,modern);
    CHECK(!guarded.SaveAndApply(values,[&](int i){return changed.Read(i);},[&](int i,const auto& b){return changed.Write(i,b);}));
    CHECK(changed.writes==0 && guarded.Notice()==OptionsNotice::DiskChanged && guarded.Get()==oldValues);
    std::cout << "M1.6 preference upgrade core: " << checks << " checks; " << failures << " failures\n";
    return failures?1:0;
}
