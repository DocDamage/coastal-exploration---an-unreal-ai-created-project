#include "Core/FirstSignalRules.h"
#include <array>
#include <iostream>
#include <string>

namespace {
int checks = 0;
int failures = 0;
void Expect(bool value, const std::string& name) {
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL: " << name << '\n'; }
}
bool Equal(const coastal::Facts& a, const coastal::Facts& b) {
    return a.cabinVisited==b.cabinVisited && a.radioInspected==b.radioInspected
        && a.dockDiscovered==b.dockDiscovered && a.maintenanceNoteRead==b.maintenanceNoteRead
        && a.radioRepaired==b.radioRepaired && a.messageHeard==b.messageHeard;
}
}
int main() {
    using namespace coastal;
    Facts f;
    Expect(IsValid(f), "initial snapshot valid");
    Expect(CurrentPhase(f,false)==Phase::InspectRadio,"initial objective");
    Expect(CurrentPhase(f,true)==Phase::InspectRadio,"equipment-first is permitted");
    Expect(FinishTransmission(f)==ListenResult::NotRepaired,"cannot listen before repair");
    f.radioInspected=true;
    Expect(CurrentPhase(f,false)==Phase::FindEquipment,"find after inspection");
    Expect(CurrentPhase(f,true)==Phase::ReturnToRadio,"return when carrying equipment");
    for (auto r : {CommitResult::MissingItems,CommitResult::NotConfigured,CommitResult::Failed}) {
        const Facts before=f;
        Expect(!ApplyRepairCommit(f,r),"failed commit cannot change quest");
        Expect(Equal(f,before),"failed commit preserves every fact");
    }
    Expect(ApplyRepairCommit(f,CommitResult::Committed),"successful repair transitions");
    Expect(CurrentPhase(f,false)==Phase::ListenToRadio,"post-repair objective");
    Expect(!ApplyRepairCommit(f,CommitResult::Committed),"repeat commit is idempotent");
    Expect(FinishTransmission(f)==ListenResult::Applied,"finish message");
    Expect(FinishTransmission(f)==ListenResult::AlreadyApplied,"repeat message is idempotent");
    Expect(CurrentPhase(f,false)==Phase::Complete,"complete without equipment");

    Facts recovered;
    Expect(ApplyRepairCommit(recovered,CommitResult::AlreadyCommitted),"recover transaction marker");
    Expect(recovered.radioInspected && recovered.radioRepaired,"recovery implies inspection");
    Expect(IsValid(recovered),"recovery is internally valid");
    Facts impossible;
    impossible.messageHeard=true;
    Expect(!IsValid(impossible),"message before repair invalid");
    impossible.messageHeard=false; impossible.radioRepaired=true;
    Expect(!IsValid(impossible),"repair without inspection invalid snapshot");

    // Exhaustive six-flag input space, not an Unreal or vendor integration test.
    for (int mask=0; mask<64; ++mask) {
        Facts state{bool(mask&1),bool(mask&2),bool(mask&4),bool(mask&8),bool(mask&16),bool(mask&32)};
        const bool expected=(!(mask&16)||(mask&2)) && (!(mask&32)||(mask&16));
        Expect(IsValid(state)==expected,"snapshot invariants mask "+std::to_string(mask));
        if (!IsValid(state)) continue;
        for (auto r : {CommitResult::MissingItems,CommitResult::NotConfigured,CommitResult::Failed}) {
            auto candidate=state;
            ApplyRepairCommit(candidate,r);
            Expect(Equal(candidate,state),"failure nonmutation exhaustive");
        }
        for (auto r : {CommitResult::Committed,CommitResult::AlreadyCommitted}) {
            auto candidate=state;
            ApplyRepairCommit(candidate,r);
            Expect(IsValid(candidate),"successful repair invariant exhaustive");
            Expect(candidate.radioRepaired,"success repairs exhaustive");
            const auto once=candidate;
            Expect(!ApplyRepairCommit(candidate,r) && Equal(candidate,once),"repeat repair stable exhaustive");
        }
        auto candidate=state;
        FinishTransmission(candidate);
        Expect(IsValid(candidate),"listen preserves invariant exhaustive");
    }
    std::cout << checks << " checks; " << failures << " failures\n";
    return failures ? 1 : 0;
}
