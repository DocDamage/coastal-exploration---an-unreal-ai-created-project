#include "CoastalSaveCoordinator.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

bool UCoastalSaveCoordinator::BeginPlayerReturn(ACharacter* Character, uint64 ExpectedEpoch)
{
    return IsInGameThread() && Ready() && HasActiveCampaign() && Player == Character
        && ExpectedEpoch == SessionEpoch && Gate.BeginRecovery();
}
bool UCoastalSaveCoordinator::RelocatePlayerDuringReturn(ACharacter* Character, uint64 ExpectedEpoch,
    const FTransform& Destination)
{
    if (!IsInGameThread() || !Gate.IsRecovery() || Gate.Poisoned() || !IsValid(Player)
        || Player != Character || ExpectedEpoch != SessionEpoch || !SafeDestination(Destination)) return false;
    if (!Player->TeleportTo(Destination.GetLocation(), Destination.Rotator(), false, false)) return false;
    Player->GetCharacterMovement()->StopMovementImmediately();
    // TeleportTo may adjust position to fit: validate the ACTUAL result, not only the request.
    return SafeDestination(Player->GetActorTransform());
}
bool UCoastalSaveCoordinator::FinishPlayerReturn(ACharacter* Character, uint64 ExpectedEpoch)
{
    if (!IsInGameThread() || !Gate.IsRecovery() || Gate.Poisoned() || !IsValid(Player)
        || Player != Character || ExpectedEpoch != SessionEpoch || !SafeDestination(Player->GetActorTransform())) return false;
    DryCheckpoint = Player->GetActorTransform();
    Gate.EndRecovery(); Gate.RequestSave(); return true;
}
void UCoastalSaveCoordinator::FailPlayerReturn(const FString& Detail)
{
    Gate.Poison();
    Notice(ECoastalSaveResult::RecoveryRequired, TEXT("Safe return stopped; exit and relaunch a validated save. ") + Detail);
}
