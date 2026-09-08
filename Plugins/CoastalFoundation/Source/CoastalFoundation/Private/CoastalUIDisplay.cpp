#include "CoastalUISessionComponent.h"
#include "CoastalDisplaySettings.h"
#include "CoastalPanelWidget.h"
#include "CoastalSaveCoordinator.h"
#include "CoreGlobals.h"

void UCoastalUISessionComponent::InitializeDisplay()
{
    DisplaySettings = NewObject<UCoastalDisplaySettings>(this);
    if (DisplaySettings) DisplaySettings->Initialize(Controller, bEnableDisplaySettings);
}
void UCoastalUISessionComponent::CancelDisplay()
{ if (IsValid(DisplaySettings)) DisplaySettings->Cancel(); }
void UCoastalUISessionComponent::TickDisplay()
{
    if (!IsValid(DisplaySettings)) return;
    auto* Panel = !Panels.IsEmpty() ? Panels.Last().Get() : nullptr;
    const auto Top = Flow.Top() ? Flow.Top()->ticket : coastal::PanelTicket{};
    const bool Presented = IsValid(Panel) && Panel->IsInViewport() && Panel->IsVisible() && Panel->HasBeenPresented();
    const bool Healthy = IsValid(Saves) && IsValid(Panel) && Panel->IsInViewport() && Panel->IsVisible() && !bSessionReset && !bRecoveryPending && !Saves->IsRecoveryRequired()
        && Epoch == Saves->GetSessionEpoch() && !Saves->IsPlayerReturnActive();
    const auto Before = DisplaySettings->Trial().Phase();
    DisplaySettings->Tick(Top, Presented, Healthy);
    const auto& Trial = DisplaySettings->Trial();
    // A paint before a resolution change is not presentation in the changed resolution.
    if (Trial.PresentationRevision() != DisplayPaintRevision)
    {
        DisplayPaintRevision = Trial.PresentationRevision();
        if (IsValid(Panel) && Top.kind == coastal::PanelKind::ConfirmDisplay)
        { Panel->ResetPresentation(); Panel->ScrollToTop(); }
    }
    const bool Keep = IsValid(Panel) && DisplaySettings->CanKeep(Top, Panel->HasBeenPresented());
    if (Keep != bDisplayKeepAvailable || Before != Trial.Phase())
    { bDisplayKeepAvailable = Keep; bRefreshPending = true; }
    if (Top.kind == coastal::PanelKind::ConfirmDisplay && Trial.Phase() != coastal::DisplayPhase::Testing)
        CloseTop(); // Timeout/loss/revert never claims a player Keep command.
}
void UCoastalUISessionComponent::SelectDisplayMode(int32 ModeStep, int32 ResolutionStep)
{
    if (!IsValid(DisplaySettings) || !DisplaySettings->Ready() || DisplaySettings->Trial().Busy()) return;
    const auto& All = DisplaySettings->Modes();
    if (All.empty()) return;
    std::vector<coastal::DisplayWindowMode> Types;
    for (auto Mode : All)
        if (std::find(Types.begin(), Types.end(), Mode.window) == Types.end()) Types.push_back(Mode.window);
    if (ModeStep)
    {
        auto Found = std::find(Types.begin(), Types.end(), DisplayDraft.window);
        const int Index = Found == Types.end() ? -1 : static_cast<int>(Found - Types.begin());
        const auto Type = Types[coastal::CycleSelection(Index, ModeStep, static_cast<int>(Types.size()))];
        auto Match = std::find_if(All.begin(), All.end(), [&](auto M) { return M.window == Type
            && M.width == DisplayDraft.width && M.height == DisplayDraft.height; });
        if (Match == All.end()) Match = std::find_if(All.begin(), All.end(), [Type](auto M) { return M.window == Type; });
        DisplayDraft = *Match;
    }
    else
    {
        std::vector<coastal::DisplayMode> Sizes;
        for (auto Mode : All) if (Mode.window == DisplayDraft.window) Sizes.push_back(Mode);
        if (Sizes.empty()) { DisplayDraft = All.front(); bRefreshPending = true; return; }
        auto Found = std::find(Sizes.begin(), Sizes.end(), DisplayDraft);
        const int Index = Found == Sizes.end() ? -1 : static_cast<int>(Found - Sizes.begin());
        DisplayDraft = Sizes[coastal::CycleSelection(Index, ResolutionStep, static_cast<int>(Sizes.size()))];
    }
    bRefreshPending = true;
}
void UCoastalUISessionComponent::DisplayCommand(FName Id)
{
    using K = coastal::PanelKind;
    if (!IsValid(DisplaySettings)) return;
    if (Id == TEXT("display_options"))
    {
        DisplaySettings->RefreshCatalogue(); DisplayDraft = DisplaySettings->Current(); DisplaySettings->ReadGraphics();
        Push(K::Display); return;
    }
    if (Id == TEXT("display_refresh"))
    { DisplaySettings->RefreshCatalogue(); DisplayDraft = DisplaySettings->Current(); DisplaySettings->ReadGraphics(); bRefreshPending = true; return; }
    if (Id == TEXT("graphics_preset")) { DisplaySettings->CycleGraphicsPreset(); bRefreshPending = true; return; }
    if (Id == TEXT("graphics_vsync")) { DisplaySettings->ToggleGraphicsVSync(); bRefreshPending = true; return; }
    if (Id == TEXT("graphics_cap")) { DisplaySettings->CycleGraphicsCap(); bRefreshPending = true; return; }
    if (Id == TEXT("graphics_apply") || Id == TEXT("graphics_save"))
    { DisplaySettings->ApplyGraphics(Id == TEXT("graphics_save")); bRefreshPending = true; return; }
    if (Id == TEXT("display_mode")) { SelectDisplayMode(1, 0); return; }
    if (Id == TEXT("display_previous")) { SelectDisplayMode(0, -1); return; }
    if (Id == TEXT("display_next")) { SelectDisplayMode(0, 1); return; }
    if (Id == TEXT("display_test"))
    {
        if (!DisplaySettings->Ready() || DisplaySettings->Trial().Busy() || !Push(K::ConfirmDisplay)) return;
        if (!DisplaySettings->Begin(DisplayDraft, Panels.Last()->Ticket)) { CloseTop(); return; }
        Panels.Last()->ResetPresentation(); bDisplayKeepAvailable = false; bRefreshPending = true; return;
    }
    if (Id == TEXT("display_revert")) { CancelDisplay(); CloseTop(); return; }
    if (Id == TEXT("display_keep") || Id == TEXT("display_save"))
    {
        if (Panels.IsEmpty()) return;
        auto* Panel = Panels.Last().Get();
        if (DisplaySettings->Keep(Panel->Ticket, Panel->IsInViewport() && Panel->IsVisible() && Panel->HasBeenPresented(), Id == TEXT("display_save")))
            CloseTop();
    }
}
