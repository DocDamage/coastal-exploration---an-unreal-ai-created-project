#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Brushes/SlateColorBrush.h"

// Values are linear colors. Styles are copied into the owning UMG widget.
namespace CoastalUITheme
{
inline FLinearColor Ink() { return FLinearColor(0.018f, 0.033f, 0.039f, 1.0f); }
inline FLinearColor Paper() { return FLinearColor(0.89f, 0.85f, 0.73f, 1.0f); }
inline FLinearColor Muted() { return FLinearColor(0.56f, 0.65f, 0.63f, 1.0f); }
inline FLinearColor Accent() { return FLinearColor(0.79f, 0.57f, 0.27f, 1.0f); }
inline FLinearColor Focus() { return FLinearColor(0.07f, 0.125f, 0.14f, 1.0f); }
inline FLinearColor Button() { return FLinearColor(0.025f, 0.055f, 0.065f, 1.0f); }
inline FButtonStyle CommandStyle()
{
    FButtonStyle Style;
    Style.SetNormal(FSlateColorBrush(FLinearColor::White));
    Style.SetHovered(FSlateColorBrush(FLinearColor(1.2f, 1.2f, 1.2f)));
    Style.SetPressed(FSlateColorBrush(FLinearColor(0.7f, 0.7f, 0.7f)));
    Style.SetDisabled(FSlateColorBrush(FLinearColor(0.35f, 0.35f, 0.35f)));
    Style.SetNormalPadding(FMargin(18, 12));
    Style.SetPressedPadding(FMargin(18, 13, 18, 11));
    return Style;
}
}
