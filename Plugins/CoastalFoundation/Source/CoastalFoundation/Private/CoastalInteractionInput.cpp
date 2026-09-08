#include "CoastalInteractionRelayComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"

bool UCoastalInteractionRelayComponent::InstallNativeInput()
{
    auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer());
    if (!Subsystem || !Cast<UEnhancedPlayerInput>(Controller->PlayerInput))
    { LastDetail = TEXT("Native interaction requires the real Enhanced Input local-player setup."); return false; }
    InteractContext = NewObject<UInputMappingContext>(this);
    InteractAction = NewObject<UInputAction>(this);
    InteractInput = NewObject<UEnhancedInputComponent>(Controller);
    if (!InteractContext || !InteractAction || !InteractInput)
    { LastDetail = TEXT("Interaction input allocation failed."); return false; }
    InteractAction->ValueType = EInputActionValueType::Boolean;
    InteractAction->bConsumeInput = true; InteractAction->bTriggerWhenPaused = false;
    InteractContext->MapKey(InteractAction, EKeys::E);
    InteractContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
    InteractInput->RegisterComponent(); InteractInput->Priority = 20; InteractInput->bBlockInput = false;
    InteractInput->BindAction(InteractAction, ETriggerEvent::Started, this, &UCoastalInteractionRelayComponent::NativePressed);
    InteractInput->BindAction(InteractAction, ETriggerEvent::Completed, this, &UCoastalInteractionRelayComponent::NativeReleased);
    InteractInput->BindAction(InteractAction, ETriggerEvent::Canceled, this, &UCoastalInteractionRelayComponent::NativeCanceled);
    Controller->PushInputComponent(InteractInput);
    FModifyContextOptions Options; Options.bIgnoreAllPressedKeysUntilRelease = true;
    Subsystem->AddMappingContext(InteractContext, 20, Options);
    return true;
}
void UCoastalInteractionRelayComponent::RemoveNativeInput()
{
    if (IsValid(Controller))
    {
        if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()))
            if (InteractContext) Subsystem->RemoveMappingContext(InteractContext);
        if (InteractInput) Controller->PopInputComponent(InteractInput);
    }
    if (InteractInput) InteractInput->DestroyComponent();
    InteractInput = nullptr; InteractAction = nullptr; InteractContext = nullptr;
}
bool UCoastalInteractionRelayComponent::NativeKeysDown() const
{
    return IsValid(Controller) && (Controller->IsInputKeyDown(EKeys::E)
        || Controller->IsInputKeyDown(EKeys::Gamepad_FaceButton_Left));
}
void UCoastalInteractionRelayComponent::NativePressed() { Press(); }
void UCoastalInteractionRelayComponent::NativeReleased()
{
    Synchronize();
    if (!NativeKeysDown()) Intent.Released();
}
void UCoastalInteractionRelayComponent::NativeCanceled() { Intent.Cancel(); }
