#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoastalFoundationTypes.h"
#include "CoastalContractLibrary.generated.h"

UCLASS()
class COASTALFOUNDATION_API UCoastalContractLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Coastal|Contracts")
    static bool MakeRequirementsFingerprint(const TArray<FCoastalItemRequirement>& Requirements, FString& Fingerprint);
    UFUNCTION(BlueprintPure, Category="Coastal|Contracts")
    static FName PickupTransactionId(FName WorldId);
};
