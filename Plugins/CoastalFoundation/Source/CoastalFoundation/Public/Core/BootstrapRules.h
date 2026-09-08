#pragma once
#include <cstdint>
namespace coastal
{
// No provider or engine simulation. Used by the real bootstrap component.
enum class StartupPhase : std::uint8_t { Idle, Checking, Blocked, Binding, Ready, RestartRequired, Stopped };
enum class StartupRequest : std::uint8_t { Proceed, Busy, AlreadyReady, RestartRequired, Stopped };
class StartupGate
{
public:
    StartupPhase Phase() const { return phase_; }
    StartupRequest Begin()
    {
        switch (phase_)
        {
        case StartupPhase::Idle: case StartupPhase::Blocked:
            phase_ = StartupPhase::Checking; return StartupRequest::Proceed;
        case StartupPhase::Checking: case StartupPhase::Binding: return StartupRequest::Busy;
        case StartupPhase::Ready: return StartupRequest::AlreadyReady;
        case StartupPhase::RestartRequired: return StartupRequest::RestartRequired;
        default: return StartupRequest::Stopped;
        }
    }
    bool FinishChecks(bool passed)
    {
        if (phase_ != StartupPhase::Checking) return false;
        phase_ = passed ? StartupPhase::Binding : StartupPhase::Blocked; return true;
    }
    bool FinishBinding(bool passed)
    {
        if (phase_ != StartupPhase::Binding) return false;
        phase_ = passed ? StartupPhase::Ready : StartupPhase::RestartRequired; return true;
    }
    // Used only when lifetime/integration integrity has been lost, not a retryable read failure.
    void RequireRestart() { if (phase_ != StartupPhase::Stopped) phase_ = StartupPhase::RestartRequired; }
    void Stop() { phase_ = StartupPhase::Stopped; }
private:
    StartupPhase phase_ = StartupPhase::Idle;
};
}
