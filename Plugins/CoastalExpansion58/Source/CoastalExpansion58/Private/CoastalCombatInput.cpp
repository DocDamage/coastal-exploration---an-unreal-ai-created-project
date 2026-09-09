#include "CoastalCombatComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

bool UCoastalCombatComponent::InstallInput()
{
    if (!IsValid(Controller) || bInputInstalled || !Controller->GetLocalPlayer()
        || !Cast<UEnhancedPlayerInput>(Controller->PlayerInput)) return false;
    auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer());
    if (!Subsystem || !FireKeyboardKey.IsValid() || !FireGamepadKey.IsValid()
        || !ReloadKeyboardKey.IsValid() || !ReloadGamepadKey.IsValid()) return false;
    CombatContext = NewObject<UInputMappingContext>(this);
    CombatInput = NewObject<UEnhancedInputComponent>(Controller);
    UInputAction* Fire = NewObject<UInputAction>(this);
    UInputAction* Reload = NewObject<UInputAction>(this);
    UInputAction* Aim = NewObject<UInputAction>(this);
    if (!CombatContext || !CombatInput || !Fire || !Reload || !Aim) return false;
    CombatActions = {Fire, Reload, Aim};
    for (UInputAction* Action : CombatActions)
    {
        Action->ValueType = EInputActionValueType::Boolean;
        Action->bConsumeInput = true;
        Action->bTriggerWhenPaused = false;
    }
    CombatContext->MapKey(Fire, FireKeyboardKey);
    CombatContext->MapKey(Fire, FireGamepadKey);
    CombatContext->MapKey(Reload, ReloadKeyboardKey);
    CombatContext->MapKey(Reload, ReloadGamepadKey);
    CombatContext->MapKey(Aim, EKeys::RightMouseButton);
    CombatContext->MapKey(Aim, EKeys::Gamepad_LeftTrigger);
    CombatInput->Priority = 5;
    CombatInput->bBlockInput = false;
    CombatInput->RegisterComponent();
    CombatInput->BindAction(Fire, ETriggerEvent::Started, this, &UCoastalCombatComponent::InputFire);
    CombatInput->BindAction(Reload, ETriggerEvent::Started, this, &UCoastalCombatComponent::InputReload);
    CombatInput->BindAction(Aim, ETriggerEvent::Triggered, this, &UCoastalCombatComponent::InputAim);
    CombatInput->BindAction(Aim, ETriggerEvent::Completed, this, &UCoastalCombatComponent::InputAimReleased);
    CombatInput->BindAction(Aim, ETriggerEvent::Canceled, this, &UCoastalCombatComponent::InputAimReleased);
    Controller->PushInputComponent(CombatInput);
    FModifyContextOptions Options;
    Options.bIgnoreAllPressedKeysUntilRelease = true;
    Subsystem->AddMappingContext(CombatContext, 5, Options);
    bInputInstalled = true;
    return true;
}

void UCoastalCombatComponent::RemoveInput()
{
    if (IsValid(Controller))
    {
        if (Controller->GetLocalPlayer())
            if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()))
                if (CombatContext) Subsystem->RemoveMappingContext(CombatContext);
        if (CombatInput) Controller->PopInputComponent(CombatInput);
    }
    if (CombatInput) CombatInput->DestroyComponent();
    CombatInput = nullptr; CombatContext = nullptr; CombatActions.Reset();
    bInputInstalled = false;
    bAimHeld = false; bAimNeedsRelease = false;
}

void UCoastalCombatComponent::InputFire() { TryFire(); }
void UCoastalCombatComponent::InputReload() { TryReload(); }
void UCoastalCombatComponent::InputAim()
{
    if (!bAimNeedsRelease && CanReceiveHostileAttack()) bAimHeld = true;
}
void UCoastalCombatComponent::InputAimReleased() { bAimHeld = false; bAimNeedsRelease = false; }
bool UCoastalCombatComponent::IsAiming() const
{ return bAimHeld && !bAimNeedsRelease && CanReceiveHostileAttack(); }
void UCoastalCombatComponent::InterruptAim()
{
    if (bAimHeld || (Controller && (Controller->IsInputKeyDown(EKeys::RightMouseButton)
        || Controller->IsInputKeyDown(EKeys::Gamepad_LeftTrigger)))) bAimNeedsRelease = true;
    bAimHeld = false;
}
