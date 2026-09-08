#include "CoastalPanelWidget.h"
#include "CoastalUISessionComponent.h"
#include "CoastalUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "CoreGlobals.h"

void UCoastalCommandButton::BindCommand(FName Id)
{ Command = Id; OnClicked.AddUniqueDynamic(this, &UCoastalCommandButton::Clicked); }
void UCoastalCommandButton::Clicked() { OnCommand.Broadcast(Command); }
void UCoastalPanelWidget::Setup(UCoastalUISessionComponent* Owner, coastal::PanelTicket InTicket)
{ Session = Owner; Ticket = InTicket; SetIsFocusable(true); }
TSharedRef<SWidget> UCoastalPanelWidget::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Canvas;
    auto* Frame = WidgetTree->ConstructWidget<UBorder>();
    Frame->SetBrushColor(CoastalUITheme::Ink()); Frame->SetPadding(FMargin(28));
    auto* Position = Canvas->AddChildToCanvas(Frame);
    Position->SetAnchors(FAnchors(0.12f, 0.045f, 0.88f, 0.955f)); Position->SetOffsets(FMargin(0));
    Scroller = WidgetTree->ConstructWidget<UScrollBox>(); Frame->SetContent(Scroller);
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>(); Scroller->AddChild(Column);
    Heading = WidgetTree->ConstructWidget<UTextBlock>(); Heading->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 26));
    Heading->SetColorAndOpacity(CoastalUITheme::Paper());
    Heading->SetAutoWrapText(true); Column->AddChildToVerticalBox(Heading)->SetPadding(FMargin(0, 0, 0, 12));
    SaveField = WidgetTree->ConstructWidget<UEditableTextBox>();
    SaveField->SetForegroundColor(FLinearColor(0.025f,0.045f,0.065f,1.0f));
    SaveField->SetHintText(FText::FromString(TEXT("Save-set name (not a file path)")));
    SaveField->OnTextChanged.AddUniqueDynamic(this, &UCoastalPanelWidget::SaveFieldChanged);
    Column->AddChildToVerticalBox(SaveField)->SetPadding(FMargin(0, 0, 0, 12));
    Body = WidgetTree->ConstructWidget<UTextBlock>(); Body->SetAutoWrapText(true);
    Body->SetColorAndOpacity(CoastalUITheme::Paper());
    Body->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20)); Column->AddChildToVerticalBox(Body);
    Notice = WidgetTree->ConstructWidget<UTextBlock>(); Notice->SetAutoWrapText(true);
    Notice->SetColorAndOpacity(CoastalUITheme::Accent());
    Notice->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
    Column->AddChildToVerticalBox(Notice)->SetPadding(FMargin(0, 8));
    Actions = WidgetTree->ConstructWidget<UVerticalBox>(); Column->AddChildToVerticalBox(Actions);
    Help = WidgetTree->ConstructWidget<UTextBlock>(); Help->SetAutoWrapText(true);
    Help->SetColorAndOpacity(CoastalUITheme::Muted());
    Help->SetText(FText::FromString(TEXT("D-pad / Up-Down / Tab: focus | A / Enter: choose | B / Esc: back | shoulders / Page Up-Down: scroll")));
    Help->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14));
    Column->AddChildToVerticalBox(Help)->SetPadding(FMargin(0, 12, 0, 0));
    return Super::RebuildWidget();
}
void UCoastalPanelWidget::Refresh()
{
    if (!Session || !Actions) return;
    FText Title, Text; TArray<FCoastalUIChoice> Choices; Session->Present(Ticket.kind, Title, Text, Choices);
    Heading->SetText(Title); Body->SetText(Text); Notice->SetText(Session->PanelStatus(Ticket.kind));
    Heading->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Session->FontSize(26)));
    Body->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Session->FontSize(20)));
    Notice->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Session->FontSize(16)));
    Help->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Session->FontSize(14)));
    auto EditableStyle = SaveField->GetWidgetStyle();
    EditableStyle.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Session->FontSize(19)));
    SaveField->SetWidgetStyle(EditableStyle);
    // UE 5.7 SetWidgetStyle forwards &InStyle to Slate. Rebind to the text
    // box's owned WidgetStyle before this temporary leaves scope.
    SaveField->SynchronizeProperties();
    const bool IsSession = Ticket.kind == coastal::PanelKind::Session;
    SaveField->SetVisibility(IsSession ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (IsSession) SaveField->SetText(Session->SelectedSaveSet.IsNone() ? FText::GetEmpty() : FText::FromName(Session->SelectedSaveSet));
    Actions->ClearChildren(); Buttons.Reset();
    for (const auto& Choice : Choices)
    {
        auto* Button = WidgetTree->ConstructWidget<UCoastalCommandButton>();
        Button->SetStyle(CoastalUITheme::CommandStyle());
        Button->SetBackgroundColor(CoastalUITheme::Button());
        Button->BindCommand(Choice.Command); Button->SetIsEnabled(Choice.bEnabled);
        Button->OnCommand.AddUniqueDynamic(this, &UCoastalPanelWidget::Execute);
        auto* Content = WidgetTree->ConstructWidget<UHorizontalBox>();
        if (UTexture2D* Icon = Choice.Icon.Get())
        {
            auto* Image = WidgetTree->ConstructWidget<UImage>();
            Image->SetBrushFromTexture(Icon, false);
            // Persist the size before Slate exists; SetDesiredSizeOverride only
            // affects an already-built SImage and is lost during initial setup.
            auto IconBrush = Image->GetBrush(); IconBrush.SetImageSize(FVector2D(48.0f, 48.0f));
            Image->SetBrush(IconBrush);
            auto* IconSlot = Content->AddChildToHorizontalBox(Image);
            IconSlot->SetPadding(FMargin(0, 0, 8, 0)); IconSlot->SetVerticalAlignment(VAlign_Center);
        }
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(Choice.Label);
        Label->SetColorAndOpacity(Choice.bEnabled ? CoastalUITheme::Paper() : CoastalUITheme::Muted());
        Label->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Session->FontSize(19))); Label->SetAutoWrapText(true);
        Label->SetJustification(Choice.Icon ? ETextJustify::Left : ETextJustify::Center);
        auto* LabelSlot = Content->AddChildToHorizontalBox(Label);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); LabelSlot->SetVerticalAlignment(VAlign_Center);
        if(auto* ContentSlot=Cast<UButtonSlot>(Button->AddChild(Content)))ContentSlot->SetHorizontalAlignment(HAlign_Fill);
        Buttons.Add(Button);
        Actions->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 4));
    }
}
void UCoastalPanelWidget::Execute(FName Id)
{ if (Session) Session->Command(this, Id); }
void UCoastalPanelWidget::SaveFieldChanged(const FText& Text)
{ if (Session) Session->SetSaveSetText(Text); }
void UCoastalPanelWidget::ScrollToTop() { if (Scroller) Scroller->ScrollToStart(); }
bool UCoastalPanelWidget::HasBeenPresented() const { return Presented.Ready(GFrameCounter); }
int32 UCoastalPanelWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& Culling, FSlateWindowElementList& Elements, int32 Layer,
    const FWidgetStyle& Style, bool bEnabled) const
{
    const int32 Result = Super::NativePaint(Args, Geometry, Culling, Elements, Layer, Style, bEnabled);
    if (bEnabled && Body && Scroller && !Body->GetText().IsEmpty())
    {
        const auto B = Body->GetCachedGeometry().GetLayoutBoundingRect();
        const auto V = Scroller->GetCachedGeometry().GetLayoutBoundingRect();
        if (coastal::VisibleUIIntersection({B.Left, B.Top, B.Right, B.Bottom}, {V.Left, V.Top, V.Right, V.Bottom}))
            Presented.MarkPainted(GFrameCounter);
    }
    return Result;
}
