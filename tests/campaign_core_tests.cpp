#include "Core/CampaignRules.h"
#include "Core/TransactionRules.h"
#include "m1_test_support.h"
#include <array>
#include <algorithm>
int main()
{
    using namespace coastal;
    TestRun t;
    const SlotInfo missing{}, valid1{SlotStatus::Valid, 1}, valid2{SlotStatus::Valid, 2};
    const SlotInfo corrupt{SlotStatus::Corrupt, 0}, future{SlotStatus::Incompatible, 0};
    t.Expect(ChooseSlot(missing, missing).index == -1, "no saves");
    t.Expect(ChooseSlot(valid1, valid2).index == 1, "highest generation B");
    t.Expect(ChooseSlot(valid2, valid1).index == 0, "highest generation A");
    t.Expect(ChooseSlot(corrupt, valid1).recovered, "A corruption recovery");
    t.Expect(ChooseSlot(valid1, corrupt).recovered, "B corruption recovery");
    t.Expect(ChooseSlot(valid1, missing).index == 0 && !ChooseSlot(valid1, missing).recovered, "first save is not recovery");
    t.Expect(ChooseSlot(corrupt, corrupt).index == -1, "both damaged");
    t.Expect(ChooseSlot(valid1, future).blocked, "no downgrade overwrite");
    t.Expect(ChooseSlot(future, valid1).blocked, "future A also blocks");
    t.Expect(ChooseSlot(valid1, valid1).blocked, "ambiguous tied generations");
    t.Expect(ChooseSlot({SlotStatus::Valid, 0}, missing).blocked, "invalid generation");
    std::int64_t next = 0;
    t.Expect(NextGeneration(0, next) && next == 1, "initial generation");
    t.Expect(NextGeneration(42, next) && next == 43, "advance generation");
    t.Expect(!NextGeneration(-1, next), "negative rejected");
    t.Expect(!NextGeneration(std::numeric_limits<std::int64_t>::max(), next), "overflow rejected");
    OperationGate gate;
    t.Expect(gate.BeginMutation(), "mutation begins");
    t.Expect(!gate.BeginMutation() && !gate.BeginIO(), "no reentrant mutation or save");
    gate.RequestSave(); gate.RequestSave();
    t.Expect(!gate.TakeSaveRequest(), "save waits during repair");
    gate.EndMutation();
    t.Expect(gate.TakeSaveRequest() && !gate.TakeSaveRequest(), "coalesced save after outer mutation");
    t.Expect(gate.BeginIO() && !gate.BeginMutation(), "restore blocks world actions");
    gate.EndIO(); gate.RequestSave(); gate.ClearPending();
    t.Expect(!gate.TakeSaveRequest(), "load clears old autosave");
    gate.Poison(); gate.RequestSave();
    t.Expect(gate.Poisoned() && !gate.BeginMutation() && !gate.BeginIO() && !gate.TakeSaveRequest(), "rollback fatal lock");
    for (unsigned bits = 0; bits < 64; ++bits)
    {
        Facts f{bool(bits&1u),bool(bits&2u),bool(bits&4u),bool(bits&8u),bool(bits&16u),bool(bits&32u)};
        t.Expect(CoherentMission(f, f.radioRepaired, f.messageHeard, f.messageHeard, f.maintenanceNoteRead) == IsValid(f),
            "all 64 objective combinations with matching external state");
        if (IsValid(f))
        {
            t.Expect(!CoherentMission(f, !f.radioRepaired, f.messageHeard, f.messageHeard, f.maintenanceNoteRead), "ledger mismatch");
            t.Expect(!CoherentMission(f, f.radioRepaired, !f.messageHeard, f.messageHeard, f.maintenanceNoteRead), "transcript mismatch");
            t.Expect(!CoherentMission(f, f.radioRepaired, f.messageHeard, !f.messageHeard, f.maintenanceNoteRead), "lead mismatch");
            t.Expect(!CoherentMission(f, f.radioRepaired, f.messageHeard, f.messageHeard, !f.maintenanceNoteRead), "note mismatch");
        }
    }
    std::vector<Requirement> requirements{{"item.radio_battery",1},{"item.marine_fuse",1}};
    std::string fingerprint, reversed, invalid;
    t.Expect(RequirementsFingerprint(requirements, fingerprint), "canonical repair");
    std::reverse(requirements.begin(), requirements.end());
    t.Expect(RequirementsFingerprint(requirements, reversed) && reversed == fingerprint, "order independent");
    t.Expect(fingerprint == "requirements.v1|16:item.marine_fuse:1|18:item.radio_battery:1|", "stable protocol fingerprint");
    t.Expect(!RequirementsFingerprint({}, invalid), "empty requirement rejects");
    t.Expect(!RequirementsFingerprint({{"item.a",0}}, invalid), "zero quantity rejects");
    t.Expect(!RequirementsFingerprint({{"item.a",-1}}, invalid), "negative quantity rejects");
    t.Expect(!RequirementsFingerprint({{"item.a",1},{"item.a",2}}, invalid), "duplicate item rejects");
    t.Expect(!RequirementsFingerprint({{"item.a|item.b",1}}, invalid), "delimiter injection rejects");
    t.Expect(!RequirementsFingerprint({{"ITEM.A",1}}, invalid), "noncanonical case rejects");
    t.Expect(!RequirementsFingerprint({{"",1}}, invalid) && invalid.empty(), "invalid output cleared");
    t.Expect(!ValidLogicalId("../bad") && !ValidLogicalId("hello world"), "unsafe ID rejects");
    t.Expect(ValidLogicalId("world.test.battery") && ValidLogicalId("test_01"), "authored IDs accept");
    t.Expect(!ValidLogicalId("..") && !ValidLogicalId(".world") && !ValidLogicalId("world."), "empty ID segments reject");
    return t.Finish("Campaign/transaction core");
}
