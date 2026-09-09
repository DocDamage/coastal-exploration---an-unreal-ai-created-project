#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CoastalCharacterSubsystem.generated.h"

// Installs one cosmetic component after the existing host has initialized.
UCLASS()
class COASTALEXPANSION58_API UCoastalCharacterSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
    virtual void Tick(float Delta) override;
    virtual TStatId GetStatId() const override;
private:
    bool bAttempted = false;
};
