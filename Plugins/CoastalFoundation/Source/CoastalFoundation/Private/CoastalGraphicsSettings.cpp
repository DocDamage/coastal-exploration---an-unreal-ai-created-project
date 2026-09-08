#include "CoastalDisplaySettings.h"
#include "GameFramework/GameUserSettings.h"

namespace
{
const TCHAR* QualityName(int32 Preset)
{
    switch (Preset)
    {
    case 0: return TEXT("Low"); case 1: return TEXT("Medium");
    case 2: return TEXT("High"); case 3: return TEXT("Epic");
    default: return TEXT("Custom (preserved)");
    }
}
FString CapName(float Cap)
{ return Cap == 0 ? TEXT("Unlimited") : FString::Printf(TEXT("%g FPS"), Cap); }
}

void UCoastalDisplaySettings::ReadGraphics()
{
    bGraphicsDraft = false;
    if (!Ready() || Model.Busy()) return;
    GraphicsBefore = Settings->ScalabilityQuality;
    GraphicsQualityDraft = GraphicsBefore;
    GraphicsPreset = Settings->GetOverallScalabilityLevel();
    // Cinematic and mixed settings are preserved until a preset is chosen.
    if (GraphicsPreset < 0 || GraphicsPreset > 3) GraphicsPreset = -1;
    GraphicsCap = GraphicsBeforeCap = Settings->GetFrameRateLimit();
    bGraphicsVSync = bGraphicsBeforeVSync = Settings->IsVSyncEnabled();
    bGraphicsDraft = FMath::IsFinite(GraphicsCap) && GraphicsCap >= 0;
}
bool UCoastalDisplaySettings::GraphicsReady() const
{
    return bGraphicsDraft && Ready() && !Model.Busy()
        && Settings->ScalabilityQuality == GraphicsBefore
        && Settings->GetFrameRateLimit() == GraphicsBeforeCap
        && Settings->IsVSyncEnabled() == bGraphicsBeforeVSync;
}
bool UCoastalDisplaySettings::GraphicsChanged() const
{
    if (!GraphicsReady()) return false;
    return GraphicsQualityDraft != GraphicsBefore
        || GraphicsCap != GraphicsBeforeCap || bGraphicsVSync != bGraphicsBeforeVSync;
}
void UCoastalDisplaySettings::CycleGraphicsPreset()
{
    if (!GraphicsReady()) return;
    GraphicsPreset = (GraphicsPreset + 1) % 4;
    GraphicsQualityDraft.SetFromSingleQualityLevel(GraphicsPreset);
}
void UCoastalDisplaySettings::ToggleGraphicsVSync()
{ if (GraphicsReady()) bGraphicsVSync = !bGraphicsVSync; }
void UCoastalDisplaySettings::CycleGraphicsCap()
{
    if (!GraphicsReady()) return;
    const float Caps[] = {30, 60, 90, 120, 144, 165, 240, 0};
    for (int32 I = 0; I < UE_ARRAY_COUNT(Caps); ++I)
        if (GraphicsCap == Caps[I]) { GraphicsCap = Caps[(I + 1) % UE_ARRAY_COUNT(Caps)]; return; }
    GraphicsCap = 60; // An existing custom limit is retained until this action.
}
bool UCoastalDisplaySettings::ApplyGraphics(bool bSave)
{
    if (!GraphicsReady() || (!GraphicsChanged() && !bSave)) return false;
    {
        TGuardValue<bool> Guard(bWriting, true);
        Settings->ScalabilityQuality = GraphicsQualityDraft;
        Settings->SetVSyncEnabled(bGraphicsVSync);
        Settings->SetFrameRateLimit(GraphicsCap);
        // Uses the engine's non-window settings path. It also reapplies existing
        // dynamic-resolution, audio-quality and HDR preferences; never ApplySettings.
        Settings->ApplyNonResolutionSettings();
        if (bSave) Settings->SaveSettings();
    }
    ReadGraphics();
    LastDetail = bSave
        ? TEXT("Graphics applied; settings save requested. Disk persistence has not been verified.")
        : TEXT("Graphics applied for this session. Choose Apply and save to keep them for future launches.");
    return true;
}
FString UCoastalDisplaySettings::DescribeGraphics() const
{
    if (!bGraphicsDraft) return TEXT("Graphics controls are unavailable in this session.");
    FString Text = FString::Printf(TEXT("Graphics draft: %s | VSync %s | %s"), QualityName(GraphicsPreset),
        bGraphicsVSync ? TEXT("On") : TEXT("Off"), *CapName(GraphicsCap));
    Text += GraphicsReady() ? TEXT("\nChoose Apply when ready. Back discards unapplied changes.")
        : TEXT("\nSettings changed outside this menu or a display trial is active. Refresh before editing graphics.");
    return Text;
}
