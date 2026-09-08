#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoastalHUDWidget.generated.h"
class UCoastalUISessionComponent;
class UTextBlock;
class USizeBox;
class UBorder;
UCLASS()
class COASTALFOUNDATION_API UCoastalHUDWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Setup(UCoastalUISessionComponent* Owner);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float Delta) override;
private:
    UPROPERTY() TObjectPtr<UCoastalUISessionComponent> Session;
    UPROPERTY() TObjectPtr<UTextBlock> ObjectiveLabel;
    UPROPERTY() TObjectPtr<UTextBlock> NoticeLabel;
    UPROPERTY() TObjectPtr<UTextBlock> Title;
    UPROPERTY() TObjectPtr<UTextBlock> Controls;
    UPROPERTY() TObjectPtr<USizeBox> WidthBox;
    UPROPERTY() TObjectPtr<UBorder> Frame;
    float RefreshDelay = 0;
};
