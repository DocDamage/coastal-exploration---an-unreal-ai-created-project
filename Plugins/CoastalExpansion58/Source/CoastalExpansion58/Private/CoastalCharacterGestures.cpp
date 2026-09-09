#include "CoastalMutableCharacterComponent.h"
#include "CoastalCombatComponent.h"
#include "CoastalGrappleComponent.h"
#include "CoastalZiplineRiderComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Animation/AnimInstance.h"

bool UCoastalMutableCharacterComponent::CanPlayGesture() const
{
    const auto* Move = Character ? Character->GetCharacterMovement() : nullptr;
    const auto* Grapple = Character ? Character->FindComponentByClass<UCoastalGrappleComponent>() : nullptr;
    return BindingValid() && bReady && !bEditing && !bGenerating && Definition
        && !Character->ActorHasTag(TEXT("Coastal.Prone"))
        && Bridge->AllowsWorldInput() && !GetWorld()->IsPaused()
        && Move && Move->IsMovingOnGround() && Move->Velocity.SizeSquared2D() < 100.f
        && Move->GetCurrentAcceleration().IsNearlyZero()
        && Character->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationBlueprint
        && Character->GetMesh()->GetAnimInstance() && !Character->GetMesh()->GetAnimInstance()->IsAnyMontagePlaying()
        && (!Combat || (!Combat->IsEncounterActive() && !Combat->GetTransientWeapon()))
        && (!Grapple || !Grapple->IsGrappleDeployed());
}

bool UCoastalMutableCharacterComponent::PlayNextEmote()
{
    if (!CanPlayGesture() || Definition->Emotes.IsEmpty() || !QueuedAction.IsNone()) return false;
    // Gestures can replace each other, but cannot displace an accepted interaction/hit.
    if (!ActiveAction.IsNone() && !Definition->Emotes.Contains(ActiveAction)
        && !Definition->IdleVariations.Contains(ActiveAction)) return false;
    const FName Cue = Definition->Emotes[NextEmote % Definition->Emotes.Num()];
    if (!Definition->Actions.Contains(Cue)) return false;
    QueuedAction = Cue; QueuedUntil = GetWorld()->GetTimeSeconds() + 0.3;
    NextEmote = (NextEmote + 1) % Definition->Emotes.Num();
    NextIdleTime = GetWorld()->GetTimeSeconds() + 18.0;
    return true;
}

void UCoastalMutableCharacterComponent::InputEmote() { if (!bEmoteNeedsRelease) PlayNextEmote(); }

FText UCoastalMutableCharacterComponent::CharacterActionHint(bool Gamepad) const
{
    if (!bReady || !Definition) return FText::GetEmpty();
    FString Hint = Definition->Emotes.IsEmpty() ? TEXT("")
        : Gamepad ? TEXT("D-pad down: next emote") : TEXT("G: next emote");
    if (Character && Character->FindComponentByClass<UCoastalGrappleComponent>())
    {
        if (!Hint.IsEmpty()) Hint += TEXT(" | ");
        Hint += Gamepad ? TEXT("D-pad up: grapple/release | Right/left: reel in/out")
            : TEXT("Q: grapple/release | Z/X: reel in/out");
    }
    if (Character && Character->FindComponentByClass<UCoastalZiplineRiderComponent>())
    {
        if (!Hint.IsEmpty()) Hint += TEXT(" | ");
        Hint += Gamepad ? TEXT("Right stick click: zipline / release") : TEXT("V: zipline / release");
    }
    Hint += Gamepad ? TEXT(" | B: crawl / stand") : TEXT(" | C: crawl / stand");
    return FText::FromString(Hint);
}

void UCoastalMutableCharacterComponent::UpdateGestures()
{
    auto* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
    const bool Held = PC && (PC->IsInputKeyDown(EKeys::G) || PC->IsInputKeyDown(EKeys::Gamepad_DPad_Down));
    if (!Held) bEmoteNeedsRelease = false;
    else if (!CanPlayGesture()) bEmoteNeedsRelease = true;
    if (GestureInput && GestureController.Get() != PC) RemoveGestureInput();
    if (!GestureInput && PC && PC->GetLocalPlayer() && Cast<UEnhancedPlayerInput>(PC->PlayerInput))
    {
        auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
        if (Subsystem)
        {
            GestureController = PC;
            GestureContext = NewObject<UInputMappingContext>(this);
            GestureInput = NewObject<UEnhancedInputComponent>(PC);
            EmoteInputAction = NewObject<UInputAction>(this);
            EmoteInputAction->ValueType = EInputActionValueType::Boolean;
            EmoteInputAction->bConsumeInput = true;
            EmoteInputAction->bTriggerWhenPaused = false;
            GestureContext->MapKey(EmoteInputAction, EKeys::G);
            GestureContext->MapKey(EmoteInputAction, EKeys::Gamepad_DPad_Down);
            GestureInput->Priority = 4;
            GestureInput->RegisterComponent();
            GestureInput->BindAction(EmoteInputAction, ETriggerEvent::Started, this, &UCoastalMutableCharacterComponent::InputEmote);
            PC->PushInputComponent(GestureInput);
            FModifyContextOptions Options; Options.bIgnoreAllPressedKeysUntilRelease = true;
            Subsystem->AddMappingContext(GestureContext, 4, Options);
        }
    }
    const double Now = GetWorld()->GetTimeSeconds();
    if (!CanPlayGesture() || !ActiveAction.IsNone() || !QueuedAction.IsNone())
    {
        NextIdleTime = Now + 18.0;
        // Entering an encounter retires cosmetic gestures before equip starts.
        if (Combat && Combat->IsEncounterActive() && Definition
            && (Definition->Emotes.Contains(ActiveAction) || Definition->IdleVariations.Contains(ActiveAction))) StopAction(false);
        return;
    }
    if (NextIdleTime <= 0.0) NextIdleTime = Now + 18.0;
    if (Now >= NextIdleTime && !Definition->IdleVariations.IsEmpty())
    {
        const FName Cue = Definition->IdleVariations[NextIdleVariation % Definition->IdleVariations.Num()];
        NextIdleVariation = (NextIdleVariation + 1) % Definition->IdleVariations.Num();
        NextIdleTime = Now + 18.0;
        if (Definition->Actions.Contains(Cue)) { QueuedAction = Cue; QueuedUntil = Now + 0.3; }
    }
}

void UCoastalMutableCharacterComponent::RemoveGestureInput()
{
    if (auto* PC = GestureController.Get())
    {
        if (PC->GetLocalPlayer())
            if (auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
                if (GestureContext) Subsystem->RemoveMappingContext(GestureContext);
        if (GestureInput) PC->PopInputComponent(GestureInput);
    }
    if (GestureInput) GestureInput->DestroyComponent();
    GestureInput = nullptr; GestureContext = nullptr; EmoteInputAction = nullptr; GestureController.Reset();
}
