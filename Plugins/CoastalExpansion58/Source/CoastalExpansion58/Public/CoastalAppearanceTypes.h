#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/SaveGame.h"
#include "CoastalAppearanceTypes.generated.h"

class UCustomizableObjectInstance;
class UIKRetargeter;
class USkeleton;
class UAnimSequence;

UENUM()
enum class ECoastalAppearanceField : uint8 { Choice, Scalar, Color };

USTRUCT(BlueprintType)
struct FCoastalAppearanceControl
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FString Parameter;
    UPROPERTY(EditAnywhere) FString LinkedParameter;
    UPROPERTY(EditAnywhere) FText Label;
    UPROPERTY(EditAnywhere) ECoastalAppearanceField Kind = ECoastalAppearanceField::Choice;
    UPROPERTY(EditAnywhere) TArray<FString> Choices;
    UPROPERTY(EditAnywhere) TArray<FLinearColor> Colors;
    UPROPERTY(EditAnywhere) float Minimum = 0.f;
    UPROPERTY(EditAnywhere) float Maximum = 1.f;
    UPROPERTY(EditAnywhere) float Step = 0.1f;
    UPROPERTY(EditAnywhere) int32 DefaultSelection = 0;
};

USTRUCT(BlueprintType)
struct FCoastalCharacterAction
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) TObjectPtr<UAnimSequence> Clip;
    UPROPERTY(EditAnywhere) float StartSeconds = 0.f;
    UPROPERTY(EditAnywhere) float DurationSeconds = 2.f;
    UPROPERTY(EditAnywhere) bool bUpperBody = false;
};

UCLASS()
class COASTALEXPANSION58_API UCoastalCharacterDefinition : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) FName AppearanceVersion = TEXT("coastal.human.v1");
    UPROPERTY(EditAnywhere) TObjectPtr<UCustomizableObjectInstance> DefaultInstance;
    UPROPERTY(EditAnywhere) TArray<FCoastalAppearanceControl> Controls;
    UPROPERTY(EditAnywhere) TMap<TObjectPtr<USkeleton>, TObjectPtr<UIKRetargeter>> Retargeters;
    UPROPERTY(EditAnywhere) TMap<FName, FCoastalCharacterAction> Actions;
    UPROPERTY(EditAnywhere) TArray<FName> Emotes;
    UPROPERTY(EditAnywhere) TArray<FName> IdleVariations;
};

// Cosmetic sidecar keyed by the campaign GUID. Legacy campaign bytes stay intact.
UCLASS()
class COASTALEXPANSION58_API UCoastalAppearanceSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Schema = 1;
    UPROPERTY(SaveGame) FGuid Campaign;
    UPROPERTY(SaveGame) FName AppearanceVersion;
    UPROPERTY(SaveGame) int64 Generation = 0;
    UPROPERTY(SaveGame) TArray<int32> Selections;
};
