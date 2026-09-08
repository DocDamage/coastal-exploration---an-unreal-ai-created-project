#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoastalVendorInspection.generated.h"
class ACoastalSwimmingZone;
UCLASS()
class COASTALHOSTEDITOR_API UCoastalVendorInspection : public UBlueprintFunctionLibrary {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable, Category="Coastal|Local")
 static bool DumpBlueprint(const FString& AssetPath, const FString& OutputFile);
 UFUNCTION(BlueprintCallable, Category="Coastal|Local")
 static bool BuildIntegrationAssets();
 UFUNCTION(BlueprintCallable, Category="Coastal|Local")
 static bool BuildSwimmingZoneBox(ACoastalSwimmingZone* Zone, FVector Size);
 // Editor-only acceptance entry point; feeds the controller's real input stack.
 UFUNCTION(BlueprintCallable, Category="Coastal|Local")
 static bool SubmitCombatTestKey(FName KeyName, bool bPressed);
};
