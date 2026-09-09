#include "CoastalPanelWidget.h"
#include "CoastalUISessionComponent.h"
#include "CoastalUITheme.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"

int32 UCoastalPanelWidget::FocusIndex() const
{
    if (SaveField && SaveField->IsVisible() && SaveField->HasAnyUserFocus()) return Buttons.Num();
    for (int32 I = 0; I < Buttons.Num(); ++I) if (Buttons[I]->HasAnyUserFocus()) return I;
    return 0;
}
void UCoastalPanelWidget::FocusDefault(int32 Preferred, bool bScroll)
{
    if (Preferred == Buttons.Num() && SaveField && SaveField->IsVisible())
    {
        SaveField->SetUserFocus(GetOwningPlayer());
        if (bScroll) Scroller->ScrollWidgetIntoView(SaveField, false, EDescendantScrollDestination::IntoView, 8);
        return;
    }
    if (Buttons.IsEmpty()) { SetUserFocus(GetOwningPlayer()); return; }
    const int32 Start = FMath::Clamp(Preferred, 0, Buttons.Num() - 1);
    for (int32 Offset = 0; Offset < Buttons.Num(); ++Offset)
    {
        auto* Button = Buttons[(Start + Offset) % Buttons.Num()].Get();
        if (Button->GetIsEnabled()) { Button->SetUserFocus(GetOwningPlayer()); if (bScroll) Scroller->ScrollWidgetIntoView(Button, false, EDescendantScrollDestination::IntoView, 8); return; }
    }
    SetUserFocus(GetOwningPlayer());
}
FName UCoastalPanelWidget::FocusedCommand() const
{
    for (const auto& Button : Buttons) if (Button->HasAnyUserFocus()) return Button->Command;
    return NAME_None;
}
void UCoastalPanelWidget::RestoreCommandFocus(FName Command, int32 Fallback, bool bScroll)
{
    if (!Command.IsNone())
        for (int32 I = 0; I < Buttons.Num(); ++I)
            if (Buttons[I]->Command == Command && Buttons[I]->GetIsEnabled())
            { FocusDefault(I, bScroll); return; }
    FocusDefault(Fallback, bScroll);
}
FReply UCoastalPanelWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    const auto Key = Event.GetKey();
    if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right)
    { if (!Event.IsRepeat()) Execute(TEXT("back")); return FReply::Handled(); }
    if (Key == EKeys::PageDown || Key == EKeys::Gamepad_RightShoulder)
    { Scroller->SetScrollOffset(Scroller->GetScrollOffset() + 240); return FReply::Handled(); }
    if (Key == EKeys::PageUp || Key == EKeys::Gamepad_LeftShoulder)
    { Scroller->SetScrollOffset(FMath::Max(0.0f, Scroller->GetScrollOffset() - 240)); return FReply::Handled(); }
    const bool Previous = Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || (Key == EKeys::Tab && Event.IsShiftDown());
    const bool Next = Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || (Key == EKeys::Tab && !Event.IsShiftDown());
    if (Previous || Next)
    {
        const bool IncludeName = Key == EKeys::Tab && SaveField && SaveField->IsVisible();
        const int32 Count = Buttons.Num() + (IncludeName ? 1 : 0);
        int32 Index = FocusIndex();
        const int32 PreviousIndex = Index;
        if (!IncludeName && Index == Buttons.Num()) Index = Previous ? 0 : Buttons.Num() - 1;
        for (int32 I = 0; I < Count; ++I)
        {
            Index = coastal::CycleSelection(Index, Previous ? -1 : 1, Count);
            if (IncludeName && Index == Buttons.Num()) { FocusDefault(Index); break; }
            if (Buttons[Index]->GetIsEnabled()) { Buttons[Index]->SetUserFocus(GetOwningPlayer()); Scroller->ScrollWidgetIntoView(Buttons[Index], false, EDescendantScrollDestination::IntoView, 8); break; }
        }
        if (Session && FocusIndex() != PreviousIndex) Session->NotifyMenuFocus(this);
        return FReply::Handled();
    }
    if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
    {
        if (SaveField && SaveField->HasAnyUserFocus()) return Super::NativeOnPreviewKeyDown(Geometry, Event);
        const int32 I = FocusIndex();
        if (!Event.IsRepeat() && Buttons.IsValidIndex(I) && Buttons[I]->GetIsEnabled()) Execute(Buttons[I]->Command);
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
void UCoastalPanelWidget::NativeTick(const FGeometry& Geometry, float Delta)
{
    Super::NativeTick(Geometry, Delta);
    if (Session && Notice) Notice->SetText(Session->PanelStatus(Ticket.kind));
    if (Session && Help) Help->SetText(Session->MenuHelp());
    if (Session && Heading && Ticket.kind == coastal::PanelKind::ConfirmDisplay)
        Heading->SetText(Session->DisplayConfirmationTitle());
    for (const auto& Button : Buttons)
        Button->SetBackgroundColor(Button->HasAnyUserFocus() ? CoastalUITheme::Focus() : CoastalUITheme::Button());
}
