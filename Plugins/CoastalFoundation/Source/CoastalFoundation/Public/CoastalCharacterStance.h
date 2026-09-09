#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CoastalCharacterStance.generated.h"

UINTERFACE(MinimalAPI)
class UCoastalCharacterStance : public UInterface { GENERATED_BODY() };

// Transient stance owns capsule changes; save/recovery still own placement.
class COASTALFOUNDATION_API ICoastalCharacterStance
{
    GENERATED_BODY()
public:
    virtual bool IsStanceActive() const = 0;
    virtual FTransform StandingSaveTransform() const = 0;
    virtual void PrepareStandingPlacement() = 0;
};
