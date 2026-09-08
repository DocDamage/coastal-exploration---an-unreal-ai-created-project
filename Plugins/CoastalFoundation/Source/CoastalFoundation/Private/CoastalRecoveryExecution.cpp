#include "CoastalPlayerRecoveryComponent.h"
#include "CoastalSafetyVolume.h"
#include "CoastalPlacementLibrary.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSwimmingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
namespace { const FName ReturnBlocker(TEXT("coastal.safety.return")); }

bool UCoastalPlayerRecoveryComponent::RequestDefeatReturn()
{
    if (!IsInGameThread() || !IsInitialized() || Flow.Active()
        || !IsValid(Saves) || !IsValid(Player) || !IsValid(Controller) || !IsValid(Bridge)
        || Controller->GetPawn() != Player || Bridge->GetCoordinator() != Saves
        || !Saves->HasActiveCampaign() || Saves->IsBusy() || Saves->IsRecoveryRequired()
        || Saves->IsPlayerReturnActive() || Saves->GetSessionEpoch() != Epoch
        || !Bridge->AllowsWorldInput() || UGameplayStatics::IsGamePaused(this))
    {
        return false;
    }
    BeginReturn(TEXT("Defeated: returning to dry ground."));
    return Flow.Active();
}

void UCoastalPlayerRecoveryComponent::BeginReturn(const FString& Reason)
{
    Visit.Reset();
    if (IsValid(Swimming)) Swimming->CancelSwimmingForRecovery();
    if (!Saves->BeginPlayerReturn(Player, Epoch)) return; // No deferred teleport intention.
    if (!Flow.Begin(Epoch) || !Bridge->AcquireUIBlocker(ReturnBlocker))
    { FailReturn(TEXT("Unable to acquire exclusive safe-return ownership.")); return; }
    bBlockerOwned = true; bUsedFallback = false; Primary = Saves->GetDryCheckpoint();
    Bridge->CloseStorage(); Bridge->CancelTranscript();
    if (!InstallReturnInput()) { FailReturn(TEXT("Could not isolate gameplay input for safe return.")); return; }
    Controller->SetIgnoreMoveInput(true); Controller->SetIgnoreLookInput(true); bControlsOwned = true;
    Player->StopJumping(); Player->ConsumeMovementInputVector();
    Player->GetCharacterMovement()->StopMovementImmediately(); Player->GetCharacterMovement()->DisableMovement();
    // Preserve an already-owned cinematic fade: recovery still works without its own fade.
    auto* Camera = Controller->PlayerCameraManager.Get();
    if (bBoundFadeCamera && IsValid(Camera) && !Camera->bEnableFading)
    { FadeManager = Camera; bFadeOwned = true; Camera->SetManualCameraFade(0, FLinearColor::Black, false); }
    Publish(ECoastalReturnNotice::Returning, Reason);
}
bool UCoastalPlayerRecoveryComponent::PlacePlayer()
{
    // A maximum of two authored candidates. No random search, origin warp, or retry loop.
    const int Selected = coastal::TryReturnCandidates(!Primary.Equals(Fallback), [this](int Index)
        { return Saves->RelocatePlayerDuringReturn(Player, Epoch, Index == 0 ? Primary : Fallback); });
    bUsedFallback = Selected == 1;
    return Selected >= 0;
}
void UCoastalPlayerRecoveryComponent::AdvanceReturn(float DeltaTime)
{
    if (!Flow.Matches(Saves->GetSessionEpoch()) || !Saves->IsPlayerReturnActive())
    { FailReturn(TEXT("Campaign/session changed during safe return.")); return; }
    if (UGameplayStatics::IsGamePaused(this)) return;
    ACoastalSafetyVolume *Hazard = nullptr, *Checkpoint = nullptr;
    if (!InspectVolumes(Hazard, Checkpoint)) { FailReturn(TEXT("Safety geometry changed during return.")); return; }
    if (Flow.Phase() == coastal::ReturnPhase::FadingIn || Flow.Phase() == coastal::ReturnPhase::Settling)
    {
        if (Hazard || BelowBoundary(Player->GetActorLocation())
            || !UCoastalPlacementLibrary::IsDryDestination(Player, Player->GetActorTransform()))
        { FailReturn(TEXT("The return destination became unsafe. A repeated teleport loop was prevented.")); return; }
    }
    const auto PreviousPhase = Flow.Phase();
    if (!Flow.Advance(DeltaTime, Player->GetCharacterMovement()->IsMovingOnGround()))
    { FailReturn(TEXT("Invalid safe-return timer state.")); return; }
    if (Flow.Phase() == coastal::ReturnPhase::Failed)
    { FailReturn(TEXT("The character did not settle safely after returning. No repeated teleport was attempted.")); return; }
    if (Flow.Phase() == coastal::ReturnPhase::Placement)
    {
        if (!PlacePlayer() || !Flow.Placed())
        { FailReturn(TEXT("Last checkpoint and initial fallback are blocked or unsafe. Disk saves were preserved.")); return; }
    }
    if (Flow.Phase() == coastal::ReturnPhase::Settling && PreviousPhase != coastal::ReturnPhase::Settling)
    { Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking); Player->GetCharacterMovement()->StopMovementImmediately(); }
    if (bFadeOwned && FadeManager.IsValid())
        FadeManager->SetManualCameraFade(static_cast<float>(Flow.Opacity()), FLinearColor::Black, false);
    if (!Flow.CanFinish()) return;
    if (!Saves->FinishPlayerReturn(Player, Epoch))
    { FailReturn(TEXT("Final dry-ground validation failed. Save and interaction remain locked.")); return; }
    Flow.Finish(); ClearFade(); Player->ConsumeMovementInputVector(); ReleaseControls();
    Publish(ECoastalReturnNotice::Returned, bUsedFallback
        ? TEXT("Returned to the initial dry fallback; the last checkpoint was unavailable. Inventory/progress kept. Save queued.")
        : TEXT("Returned to dry ground. Inventory and progress kept. Campaign save queued."));
}
void UCoastalPlayerRecoveryComponent::ClearFade()
{
    if (bFadeOwned && FadeManager.IsValid()) FadeManager->StopCameraFade();
    bFadeOwned = false; FadeManager.Reset();
}
void UCoastalPlayerRecoveryComponent::ReleaseControls()
{
    RemoveReturnInput();
    if (bControlsOwned && IsValid(Controller))
    { Controller->SetIgnoreMoveInput(false); Controller->SetIgnoreLookInput(false); }
    bControlsOwned = false;
    if (bBlockerOwned && IsValid(Bridge)) Bridge->ReleaseUIBlocker(ReturnBlocker);
    bBlockerOwned = false;
}
void UCoastalPlayerRecoveryComponent::FailReturn(const FString& Detail)
{
    // The flow can already be Failed because of a settle timeout. Publish exactly once.
    if (bStopped || bFailurePublished) return;
    bFailurePublished = true;
    Flow.Fail(); Visit.Reset(); ClearFade();
    if (IsValid(Player) && Player->GetCharacterMovement())
    { Player->GetCharacterMovement()->StopMovementImmediately(); Player->GetCharacterMovement()->DisableMovement(); }
    if (!bControlsOwned && IsValid(Controller))
    { Controller->SetIgnoreMoveInput(true); Controller->SetIgnoreLookInput(true); bControlsOwned = true; }
    if (!bBlockerOwned && IsValid(Bridge)) bBlockerOwned = Bridge->AcquireUIBlocker(ReturnBlocker);
    if (IsValid(Saves) && !Saves->IsRecoveryRequired()) Saves->FailPlayerReturn(Detail);
    Publish(ECoastalReturnNotice::RestartRequired, TEXT("SAFE RETURN STOPPED: ") + Detail);
}
