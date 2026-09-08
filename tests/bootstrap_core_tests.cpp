#include "Core/BootstrapRules.h"
#include "m1_test_support.h"
#include <array>
int main()
{
    using namespace coastal;
    TestRun t;
    StartupGate gate;
    t.Expect(gate.Phase() == StartupPhase::Idle, "idle before explicit request");
    t.Expect(!gate.FinishChecks(true) && !gate.FinishBinding(true), "out-of-order completion cannot initialize");
    t.Expect(gate.Begin() == StartupRequest::Proceed && gate.Phase() == StartupPhase::Checking, "explicit first attempt");
    t.Expect(gate.Begin() == StartupRequest::Busy, "provider callbacks cannot reenter startup");
    t.Expect(gate.FinishChecks(false) && gate.Phase() == StartupPhase::Blocked, "read-only failure retryable");
    t.Expect(!gate.FinishBinding(true), "failed checks cannot skip to ready");
    t.Expect(gate.Begin() == StartupRequest::Proceed, "explicit retry after fix");
    t.Expect(gate.FinishChecks(true) && gate.Phase() == StartupPhase::Binding, "bind only after successful checks");
    t.Expect(gate.Begin() == StartupRequest::Busy, "binding callbacks cannot reenter");
    t.Expect(!gate.FinishChecks(false) && gate.Phase() == StartupPhase::Binding, "late stale check rejected");
    t.Expect(gate.FinishBinding(false) && gate.Phase() == StartupPhase::RestartRequired, "partial configuration never retryable");
    t.Expect(gate.Begin() == StartupRequest::RestartRequired, "poisoned startup cannot restart in place");
    t.Expect(!gate.FinishBinding(true) && !gate.FinishChecks(true), "stale success cannot unpoison");
    gate.Stop(); gate.RequireRestart();
    t.Expect(gate.Phase() == StartupPhase::Stopped && gate.Begin() == StartupRequest::Stopped, "teardown terminal");
    StartupGate ready;
    ready.Begin(); ready.FinishChecks(true); ready.FinishBinding(true);
    t.Expect(ready.Phase() == StartupPhase::Ready && ready.Begin() == StartupRequest::AlreadyReady, "repeat successful request idempotent");
    t.Expect(!ready.FinishBinding(false) && !ready.FinishChecks(false), "stale completions cannot alter ready state");
    ready.RequireRestart();
    t.Expect(ready.Begin() == StartupRequest::RestartRequired, "lifetime failure can stop initialized session");
    // Independently exercise operations from every reachable phase, not a simulated engine.
    auto at = [](int phase)
    {
        StartupGate g;
        if (phase == 0) return g;
        g.Begin();
        if (phase == 1) return g;
        g.FinishChecks(phase != 2);
        if (phase == 2 || phase == 3) return g;
        g.FinishBinding(phase == 4);
        if (phase == 6) g.Stop();
        return g;
    };
    const std::array<StartupRequest,7> requests{StartupRequest::Proceed,StartupRequest::Busy,
        StartupRequest::Proceed,StartupRequest::Busy,StartupRequest::AlreadyReady,
        StartupRequest::RestartRequired,StartupRequest::Stopped};
    for (int phase=0;phase<7;++phase)
    {
        auto g=at(phase); t.Expect(g.Begin()==requests[static_cast<std::size_t>(phase)],"phase request policy");
        for (bool success : {false,true})
        {
            auto check=at(phase);const auto prior=check.Phase();
            t.Expect(check.FinishChecks(success)==(phase==1),"checks completion only in checking");
            t.Expect(phase==1 || check.Phase()==prior,"rejected checks preserve phase");
            auto bind=at(phase);const auto old=bind.Phase();
            t.Expect(bind.FinishBinding(success)==(phase==3),"binding completion only in binding");
            t.Expect(phase==3 || bind.Phase()==old,"rejected binding preserves phase");
        }
        auto stopped=at(phase);stopped.Stop();
        t.Expect(stopped.Begin()==StartupRequest::Stopped,"teardown from every phase is terminal");
        t.Expect(!stopped.FinishChecks(true)&&!stopped.FinishBinding(true),"teardown rejects pending callbacks");
    }
    return t.Finish("Startup lifecycle core");
}
