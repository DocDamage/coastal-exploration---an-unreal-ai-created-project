#include "Core/InventoryViewRules.h"
#include "m1_test_support.h"
#include <climits>
int main()
{
    using namespace coastal;
    TestRun t;
    const std::string id = "container.player";
    const InventoryViewRow battery{"guid1", "item.radio_battery", 1, 0, 0, 1, 2};
    const InventoryViewRow fuse{"guid2", "item.marine_fuse", 1, 2, 0, 1, 1};
    const auto Valid = [&](const std::vector<InventoryViewRow>& items) { return ValidInventoryView(id,id,6,4,2,items); };
    t.Expect(Valid({}), "empty container projection allowed");
    t.Expect(Valid({battery, fuse}), "nonoverlapping valid items");
    t.Expect(!ValidInventoryView(id,"world.remote",6,4,2,{}), "wrong container rejected");
    t.Expect(!ValidInventoryView(id,id,6,4,-1,{}), "missing revision rejected");
    t.Expect(!ValidInventoryView(id,id,0,4,0,{}), "zero grid rejected");
    t.Expect(!ValidInventoryView(id,id,65,4,0,{}), "unbounded grid rejected");
    t.Expect(!Valid({battery,battery}), "duplicate instance rejected");
    auto changed = fuse; changed.x = 0;
    t.Expect(!Valid({battery,changed}), "overlap rejected");
    changed = battery; changed.instance.clear(); t.Expect(!Valid({changed}), "missing instance rejected");
    changed = battery; changed.definition = "../item"; t.Expect(!Valid({changed}), "bad definition rejected");
    changed = battery; changed.quantity = 0; t.Expect(!Valid({changed}), "zero quantity rejected");
    changed.quantity = -1; t.Expect(!Valid({changed}), "negative quantity rejected");
    for (int bad : {-1, 0, 100, INT_MAX})
    {
        changed = battery; changed.width = bad; t.Expect(!Valid({changed}), "invalid item width");
        changed = battery; changed.height = bad; t.Expect(!Valid({changed}), "invalid item height");
    }
    for (int bad : {-1, 100, INT_MAX})
    {
        changed = battery; changed.x = bad; t.Expect(!Valid({changed}), "invalid x including overflow");
        changed = battery; changed.y = bad; t.Expect(!Valid({changed}), "invalid y including overflow");
    }
    std::vector<InventoryViewRow> packed;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 6; ++x) packed.push_back({"i"+std::to_string(y*6+x),"item.fuse",1,x,y,1,1});
    t.Expect(Valid(packed), "full grid valid; view does not infer free space from item count");
    auto colliding = fuse; colliding.instance = "extra"; packed.push_back(colliding);
    t.Expect(!Valid(packed), "overfilled grid rejected");
    std::string set = "stale";
    t.Expect(SaveSetFromSlot("Coastal_m1_test_01_A",set) && set == "m1_test_01", "parse A slot");
    t.Expect(SaveSetFromSlot("Coastal_m1_test_01_B",set) && set == "m1_test_01", "deduplicate pair by set");
    for (const auto* slot : {"", "Other_test_A", "coastal_test_A", "Coastal__A", "Coastal_none_B",
        "Coastal_../x_A", "Coastal_test_A.sav", "Coastal_UPPER_A", "Coastal_test_C", "Coastal_a..b_B",
        "Coastal_test-A_B", "Coastal_test_A/extra", "Coastal_test_aa"})
    { set = "stale"; t.Expect(!SaveSetFromSlot(slot,set) && set.empty(), "reject noncanonical/foreign names without retaining stale output"); }
    t.Expect(SaveSetFromSlot("Coastal_"+std::string(64,'a')+"_A",set), "64 character set accepted");
    t.Expect(!SaveSetFromSlot("Coastal_"+std::string(65,'a')+"_A",set), "oversize set rejected");
    for (int x = 0; x <= 6; ++x)
        for (int y = 0; y <= 4; ++y)
        {
            changed = battery; changed.x = x; changed.y = y;
            t.Expect(Valid({changed}) == (x < 6 && y < 3), "grid boundary matrix");
        }
    return t.Finish("Inventory view/catalogue core");
}
