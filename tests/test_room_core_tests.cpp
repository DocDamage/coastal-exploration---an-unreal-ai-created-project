#include "Core/TestRoomRules.h"
#include "m1_test_support.h"
#include <algorithm>
int main()
{
    using namespace coastal;
    TestRun t;
    const std::vector<RoomObject> good(TestRoomManifest().begin(),TestRoomManifest().end());
    auto has=[](const std::vector<RoomIssue>& issues,const std::string& code,const std::string& subject="")
    {
        return std::any_of(issues.begin(),issues.end(),[&](const auto& i)
        {return i.code==code && (subject.empty() || i.subject==subject);});
    };
    t.Expect(AuditTestRoom(good,1).empty(),"exact pristine M1 room accepted");
    auto reversed=good;std::reverse(reversed.begin(),reversed.end());
    t.Expect(AuditTestRoom(reversed,1).empty(),"actor iteration order immaterial");
    for(int directors : {-1,0,2,15})
        t.Expect(has(AuditTestRoom(good,directors),"room.director_count"),"one director only");
    for(std::size_t i=0;i<good.size();++i)
    {
        auto rows=good;rows.erase(rows.begin()+static_cast<std::ptrdiff_t>(i));
        t.Expect(has(AuditTestRoom(rows,1),"room.missing_object",good[i].id),"missing actor identified");
        rows=good;rows.push_back(rows[i]);
        t.Expect(has(AuditTestRoom(rows,1),"room.duplicate_id",good[i].id),"duplicate actor identified");
        rows=good;rows[i].kind="INVALID";
        t.Expect(has(AuditTestRoom(rows,1),"room.kind_mismatch",good[i].id),"wrong kind rejected");
        rows=good;rows[i].active=true;
        t.Expect(has(AuditTestRoom(rows,1),"room.not_pristine",good[i].id),"pre-restored actor rejected");
        rows=good;rows[i].journal="journal.wrong";
        t.Expect(has(AuditTestRoom(rows,1),"room.journal_mismatch",good[i].id),"wrong journal mapping rejected");
        rows=good;rows[i].item="item.wrong";
        t.Expect(has(AuditTestRoom(rows,1),"room.item_mismatch",good[i].id),"wrong item mapping rejected");
    }
    for(const auto& bad : {"", "world..test", "World.test.battery", "world/test", ".leading", "ending."})
    {
        auto rows=good;rows[3].id=bad;
        t.Expect(has(AuditTestRoom(rows,1),"room.invalid_id"),"invalid logical ID rejected");
    }
    for(int quantity : {-1,0,2,2147483647})
        for(std::size_t i : {std::size_t{3},std::size_t{4}})
        {
            auto rows=good;rows[i].quantity=quantity;
            t.Expect(has(AuditTestRoom(rows,1),"room.item_mismatch"),"one guaranteed part only");
        }
    auto extras=good;extras.push_back({"world.other","DOOR","","",1,false});
    t.Expect(has(AuditTestRoom(extras,1),"room.unexpected_object"),"extra persistent IDs require deliberate new manifest");
    t.Expect(has(AuditTestRoom(std::vector<RoomObject>(257,good[0]),1),"room.too_many_objects"),"bounded audit work");
    // Every subset proves a missing optional actor is an authoring error, not a quest prerequisite.
    for(unsigned mask=0;mask<128;++mask)
    {
        std::vector<RoomObject> rows;
        for(unsigned i=0;i<7;++i) if(mask & (1u<<i)) rows.push_back(good[i]);
        const auto issues=AuditTestRoom(rows,1);
        t.Expect(issues.empty()==(mask==127),"only the complete authored test room passes");
    }
    const auto unchanged=AuditTestRoom(good,1);
    t.Expect(unchanged.empty() && good[3].quantity==1 && !good[0].active,"validation leaves source data unchanged");
    return t.Finish("Test-room manifest core");
}
