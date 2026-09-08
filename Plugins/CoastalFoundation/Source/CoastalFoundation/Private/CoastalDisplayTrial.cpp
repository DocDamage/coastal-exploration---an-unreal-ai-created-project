#include "CoastalDisplaySettings.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/PlatformTime.h"
#include "UnrealEngine.h"
#include "CoreGlobals.h"

namespace
{
    EWindowMode::Type ToNative(coastal::DisplayWindowMode Mode)
    {
        switch (Mode)
        {
        case coastal::DisplayWindowMode::Fullscreen: return EWindowMode::Fullscreen;
        case coastal::DisplayWindowMode::Borderless: return EWindowMode::WindowedFullscreen;
        default: return EWindowMode::Windowed;
        }
    }
}
void UCoastalDisplaySettings::Dispatch(coastal::DisplayCommand Command)
{
    if (Command == coastal::DisplayCommand::None) return;
    if (!Binding())
    { Model.Fail(); LastDetail = TEXT("Display owner/viewport lost. Restoration was NOT confirmed. Relaunch; no trial preference was saved."); return; }
    const auto Target = Command == coastal::DisplayCommand::RequestCandidate ? Model.Proposed() : Model.Before();
    // Deferred engine request only. Do not stage config fields or call ApplySettings (which saves).
    FSystemResolution::RequestResolutionChange(Target.width, Target.height, ToNative(Target.window));
    LastDetail = Command == coastal::DisplayCommand::RequestCandidate ? TEXT("Display trial requested; not yet confirmed.")
        : TEXT("Previous viewport requested. Waiting to OBSERVE restoration; no settings save requested.");
}
void UCoastalDisplaySettings::Tick(coastal::PanelTicket Top, bool bPresented, bool bSessionHealthy)
{
    if (bStopped || bWriting || !Model.Busy()) return;
    auto O = Observe(Top, bPresented);
    if (Model.Phase() == coastal::DisplayPhase::Testing && (!SettingsUnchanged() || !bSessionHealthy))
    { bFaultAfterRestore |= !SettingsUnchanged(); O.ownerReady = false; }
    const auto Previous = Model.Phase(); Dispatch(Model.Step(O));
    if (Previous == coastal::DisplayPhase::Reverting && Model.Phase() == coastal::DisplayPhase::Idle)
    {
        LastDetail = TEXT("Previous viewport observed restored. No display save was requested.");
        if (bFaultAfterRestore)
        { Model.Fail(); LastDetail += TEXT(" A competing settings edit was observed; further trials are disabled until relaunch."); }
    }
    if (Model.Phase() == coastal::DisplayPhase::Failed)
        LastDetail = TEXT("Display restoration/ownership could not be verified. Trials disabled until relaunch. No trial save requested.");
}
bool UCoastalDisplaySettings::CanKeep(coastal::PanelTicket Top, bool bPresented) const
{ return Ready() && SettingsUnchanged() && Model.CanKeep(Observe(Top, bPresented)); }
bool UCoastalDisplaySettings::Keep(coastal::PanelTicket Top, bool bPresented, bool bRequestSave)
{
    if (!Ready()) return false;
    Tick(Top, bPresented, true); // Recheck the real viewport, identity and real-time deadline on the command path.
    if (!SettingsUnchanged() || !Model.Keep(Observe(Top, bPresented))) return false;
    LastDetail = TEXT("Display kept for this session only. Engine saved display settings were not changed.");
    if (bRequestSave)
    {
        // SaveSettings is void and serializes engine user settings, not a verified two-field transaction.
        // No preference/campaign save success is inferred from it.
        TGuardValue<bool> WritingGuard(bWriting, true);
        const auto Mode = Model.Proposed();
        Settings->SetScreenResolution(FIntPoint(Mode.width, Mode.height));
        Settings->SetFullscreenMode(ToNative(Mode.window));
        Settings->ConfirmVideoMode(); Settings->SaveSettings();
        LastDetail = TEXT("Display kept. Engine settings save REQUESTED; disk persistence is not verified. Check the mode after relaunch.");
    }
    return true;
}
void UCoastalDisplaySettings::Cancel()
{
    if (bStopped || bWriting) return;
    Dispatch(Model.Cancel(FPlatformTime::Seconds(), GFrameCounter));
}
void UCoastalDisplaySettings::Release()
{
    if (bStopped || bWriting) return;
    Cancel();
    if (Model.Busy()) LastDetail = TEXT("Display owner released after a best-effort restore request. Restoration was not observed before shutdown.");
    bStopped = true; bInitialized = false; Controller.Reset(); ViewportClient.Reset(); Settings.Reset(); BoundViewport = nullptr;
}
void UCoastalDisplaySettings::BeginDestroy()
{ Release(); Super::BeginDestroy(); }
