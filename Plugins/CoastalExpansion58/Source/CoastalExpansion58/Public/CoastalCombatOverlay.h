#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CoastalCombatOverlay.generated.h"

class UCoastalCombatComponent;
class UTextBlock;

UCLASS(NotBlueprintable)
class COASTALEXPANSION58_API UCoastalCombatOverlay : public UUserWidget
{
    GENERATED_BODY()
public:
    bool BindCombat(UCoastalCombatComponent* Owner);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    UPROPERTY() TObjectPtr<UCoastalCombatComponent> Combat;
    UPROPERTY() TObjectPtr<UTextBlock> StatusText;
    UPROPERTY() TObjectPtr<UTextBlock> CrosshairText;
    UFUNCTION() void Refresh(float Health, float Shield, int32 Clip, int32 Reserve, bool Armed);
};
