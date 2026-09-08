#include "Core/UIFlowRules.h"
#include "m1_test_support.h"
#include <array>
int main()
{
    using namespace coastal;
    TestRun t;
    UIFlow flow; flow.Reset(7);
    t.Expect(flow.Depth() == 0 && !flow.Top(), "empty flow");
    const auto root = flow.Push(PanelKind::Session, 10);
    t.Expect(root.id > 0 && root.epoch == 7 && flow.IsTop(root), "root ticket");
    t.Expect(!flow.CanCommand(root, 10), "opening input cannot execute button");
    t.Expect(!flow.Pop(root, false), "no campaign cannot dismiss session root");
    t.Expect(flow.ClaimCommand(root, 11), "next frame command");
    t.Expect(!flow.ClaimCommand(root, 11), "one command per frame");
    flow.RememberFocus(root, 3);
    const auto journal = flow.Push(PanelKind::Journal, 11);
    t.Expect(flow.Depth() == 2 && flow.IsTop(journal), "child opens");
    t.Expect(!flow.ClaimCommand(root, 12), "hidden parent rejected");
    t.Expect(!flow.Push(PanelKind::Journal, 12).id, "duplicate screen rejected");
    t.Expect(!flow.Pop(root, true), "out of order removal rejected");
    t.Expect(flow.ClaimCommand(journal, 12) && flow.Pop(journal, false), "back removes child even pre-campaign");
    t.Expect(flow.IsTop(root) && flow.Top()->focus == 3, "parent focus restored");
    t.Expect(!flow.ClaimCommand(root, 12), "same event cannot pop newly exposed parent");
    t.Expect(flow.ClaimCommand(root, 13) && flow.Pop(root, true), "later press closes one remaining layer");
    t.Expect(flow.Depth() == 0 && !flow.IsTop(root), "dismissed ticket invalid");
    auto old = flow.Push(PanelKind::Transcript, 14);
    flow.Reset(8);
    auto fresh = flow.Push(PanelKind::Transcript, 14);
    t.Expect(fresh.id > old.id && fresh.epoch == 8, "ticket monotonic across sessions");
    t.Expect(!flow.ClaimCommand(old, 15), "stale session callback rejected");
    auto forged = fresh; forged.kind = PanelKind::Storage;
    t.Expect(!flow.IsTop(forged), "forged screen kind rejected");
    forged = fresh; forged.epoch = 7;
    t.Expect(!flow.IsTop(forged), "stale epoch rejected");
    t.Expect(flow.ClaimCommand(fresh, 15), "fresh transcript command");
    flow.Reset(9); auto sameFrame = flow.Push(PanelKind::Session, 14);
    t.Expect(!flow.ClaimCommand(sameFrame, 15), "reset cannot clear debounce");
    t.Expect(flow.ClaimCommand(sameFrame, 16), "new epoch later frame allowed");
    flow.RememberFocus(sameFrame, -30);
    t.Expect(flow.Top()->focus == 0, "negative focus clamped");
    flow.Reset(10);
    const std::array<PanelKind, 9> kinds{PanelKind::Session,PanelKind::Pause,PanelKind::Inventory,
        PanelKind::Storage,PanelKind::Journal,PanelKind::Transcript,PanelKind::ItemDetails,
        PanelKind::ConfirmExit,PanelKind::ConfirmSession};
    for (std::size_t i = 0; i < kinds.size(); ++i)
        t.Expect((flow.Push(kinds[i], 20).id > 0) == (i < UIFlow::MaxDepth), "bounded stack");
    const auto recovery = flow.RequireRecovery(21);
    t.Expect(flow.Depth() == 1 && flow.RecoveryRequired() && flow.IsTop(recovery), "recovery replaces all layers");
    t.Expect(!flow.Pop(recovery, true), "fatal recovery cannot be dismissed");
    t.Expect(!flow.Push(PanelKind::Inventory, 22).id, "fatal recovery blocks new menus");
    flow.Reset(11);
    t.Expect(flow.RecoveryRequired() && !flow.Push(PanelKind::Session, 23).id, "reset cannot unpoison recovery");
    t.Expect(flow.RequireRecovery(24).id != 0, "recovery can be represented after reset");
    PresentationGate shown;
    t.Expect(!shown.Ready(999), "unpainted text never acknowledges");
    shown.MarkPainted(1000);
    t.Expect(!shown.Ready(1000) && shown.Ready(1001), "must paint before a later explicit action");
    shown.MarkPainted(1002);
    t.Expect(shown.Ready(1002), "subsequent paint does not postpone readiness forever");
    t.Expect(CycleSelection(-1, 1, 0) == -1, "empty list");
    t.Expect(CycleSelection(-1, -1, 3) == 0, "initial selection");
    t.Expect(CycleSelection(2, 1, 3) == 0 && CycleSelection(0, -1, 3) == 2, "wrapping navigation");
    t.Expect(CycleSelection(1, 0, 3) == 1, "stable selection");
    for (int count = 1; count <= 12; ++count)
        for (int current = -1; current <= count; ++current)
            for (int direction : {-1, 0, 1})
            {
                const int index = CycleSelection(current, direction, count);
                t.Expect(index >= 0 && index < count, "navigation bounds");
            }
    // Deterministic lifecycle stress: same helper as runtime, no simulated Unreal UI.
    UIFlow loops;
    for (std::uint64_t epoch = 1; epoch <= 100; ++epoch)
    {
        const auto frame = epoch * 10;
        loops.Reset(epoch); const auto a = loops.Push(PanelKind::Pause, frame);
        loops.RememberFocus(a, 2); const auto b = loops.Push(PanelKind::Journal, frame);
        t.Expect(loops.ClaimCommand(b, frame + 1) && loops.Pop(b, true), "cycle child back");
        t.Expect(!loops.ClaimCommand(a, frame + 1) && loops.Top()->focus == 2, "cycle focus and double-back protection");
        t.Expect(loops.ClaimCommand(a, frame + 2) && loops.Pop(a, true) && loops.Depth() == 0, "cycle releases all layers");
    }
    return t.Finish("UI flow/presentation core");
}
