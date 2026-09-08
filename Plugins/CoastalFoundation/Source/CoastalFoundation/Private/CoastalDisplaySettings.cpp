#include "CoastalDisplaySettings.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UnrealClient.h"
#include "CoreGlobals.h"
#include "Widgets/SWindow.h"
#include "GenericPlatform/GenericWindow.h"

namespace
{
    coastal::DisplayWindowMode FromNative(EWindowMode::Type Mode)
    {
        switch (Mode)
        {
        case EWindowMode::Fullscreen: return coastal::DisplayWindowMode::Fullscreen;
        case EWindowMode::WindowedFullscreen: return coastal::DisplayWindowMode::Borderless;
        case EWindowMode::Windowed: return coastal::DisplayWindowMode::Windowed;
        default: return static_cast<coastal::DisplayWindowMode>(-1);
        }
    }
}
bool UCoastalDisplaySettings::Binding() const
{
#if PLATFORM_WINDOWS
    auto* PC = Controller.Get(); auto* Client = ViewportClient.Get(); auto* World = IsValid(PC) ? PC->GetWorld() : nullptr;
    auto* GI = World ? World->GetGameInstance() : nullptr;
    return bInitialized && !bStopped && IsInGameThread() && !GIsEditor && GEngine && !GEngine->XRSystem.IsValid()
        && World && World->WorldType == EWorldType::Game && World->GetNetMode() == NM_Standalone
        && GI && GI->GetLocalPlayers().Num() == 1 && PC->IsLocalController() && PC->GetLocalPlayer()
        && PC->GetLocalPlayer()->ViewportClient == Client && IsValid(Client) && Client->GetWorld() == World
        && Client == GEngine->GameViewport && Client->Viewport && Client->Viewport == BoundViewport
        && !Client->Viewport->IsPlayInEditorViewport() && Settings.IsValid()
        && Settings.Get() == GEngine->GetGameUserSettings() && Settings->GetClass() == UGameUserSettings::StaticClass();
#else
    return false;
#endif
}
bool UCoastalDisplaySettings::Initialize(APlayerController* PC, bool bOptedIn)
{
    if (bInitialized || bStopped || !IsInGameThread()) return false;
    LastDetail = TEXT("Display controls unavailable: enable the single UI display owner in a Windows game (not PIE), with the standard engine settings object.");
    if (!bOptedIn || !IsValid(PC) || !GEngine || !PC->GetLocalPlayer()) return false;
    Controller = PC; ViewportClient = PC->GetLocalPlayer()->ViewportClient; Settings = GEngine->GetGameUserSettings();
    BoundViewport = ViewportClient.IsValid() ? ViewportClient->Viewport : nullptr; bInitialized = true;
    if (!Binding()) { bInitialized = false; return false; }
    return RefreshCatalogue();
}
bool UCoastalDisplaySettings::Ready() const
{ return Binding() && !bWriting && Model.Phase() != coastal::DisplayPhase::Failed; }
coastal::DisplayMode UCoastalDisplaySettings::Current() const
{
    if (!Binding()) return {};
    const FIntPoint Size = BoundViewport->GetSizeXY();
    return {Size.X, Size.Y, FromNative(BoundViewport->GetWindowMode())};
}
bool UCoastalDisplaySettings::RefreshCatalogue()
{
    if (!Ready() || Model.Busy()) return false;
    std::vector<coastal::DisplayMode> Found;
    TArray<FIntPoint> Sizes;
    if (UKismetSystemLibrary::GetConvenientWindowedResolutions(Sizes))
        for (const auto& Size : Sizes) Found.push_back({Size.X, Size.Y, coastal::DisplayWindowMode::Windowed});
    Sizes.Reset();
    const auto Window = ViewportClient->GetWindow();
    const auto NativeWindow = Window.IsValid() ? Window->GetNativeWindow() : nullptr;
    if (NativeWindow.IsValid() && NativeWindow->IsFullscreenSupported()
        && UKismetSystemLibrary::GetSupportedFullscreenResolutions(Sizes))
        for (const auto& Size : Sizes) Found.push_back({Size.X, Size.Y, coastal::DisplayWindowMode::Fullscreen});
    const FIntPoint Desktop = Settings->GetDesktopResolution();
    Found.push_back({Desktop.X, Desktop.Y, coastal::DisplayWindowMode::Borderless});
    const auto Actual = Current();
    if (Actual.window == coastal::DisplayWindowMode::Windowed) Found.push_back(Actual);
    Catalogue = coastal::DisplayCatalogue(MoveTemp(Found));
    LastDetail = Catalogue.empty() ? TEXT("No display modes were admitted from the engine query. Nothing changed.")
        : TEXT("Display modes are engine-reported candidates, not a tested monitor guarantee. Trials never save automatically.");
    return !Catalogue.empty();
}
bool UCoastalDisplaySettings::SettingsUnchanged() const
{
    if (!Binding()) return false;
    const auto Size = Settings->GetScreenResolution();
    return CapturedSettings == coastal::DisplayMode{Size.X, Size.Y, FromNative(Settings->GetFullscreenMode())};
}
coastal::DisplayObservation UCoastalDisplaySettings::Observe(coastal::PanelTicket Top, bool bPresented) const
{
    coastal::DisplayObservation O;
    O.seconds = FPlatformTime::Seconds(); O.frame = GFrameCounter; O.top = Top;
    O.ownerReady = Binding(); O.presented = bPresented;
    if (O.ownerReady) { O.actual = Current(); O.foreground = BoundViewport->IsForegroundWindow(); }
    return O;
}
bool UCoastalDisplaySettings::Begin(coastal::DisplayMode Candidate, coastal::PanelTicket Ticket)
{
    if (!Ready() || Model.Busy() || !RefreshCatalogue()) return false;
    const auto Size = Settings->GetScreenResolution();
    CapturedSettings = {Size.X, Size.Y, FromNative(Settings->GetFullscreenMode())};
    const auto Command = Model.Begin(Current(), Candidate, Catalogue, Observe(Ticket, false));
    if (Command == coastal::DisplayCommand::None) { LastDetail = TEXT("Display trial refused. Reopen and review the current mode."); return false; }
    Dispatch(Command); return Model.Phase() == coastal::DisplayPhase::Testing;
}
FString UCoastalDisplaySettings::Describe(coastal::DisplayMode Mode)
{
    const TCHAR* Name = TEXT("Unknown");
    switch (Mode.window)
    {
    case coastal::DisplayWindowMode::Windowed: Name = TEXT("Windowed"); break;
    case coastal::DisplayWindowMode::Borderless: Name = TEXT("Borderless (desktop)"); break;
    case coastal::DisplayWindowMode::Fullscreen: Name = TEXT("Fullscreen"); break;
    }
    return coastal::ValidDisplaySnapshot(Mode) ? FString::Printf(TEXT("%d x %d | %s"), Mode.width, Mode.height, Name) : TEXT("Unavailable");
}
FString UCoastalDisplaySettings::Status() const
{
    if (Model.Phase() == coastal::DisplayPhase::Testing)
        return FString::Printf(TEXT("DISPLAY TRIAL: %.0f seconds left. Revert is automatic unless you explicitly Keep. %s"),
            std::ceil(Model.SecondsLeft(FPlatformTime::Seconds())), Model.Settled() ? TEXT("Requested viewport observed.") : TEXT("Waiting for requested viewport."));
    return LastDetail;
}
