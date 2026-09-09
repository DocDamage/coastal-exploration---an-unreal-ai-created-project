#include "CoastalGrappleAnchor.h"
#include "Collision/RopeWrapTargetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACoastalGrappleAnchor::ACoastalGrappleAnchor()
{
    AnchorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnchorMesh"));
    SetRootComponent(AnchorMesh);
    AnchorMesh->SetMobility(EComponentMobility::Static);
    AnchorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded()) AnchorMesh->SetStaticMesh(Cylinder.Object);
    WrapTarget = CreateDefaultSubobject<URopeWrapTargetComponent>(TEXT("DynamicRopeAnchor"));
    WrapTarget->TargetComponent = AnchorMesh;
    Tags.Add(TEXT("Coastal.GrappleAnchor"));
}
