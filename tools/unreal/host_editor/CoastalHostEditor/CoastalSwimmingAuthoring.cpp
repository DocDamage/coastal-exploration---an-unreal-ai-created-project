#include "CoastalVendorInspection.h"
#include "CoastalSwimmingZone.h"
#include "ActorFactories/ActorFactory.h"
#include "Builders/CubeBuilder.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Components/BrushComponent.h"

bool UCoastalVendorInspection::BuildSwimmingZoneBox(ACoastalSwimmingZone* Zone, FVector Size)
{
    if (!IsInGameThread() || !GEditor || GEditor->PlayWorld || !IsValid(Zone)
        || !Zone->GetWorld() || Zone->GetWorld()->WorldType != EWorldType::Editor
        || Zone->GetWorld()->GetName() != TEXT("L_FirstSignal")
        // First Signal's authored water is one continuous plane across the playable coast.
        || Size.ContainsNaN() || Size.GetMin() < 100.0 || Size.GetMax() > 100000.0
        || !Zone->GetActorScale3D().Equals(FVector::OneVector)
        || !Zone->GetActorRotation().IsNearlyZero()) return false;

    // Use the engine's volume factory path so the brush has real BSP collision
    // for CharacterMovement::ImmersionDepth, including cooked brush geometry.
    auto* Builder = NewObject<UCubeBuilder>(GetTransientPackage());
    Builder->X = Size.X; Builder->Y = Size.Y; Builder->Z = Size.Z;
    Builder->Hollow = false; Builder->Tessellated = false;
    Zone->Modify();
    UActorFactory::CreateBrushForVolumeActor(Zone, Builder);
    Zone->SetActorHiddenInGame(true);
    UBrushComponent* VolumeBrush = Zone->GetBrushComponent();
    VolumeBrush->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    VolumeBrush->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    VolumeBrush->SetGenerateOverlapEvents(true);
    VolumeBrush->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    VolumeBrush->RecreatePhysicsState();
    VolumeBrush->UpdateOverlaps();
    Zone->MarkPackageDirty();
    return Zone->IsAuthoredCorrectly();
}
