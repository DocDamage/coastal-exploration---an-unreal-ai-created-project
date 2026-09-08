#include "CoastalHUDWidget.h"
#include "CoastalUISessionComponent.h"
#include "CoastalUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UCoastalHUDWidget::Setup(UCoastalUISessionComponent* Owner)
{ Session = Owner; SetVisibility(ESlateVisibility::HitTestInvisible); }
TSharedRef<SWidget> UCoastalHUDWidget::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(); WidgetTree->RootWidget = Canvas;
    Frame = WidgetTree->ConstructWidget<UBorder>(); auto* Border = Frame.Get(); Border->SetPadding(FMargin(16));
    Border->SetBrushColor(CoastalUITheme::Ink());
    auto* CanvasSlot = Canvas->AddChildToCanvas(Border); CanvasSlot->SetPosition(FVector2D(24, 24)); CanvasSlot->SetAutoSize(true);
    WidthBox = WidgetTree->ConstructWidget<USizeBox>(); WidthBox->SetWidthOverride(480); Border->SetContent(WidthBox);
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>(); WidthBox->SetContent(Column);
    Title = WidgetTree->ConstructWidget<UTextBlock>(); Title->SetAutoWrapText(true); Title->SetText(FText::FromString(TEXT("FIRST SIGNAL")));
    Title->SetColorAndOpacity(CoastalUITheme::Accent());
    Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 18)); Column->AddChildToVerticalBox(Title);
    ObjectiveLabel = WidgetTree->ConstructWidget<UTextBlock>(); ObjectiveLabel->SetAutoWrapText(true);
    ObjectiveLabel->SetColorAndOpacity(CoastalUITheme::Paper());
    ObjectiveLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 21)); Column->AddChildToVerticalBox(ObjectiveLabel);
    NoticeLabel = WidgetTree->ConstructWidget<UTextBlock>(); NoticeLabel->SetAutoWrapText(true);
    NoticeLabel->SetColorAndOpacity(CoastalUITheme::Accent());
    NoticeLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16)); Column->AddChildToVerticalBox(NoticeLabel);
    Controls = WidgetTree->ConstructWidget<UTextBlock>(); Controls->SetAutoWrapText(true);
    Controls->SetColorAndOpacity(CoastalUITheme::Muted());
    Controls->SetText(FText::FromString(TEXT("Esc / Menu: pause   |   Tab / Y: inventory   |   J / View: journal")));
    Controls->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16)); Column->AddChildToVerticalBox(Controls);
    return Super::RebuildWidget();
}
void UCoastalHUDWidget::NativeTick(const FGeometry& Geometry, float Delta)
{
    Super::NativeTick(Geometry, Delta); RefreshDelay -= Delta;
    if (!Session || !ObjectiveLabel || !NoticeLabel || RefreshDelay > 0) return;
    Frame->SetVisibility(Session->HasModal() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    // Keep the coast visible; honor the existing text-size preference with wrapping.
    const float Available = FMath::Max(1.0f, static_cast<float>(Geometry.GetLocalSize().X) - 80.0f);
    const float Desired = 480.0f * Session->FontSize(20) / 20.0f;
    WidthBox->SetWidthOverride(FMath::Min(Available, Desired));
    Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Session->FontSize(18)));
    ObjectiveLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Session->FontSize(21)));
    NoticeLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Session->FontSize(16)));
    Controls->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Session->FontSize(16)));
    Controls->SetText(Session->HUDControls());
    RefreshDelay = 0.2f; Title->SetText(Session->CampaignTitle());
    ObjectiveLabel->SetText(Session->Objective()); NoticeLabel->SetText(Session->Status());
}
