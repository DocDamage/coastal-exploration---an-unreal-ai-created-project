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

bool UCoastalVendorInspection::SubmitCombatTestKey(FName KeyName, bool bPressed)
{
    UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!IsInGameThread() || !World || World->GetNetMode() != NM_Standalone
        || !FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()).Contains(TEXT("/LocalHost58/")))
        return false;
    const FKey Key(KeyName);
    if (Key != EKeys::LeftMouseButton && Key != EKeys::R
        && Key != EKeys::Gamepad_RightTrigger && Key != EKeys::Gamepad_LeftShoulder) return false;
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
