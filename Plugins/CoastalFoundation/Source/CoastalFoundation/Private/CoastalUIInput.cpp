#include "CoastalUISessionComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

bool UCoastalUISessionComponent::InstallMenuInput()
{
    auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer());
    if (!Subsystem) { UIError = TEXT("Enhanced Input local-player subsystem is unavailable."); return false; }
    MenuContext = NewObject<UInputMappingContext>(this);
    MenuInput = NewObject<UEnhancedInputComponent>(Controller);
    if (!MenuContext || !MenuInput) return false;
    MenuInput->RegisterComponent(); MenuInput->Priority = 100; MenuInput->bBlockInput = false;
    const FKey Keyboard[] = {EKeys::Escape, EKeys::Tab, EKeys::J};
    const FKey Gamepad[] = {EKeys::Gamepad_Special_Right, EKeys::Gamepad_FaceButton_Top, EKeys::Gamepad_Special_Left};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        auto* Action = NewObject<UInputAction>(this);
        Action->ValueType = EInputActionValueType::Boolean;
        Action->bConsumeInput = true; Action->bTriggerWhenPaused = true;
        MenuActions.Add(Action);
        MenuContext->MapKey(Action, Keyboard[Index]); MenuContext->MapKey(Action, Gamepad[Index]);
    }
    MenuInput->BindAction(MenuActions[0], ETriggerEvent::Started, this, &UCoastalUISessionComponent::OpenPause);
    MenuInput->BindAction(MenuActions[1], ETriggerEvent::Started, this, &UCoastalUISessionComponent::OpenInventory);
    MenuInput->BindAction(MenuActions[2], ETriggerEvent::Started, this, &UCoastalUISessionComponent::OpenJournal);
    Controller->PushInputComponent(MenuInput);
    FModifyContextOptions MappingOptions; MappingOptions.bIgnoreAllPressedKeysUntilRelease = true;
    Subsystem->AddMappingContext(MenuContext, 100, MappingOptions);
    InstallInputHints();
    return true;
}
void UCoastalUISessionComponent::RemoveMenuInput()
{
    RemoveInputHints();
    if (IsValid(Controller))
    {
        if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()))
            if (MenuContext) Subsystem->RemoveMappingContext(MenuContext);
        if (MenuInput) Controller->PopInputComponent(MenuInput);
    }
    if (MenuInput) MenuInput->DestroyComponent();
    MenuInput = nullptr; MenuContext = nullptr; MenuActions.Reset();
}
void UCoastalUISessionComponent::ApplyInputOwnership()
{
    if (bInputOwned || !IsValid(Controller)) return;
    bPreviousCursor = Controller->bShowMouseCursor;
    Controller->SetIgnoreMoveInput(true); Controller->SetIgnoreLookInput(true);
    Controller->bShowMouseCursor = true;
    FInputModeUIOnly Mode; Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Controller->SetInputMode(Mode);
    if (!UGameplayStatics::IsGamePaused(this)) bPauseOwned = Controller->SetPause(true);
    bInputOwned = true;
}
void UCoastalUISessionComponent::ReleaseInputOwnership()
{
    if (!bInputOwned) return;
    if (IsValid(Controller))
    {
        Controller->SetIgnoreMoveInput(false); Controller->SetIgnoreLookInput(false);
        Controller->bShowMouseCursor = bPreviousCursor;
        Controller->SetInputMode(FInputModeGameOnly());
        if (bPauseOwned) Controller->SetPause(false);
        if (MenuContext)
            if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()))
            {
                FModifyContextOptions MappingOptions;
                MappingOptions.bIgnoreAllPressedKeysUntilRelease = true; MappingOptions.bForceImmediately = true;
                Subsystem->RequestRebuildControlMappings(MappingOptions, EInputMappingRebuildType::RebuildWithFlush);
            }
    }
    bInputOwned = bPauseOwned = false;
}
