#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoastalPlacementLibrary.generated.h"
class ACharacter;
UCLASS()
class COASTALFOUNDATION_API UCoastalPlacementLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Same M1 dry-room check used for startup and restore; not swimming/water recovery.
    UFUNCTION(BlueprintCallable, Category="Coastal|Integration")
    static bool IsDryDestination(ACharacter* Character, FTransform Transform);
};
