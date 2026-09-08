#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoastalMissionDirector.generated.h"

class UFirstSignalComponent;
class UCoastalSaveCoordinator;
UCLASS(Blueprintable)
class COASTALFOUNDATION_API ACoastalMissionDirector : public AActor
{
    GENERATED_BODY()
public:
    ACoastalMissionDirector();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal")
    TObjectPtr<UFirstSignalComponent> FirstSignal;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Coastal")
    TObjectPtr<UCoastalSaveCoordinator> Saves;
};
