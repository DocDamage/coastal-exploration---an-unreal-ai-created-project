#pragma once
#include "CoreMinimal.h"
#include "CoastalInteractionOffer.generated.h"

// A read-only prompt projection. Hyper renders it; it is never a reservation or permission token.
USTRUCT(BlueprintType)
struct COASTALFOUNDATION_API FCoastalInteractionOffer
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Coastal|Interaction") FName WorldId;
    UPROPERTY(BlueprintReadOnly, Category="Coastal|Interaction") FText ActionText;
    UPROPERTY(BlueprintReadOnly, Category="Coastal|Interaction") FText Detail;
    UPROPERTY(BlueprintReadOnly, Category="Coastal|Interaction") bool bVisible = false;
    UPROPERTY(BlueprintReadOnly, Category="Coastal|Interaction") bool bCanInteract = false;
    bool SamePresentation(const FCoastalInteractionOffer& Other) const
    {
        return WorldId == Other.WorldId && bVisible == Other.bVisible && bCanInteract == Other.bCanInteract
            && ActionText.EqualTo(Other.ActionText) && Detail.EqualTo(Other.Detail);
    }
};
