#include "CoastalCombatOverlay.h"
#include "CoastalCombatComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

bool UCoastalCombatOverlay::BindCombat(UCoastalCombatComponent* Owner)
{
    if (!IsValid(Owner) || Combat) return false;
    Combat = Owner;
    Combat->OnCombatStateChanged.AddUniqueDynamic(this, &UCoastalCombatOverlay::Refresh);
    return true;
}

TSharedRef<SWidget> UCoastalCombatOverlay::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CombatRoot"));
    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CombatStatus"));
    CrosshairText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CombatCrosshair"));
    WidgetTree->RootWidget = Root;
    if (!Root || !StatusText || !CrosshairText) return Super::RebuildWidget();
    StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.95f, 1.f, 1.f)));
    StatusText->SetShadowOffset(FVector2D(1, 1));
    CrosshairText->SetText(FText::FromString(TEXT("+")));
    CrosshairText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, .45f, .15f, 1.f)));
    if (auto* StatusSlot = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(StatusText)))
    {
        StatusSlot->SetAnchors(FAnchors(1, 1)); StatusSlot->SetAlignment(FVector2D(1, 1));
        StatusSlot->SetPosition(FVector2D(-48, -48)); StatusSlot->SetAutoSize(true);
    }
    if (auto* CrosshairSlot = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(CrosshairText)))
    {
        CrosshairSlot->SetAnchors(FAnchors(.5f, .5f)); CrosshairSlot->SetAlignment(FVector2D(.5f, .5f));
        CrosshairSlot->SetAutoSize(true);
    }
    return Super::RebuildWidget();
}

void UCoastalCombatOverlay::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (IsValid(Combat)) Refresh(Combat->GetHealth(), Combat->GetShield(),
        Combat->GetCurrentAmmo(), Combat->GetReserveAmmo(), Combat->GetTransientWeapon() != nullptr);
}

void UCoastalCombatOverlay::Refresh(float Health, float Shield, int32 Clip, int32 Reserve, bool Armed)
{
    if (!StatusText || !CrosshairText) return;
    const ESlateVisibility DisplayVisibility = Armed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
    StatusText->SetVisibility(DisplayVisibility); CrosshairText->SetVisibility(DisplayVisibility);
    StatusText->SetText(FText::FromString(FString::Printf(TEXT("HEALTH %.0f   SHIELD %.0f   AMMO %d / %d\nAim: RMB / LT    Fire: LMB / RT    Reload: R / LB"),
        Health, Shield, FMath::Max(0, Clip), FMath::Max(0, Reserve))));
}

void UCoastalCombatOverlay::NativeDestruct()
{
    if (IsValid(Combat)) Combat->OnCombatStateChanged.RemoveDynamic(this, &UCoastalCombatOverlay::Refresh);
    Combat = nullptr;
    Super::NativeDestruct();
}
