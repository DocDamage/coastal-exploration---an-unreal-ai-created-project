#include "CoastalUISessionComponent.h"
#include "CoastalDisplaySettings.h"
#include "CoastalPanelWidget.h"
#include "HAL/PlatformTime.h"

FText UCoastalUISessionComponent::PanelStatus(coastal::PanelKind Kind) const
{
    return Kind == coastal::PanelKind::ConfirmDisplay && IsValid(DisplaySettings)
        ? FText::FromString(DisplaySettings->Status()) : Status();
}
FText UCoastalUISessionComponent::DisplayConfirmationTitle() const
{
    const double Left = IsValid(DisplaySettings) ? DisplaySettings->Trial().SecondsLeft(FPlatformTime::Seconds()) : 0;
    return FText::FromString(FString::Printf(TEXT("KEEP DISPLAY?  %.0f seconds"), std::ceil(Left)));
}

void UCoastalUISessionComponent::PresentDisplay(coastal::PanelKind Kind, FText& Title, FText& Body,
    TArray<FCoastalUIChoice>& Choices)
{
    const auto Add = [&Choices](const TCHAR* Id, const TCHAR* Label, bool Enabled = true)
    { Choices.Add({FName(Id), FText::FromString(Label), Enabled}); };
    if (Kind == coastal::PanelKind::ConfirmDisplay)
    {
        Title = DisplayConfirmationTitle();
        FString Text = TEXT("Revert is the default. Escape / B also reverts.\n\n");
        if (IsValid(DisplaySettings)) Text += TEXT("Testing: ") + UCoastalDisplaySettings::Describe(DisplaySettings->Trial().Proposed())
            + TEXT("\nReturn to: ") + UCoastalDisplaySettings::Describe(DisplaySettings->Trial().Before());
        Text += TEXT("\n\nSession-only Keep does not save. Engine save requests are separate from campaigns and are not disk-verified.");
        const auto* Panel = Panels.IsEmpty() ? nullptr : Panels.Last().Get();
        const bool Keep = IsValid(DisplaySettings) && IsValid(Panel) && Panel->IsInViewport() && Panel->IsVisible()
            && DisplaySettings->CanKeep(Panel->Ticket, Panel->HasBeenPresented());
        Add(TEXT("display_revert"), TEXT("Revert to previous display"));
        Add(TEXT("display_keep"), TEXT("Keep for this session only"), Keep);
        Add(TEXT("display_save"), TEXT("Keep and request engine settings save"), Keep);
        Body = FText::FromString(Text); return;
    }
    Title = FText::FromString(TEXT("DISPLAY & GRAPHICS"));
    FString Text = TEXT("Choose a resolution and window mode, or adjust graphics quality and frame pacing.\n\n");
    const bool Ready = IsValid(DisplaySettings) && DisplaySettings->Ready() && !DisplaySettings->Trial().Busy();
    if (IsValid(DisplaySettings))
        Text += TEXT("Current viewport: ") + UCoastalDisplaySettings::Describe(DisplaySettings->Current())
            + TEXT("\nDraft: ") + UCoastalDisplaySettings::Describe(DisplayDraft)
            + TEXT("\n\n") + DisplaySettings->LastDetail;
    else Text += TEXT("Display owner unavailable. Nothing changed.");
    Text += TEXT("\n\nChanging the draft does nothing to the window. Test requires an explicit Keep before its deadline. Back, lost focus or a session change cancels the trial. Borderless uses the engine-reported desktop size; monitor selection and window position are outside this feature.");
    bool Candidate = false;
    if (Ready)
    {
        const auto& Modes = DisplaySettings->Modes();
        Candidate = DisplayDraft != DisplaySettings->Current() && std::find(Modes.begin(), Modes.end(), DisplayDraft) != Modes.end();
    }
    const bool HasModes = Ready && !DisplaySettings->Modes().empty();
    const bool GraphicsReady = IsValid(DisplaySettings) && DisplaySettings->GraphicsReady();
    if (IsValid(DisplaySettings)) Text += TEXT("\n\n") + DisplaySettings->DescribeGraphics();
    Text += TEXT("\nQuality presets also adjust render scale. Lower settings may improve performance. VSync and driver settings can further limit frame rate. Graphics saves use the engine settings file, separate from your campaign.");
    Add(TEXT("display_mode"), TEXT("Next available window mode"), HasModes);
    Add(TEXT("display_previous"), TEXT("Previous resolution"), HasModes);
    Add(TEXT("display_next"), TEXT("Next resolution"), HasModes);
    Add(TEXT("display_test"), TEXT("Test draft for 15 seconds"), Ready && Candidate);
    Add(TEXT("graphics_preset"), TEXT("Next quality preset: Low / Medium / High / Epic"), GraphicsReady);
    Add(TEXT("graphics_vsync"), TEXT("Toggle VSync draft"), GraphicsReady);
    Add(TEXT("graphics_cap"), TEXT("Next frame-rate limit"), GraphicsReady);
    Add(TEXT("graphics_apply"), TEXT("Apply graphics for this session"), GraphicsReady && DisplaySettings->GraphicsChanged());
    Add(TEXT("graphics_save"), TEXT("Apply graphics and save settings"), GraphicsReady);
    Add(TEXT("display_refresh"), TEXT("Refresh modes and discard draft"), Ready);
    Add(TEXT("back"), TEXT("Back / discard draft")); Body = FText::FromString(Text);
}
