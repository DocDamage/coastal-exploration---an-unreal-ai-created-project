#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoastalFoundationTypes.h"
#include "CoastalStoryLibrary.generated.h"
UCLASS()
class COASTALFOUNDATION_API UCoastalStoryLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Coastal|Journal")
    static FText RadioTranscript();
    UFUNCTION(BlueprintPure, Category="Coastal|Journal")
    static FText ObjectiveText(EFirstSignalPhase Phase);
    UFUNCTION(BlueprintPure, Category="Coastal|Journal")
    static FText CampaignObjective(EFirstSignalPhase Phase, const TArray<FName>& Journal);
    UFUNCTION(BlueprintPure, Category="Coastal|Journal")
    static FText CampaignTitle(EFirstSignalPhase Phase, const TArray<FName>& Journal);
    UFUNCTION(BlueprintPure, Category="Coastal|Journal")
    static FText DiscoveryAction(FName WorldId, bool bAlreadyRead);
    UFUNCTION(BlueprintPure, Category="Coastal|Journal")
    static FText JournalText(FName EntryId);
    UFUNCTION(BlueprintPure, Category="Coastal|Journal")
    static FText JournalTitle(FName EntryId);
};
