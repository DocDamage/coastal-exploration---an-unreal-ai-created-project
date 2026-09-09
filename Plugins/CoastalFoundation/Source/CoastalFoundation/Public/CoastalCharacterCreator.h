#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CoastalCharacterCreator.generated.h"

struct FCoastalUIChoice;
class UTextureRenderTarget2D;

// Optional character presentation seam. The existing UI retains input ownership.
UINTERFACE(MinimalAPI)
class UCoastalCharacterCreator : public UInterface
{
    GENERATED_BODY()
};

class COASTALFOUNDATION_API ICoastalCharacterCreator
{
    GENERATED_BODY()
public:
    virtual bool CanEditCharacter() const = 0;
    virtual bool BeginCharacterEdit() = 0;
    virtual void EndCharacterEdit() = 0;
    virtual void PresentCharacter(FText& Body, TArray<FCoastalUIChoice>& Choices) = 0;
    virtual void CharacterCommand(FName Command) = 0;
    virtual UTextureRenderTarget2D* CharacterPreview() const = 0;
    virtual FText CharacterStatus() const = 0;
    virtual FText CharacterActionHint(bool Gamepad) const { return FText::GetEmpty(); }
};
