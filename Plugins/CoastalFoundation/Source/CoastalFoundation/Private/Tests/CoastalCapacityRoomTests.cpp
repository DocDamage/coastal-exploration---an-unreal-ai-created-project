#include "Misc/AutomationTest.h"
#include "Core/TestRoomRules.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalCapacityRoomTest,"Coastal.M1Startup.CapacityManifest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCoastalCapacityRoomTest::RunTest(const FString& Parameters)
{
    std::vector<coastal::RoomObject> Base(coastal::TestRoomManifest().begin(),coastal::TestRoomManifest().end());
    TestTrue(TEXT("Base manifest stays valid"),coastal::AuditTestRoom(Base,1).empty());
    TestFalse(TEXT("Capacity profile requires every fixture pickup"),coastal::AuditTestRoom(Base,1,true).empty());
    auto Full=Base;
    for(int I=1;I<=72;++I)
    {
        const auto Digits=std::to_string(I);
        Full.push_back({"world.test.postcard."+std::string(3-Digits.size(),'0')+Digits,"PICKUP","item.old_postcard","",1,false});
    }
    TestTrue(TEXT("Exact 79-object fixture passes"),coastal::AuditTestRoom(Full,1,true).empty());
    TestFalse(TEXT("Default profile still rejects extra pickups"),coastal::AuditTestRoom(Full,1).empty());
    auto Wrong=Full;Wrong.back().item="item.radio_battery";
    TestFalse(TEXT("Capacity fixture cannot duplicate critical parts"),coastal::AuditTestRoom(Wrong,1,true).empty());
    Wrong=Full;Wrong.back().quantity=2;
    TestFalse(TEXT("Capacity pickups cannot stack"),coastal::AuditTestRoom(Wrong,1,true).empty());
    Wrong=Full;Wrong.back().active=true;
    TestFalse(TEXT("Capacity fixture must start pristine"),coastal::AuditTestRoom(Wrong,1,true).empty());
    Wrong=Full;Wrong.back().id=Wrong.front().id;
    TestFalse(TEXT("Capacity profile rejects duplicate IDs"),coastal::AuditTestRoom(Wrong,1,true).empty());
    return true;
}
#endif
