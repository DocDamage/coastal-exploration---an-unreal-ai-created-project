#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoastalIntegrationTypes.h"
#include "CoastalIntegrationLibrary.generated.h"
class UCoastalInventoryAdapter;
UCLASS()
class COASTALFOUNDATION_API UCoastalIntegrationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Read-only authored M1 manifest check. Supports editor world or initial play world.
    UFUNCTION(BlueprintCallable, Category="Coastal|Integration", meta=(WorldContext="WorldContextObject"))
    static bool AuditTestRoom(const UObject* WorldContextObject, FCoastalIntegrationReport& Report, bool CapacityFixture = false);
    // Struct-return convenience for editor Python; no out-parameter tuple assumptions.
    UFUNCTION(BlueprintCallable, Category="Coastal|Integration", meta=(WorldContext="WorldContextObject"))
    static FCoastalIntegrationReport InspectTestRoom(const UObject* WorldContextObject);
    // Read-only calls on the REAL adapter. Requires a fresh provisional session; no mutation probes.
    UFUNCTION(BlueprintCallable, Category="Coastal|Integration")
    static bool AuditProvisionalProvider(UCoastalInventoryAdapter* Provider, FCoastalIntegrationReport& Report);
};
