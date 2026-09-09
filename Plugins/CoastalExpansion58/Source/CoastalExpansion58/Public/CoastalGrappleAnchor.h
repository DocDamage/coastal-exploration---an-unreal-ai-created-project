#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoastalGrappleAnchor.generated.h"

class UStaticMeshComponent;
class URopeWrapTargetComponent;

UCLASS(Blueprintable)
class COASTALEXPANSION58_API ACoastalGrappleAnchor : public AActor
{
    GENERATED_BODY()
public:
    ACoastalGrappleAnchor();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> AnchorMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<URopeWrapTargetComponent> WrapTarget;
};
