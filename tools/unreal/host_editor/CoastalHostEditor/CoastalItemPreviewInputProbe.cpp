#include "CoastalVendorInspection.h"
#include "CoastalMissionDirector.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalUISessionComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "Misc/Paths.h"

bool UCoastalVendorInspection::SubmitItemPreviewTestInput(FName KeyName, float Value)
{
    UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    if (!IsInGameThread() || !World || World->GetNetMode() != NM_Standalone || !FSlateApplication::IsInitialized()
        || !FMath::IsFinite(Value) || FMath::Abs(Value) > 1.f
        || !FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()).Contains(TEXT("/LocalHost58/"))) return false;
    ACoastalMissionDirector* Director = nullptr;
    for (TActorIterator<ACoastalMissionDirector> It(World); It; ++It)
    { if (Director) return false; Director = *It; }
    if (!Director || !Director->Saves || !Director->Saves->HasActiveCampaign()
        || !Director->Saves->GetActiveSaveSet().ToString().StartsWith(TEXT("coastal_test_"))) return false;
    auto* PC = UGameplayStatics::GetPlayerController(World, 0);
    auto* UI = PC ? PC->FindComponentByClass<UCoastalUISessionComponent>() : nullptr;
    if (!UI || !UI->IsInitialized() || !UI->ItemPreviewTexture()) return false;
    const FKey Key(KeyName);
    if (Key == EKeys::Gamepad_RightX || Key == EKeys::Gamepad_RightY)
        return FSlateApplication::Get().ProcessAnalogInputEvent(FAnalogInputEvent(Key, FModifierKeysState(), 0, false, 0, 0, Value));
    if (Key != EKeys::W && Key != EKeys::A && Key != EKeys::S && Key != EKeys::D) return false;
    if (Value != 0.f) return FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
    return FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(Key, FModifierKeysState(), 0, false, 0, 0));
}
