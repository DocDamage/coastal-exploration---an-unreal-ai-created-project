#include "CoastalPlayerRecoveryComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

bool UCoastalPlayerRecoveryComponent::InstallReturnInput()
{
    if (!IsValid(Controller) || ReturnInput || !Controller->GetLocalPlayer()
        || !Cast<UEnhancedPlayerInput>(Controller->PlayerInput)
        || !ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer())) return false;
    ReturnInput = NewObject<UEnhancedInputComponent>(Controller);
    if (!ReturnInput) return false;
    // Block lower-priority gameplay, including Jump callbacks that ignore move/look locks.
    // Existing Coastal menu input stays at 100; no keys/actions/contexts are replaced.
    ReturnInput->Priority = 90; ReturnInput->bBlockInput = true; ReturnInput->RegisterComponent();
    Controller->PushInputComponent(ReturnInput); return true;
}
void UCoastalPlayerRecoveryComponent::RemoveReturnInput()
{
    const bool bHadInput = IsValid(ReturnInput);
    if (IsValid(Player)) { Player->StopJumping(); Player->ConsumeMovementInputVector(); }
    if (IsValid(Controller) && bHadInput) Controller->PopInputComponent(ReturnInput);
    if (bHadInput) ReturnInput->DestroyComponent();
    ReturnInput = nullptr;
    if (bHadInput && IsValid(Controller) && Controller->GetLocalPlayer())
        if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Controller->GetLocalPlayer()))
        {
            FModifyContextOptions Options;
            Options.bIgnoreAllPressedKeysUntilRelease = true; Options.bForceImmediately = true;
            Subsystem->RequestRebuildControlMappings(Options, EInputMappingRebuildType::RebuildWithFlush);
        }
}
