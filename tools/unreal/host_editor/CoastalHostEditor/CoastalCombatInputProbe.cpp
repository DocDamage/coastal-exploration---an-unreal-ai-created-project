#include "CoastalVendorInspection.h"
#include "CoastalMissionDirector.h"
#include "CoastalSaveCoordinator.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

bool UCoastalVendorInspection::SubmitCombatTestKey(FName KeyName, bool bPressed)
{
    UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!IsInGameThread() || !World || World->GetNetMode() != NM_Standalone
        || !FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()).Contains(TEXT("/LocalHost58/")))
        return false;
    const FKey Key(KeyName);
    if (Key != EKeys::LeftMouseButton && Key != EKeys::R
        && Key != EKeys::Gamepad_RightTrigger && Key != EKeys::Gamepad_LeftShoulder
        && Key != EKeys::RightMouseButton && Key != EKeys::Gamepad_LeftTrigger
        && Key != EKeys::G && Key != EKeys::Gamepad_DPad_Down
        && Key != EKeys::Q && Key != EKeys::Z && Key != EKeys::X
        && Key != EKeys::W && Key != EKeys::A && Key != EKeys::S && Key != EKeys::D && Key != EKeys::SpaceBar
        && Key != EKeys::V && Key != EKeys::Gamepad_RightThumbstick
        && Key != EKeys::C && Key != EKeys::Gamepad_FaceButton_Right
        && Key != EKeys::Gamepad_DPad_Up && Key != EKeys::Gamepad_DPad_Left && Key != EKeys::Gamepad_DPad_Right) return false;
    ACoastalMissionDirector* Director = nullptr;
    for (TActorIterator<ACoastalMissionDirector> It(World); It; ++It)
    {
        if (Director) return false;
        Director = *It;
    }
    if (!IsValid(Director) || !IsValid(Director->Saves) || !Director->Saves->HasActiveCampaign()
        || !Director->Saves->GetActiveSaveSet().ToString().StartsWith(TEXT("coastal_test_"))) return false;
    APlayerController* Controller = UGameplayStatics::GetPlayerController(World, 0);
    if (!IsValid(Controller) || !Controller->IsLocalController()) return false;
    Controller->InputKey(FInputKeyEventArgs(nullptr,
        IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), Key,
        bPressed ? IE_Pressed : IE_Released, bPressed ? 1.0f : 0.0f,
        Key.IsGamepadKey(), FPlatformTime::Cycles64()));
    return true;
}

AActor* UCoastalVendorInspection::SpawnZiplineTestObstacle(FVector Location, FVector Scale)
{
    UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!IsInGameThread() || !World || World->GetNetMode() != NM_Standalone
        || !FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()).Contains(TEXT("/LocalHost58/"))
        || Location.ContainsNaN() || Scale.ContainsNaN() || Scale.GetMin() < .1 || Scale.GetMax() > 3
        || Location.X < 12000 || Location.X > 15000 || Location.Y < 1000 || Location.Y > 2000
        || Location.Z < 300 || Location.Z > 700) return nullptr;
    ACoastalMissionDirector* Director = nullptr;
    for (TActorIterator<ACoastalMissionDirector> It(World); It; ++It)
    { if (Director) return nullptr; Director = *It; }
    if (!IsValid(Director) || !IsValid(Director->Saves) || !Director->Saves->HasActiveCampaign()
        || !Director->Saves->GetActiveSaveSet().ToString().StartsWith(TEXT("coastal_test_"))) return nullptr;
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Actor = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
    if (!Actor) return nullptr;
    Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    Actor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
    Actor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
    Actor->SetActorScale3D(Scale);
    Actor->Tags.Add(TEXT("Coastal.TestProbe"));
    return Actor;
}
