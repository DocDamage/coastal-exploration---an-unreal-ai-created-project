#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Core/UIFlowRules.h"
#include "CoastalPanelWidget.generated.h"
class UCoastalUISessionComponent;
class UVerticalBox;
class UTextBlock;
class UEditableTextBox;
class UScrollBox;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCoastalUICommand, FName, Command);

UCLASS()
class COASTALFOUNDATION_API UCoastalCommandButton : public UButton
{
    GENERATED_BODY()
public:
    FName Command;
    UPROPERTY() FCoastalUICommand OnCommand;
    void BindCommand(FName Id);
private:
    UFUNCTION() void Clicked();
};

// Native constructed UMG: no missing WidgetBlueprint assets.
UCLASS()
class COASTALFOUNDATION_API UCoastalPanelWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Setup(UCoastalUISessionComponent* Owner, coastal::PanelTicket InTicket);
    void Refresh();
    void FocusDefault(int32 Preferred = 0, bool bScroll = true);
    int32 FocusIndex() const;
    FName FocusedCommand() const;
    void RestoreCommandFocus(FName Command, int32 Fallback, bool bScroll);
    coastal::PanelTicket Ticket;
    bool HasBeenPresented() const;
    void ResetPresentation() { Presented = {}; }
    void ScrollToTop();
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual void NativeTick(const FGeometry& Geometry, float Delta) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const override;
private:
    UPROPERTY() TObjectPtr<UCoastalUISessionComponent> Session;
    UPROPERTY() TObjectPtr<UVerticalBox> Actions;
    UPROPERTY() TObjectPtr<UTextBlock> Heading;
    UPROPERTY() TObjectPtr<UTextBlock> Body;
    UPROPERTY() TObjectPtr<UTextBlock> Notice;
    UPROPERTY() TObjectPtr<UTextBlock> Help;
    UPROPERTY() TObjectPtr<UEditableTextBox> SaveField;
    UPROPERTY() TObjectPtr<UScrollBox> Scroller;
    UPROPERTY() TArray<TObjectPtr<UCoastalCommandButton>> Buttons;
    mutable coastal::PresentationGate Presented;
    UFUNCTION() void Execute(FName Command);
    UFUNCTION() void SaveFieldChanged(const FText& Text);
};
