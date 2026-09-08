#include "CoastalWorldObject.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACoastalWorldObject::ACoastalWorldObject()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    ProxyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DevelopmentProxy"));
    ProxyMesh->SetupAttachment(RootComponent);
    ProxyMesh->SetMobility(EComponentMobility::Movable);
    ProxyMesh->SetCollisionProfileName(TEXT("BlockAll"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) ProxyMesh->SetStaticMesh(Cube.Object);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DevelopmentLabel"));
    Label->SetupAttachment(RootComponent);
    Label->SetWorldSize(18.0f);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetRelativeRotation(FRotator(0, 180, 0));
}
void ACoastalWorldObject::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ProxyMesh->SetStaticMesh(VisualMesh ? VisualMesh.Get() : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    ProxyMesh->SetRelativeTransform(VisualMesh ? VisualTransform : FTransform(FQuat::Identity, FVector::ZeroVector, ProxySizeCm / 100.0));
    Label->SetRelativeLocation(FVector(0, 0, ProxySizeCm.Z * 0.5 + 40));
    Label->SetText(FText::FromString(TEXT("DEV PROXY\n") + DisplayLabel));
    Label->SetVisibility(!VisualMesh);
    ApplyNativeState(bActive);
}
void ACoastalWorldObject::ApplyNativeState(bool bNewActive)
{
    bActive = bNewActive;
    const bool bCollected = Kind == ECoastalObjectKind::Pickup && bActive;
    SetActorHiddenInGame(bCollected && !bPersistentPickupContainer);
    SetActorEnableCollision(!bCollected || bPersistentPickupContainer);
    if (Kind == ECoastalObjectKind::Door)
    {
        FRotator Rotation = VisualMesh ? VisualTransform.Rotator() : FRotator::ZeroRotator;
        Rotation.Yaw += bActive ? 90.0 : 0.0;
        ProxyMesh->SetRelativeRotation(Rotation);
    }
}

FVector ACoastalWorldObject::GetInteractionPoint() const
{ return VisualMesh && ProxyMesh ? ProxyMesh->Bounds.Origin : GetActorLocation(); }

void ACoastalWorldObject::RefreshDevelopmentProxy() { OnConstruction(GetActorTransform()); }
