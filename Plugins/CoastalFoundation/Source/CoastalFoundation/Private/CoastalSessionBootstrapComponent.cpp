#include "CoastalSessionBootstrapComponent.h"
#include "CoastalMissionDirector.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "CoastalSwimmingComponent.h"
#include "CoastalInteractionRelayComponent.h"
#include "Templates/UnrealTemplate.h"
#include "CoastalUISessionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UCoastalSessionBootstrapComponent::UCoastalSessionBootstrapComponent()
{ PrimaryComponentTick.bCanEverTick = false; }
ECoastalStartupPhase UCoastalSessionBootstrapComponent::GetPhase() const
{
    switch (Gate.Phase())
    {
    case coastal::StartupPhase::Idle: return ECoastalStartupPhase::Idle;
    case coastal::StartupPhase::Checking: return ECoastalStartupPhase::Checking;
    case coastal::StartupPhase::Blocked: return ECoastalStartupPhase::Blocked;
    case coastal::StartupPhase::Binding: return ECoastalStartupPhase::Binding;
    case coastal::StartupPhase::Ready: return ECoastalStartupPhase::Ready;
    case coastal::StartupPhase::RestartRequired: return ECoastalStartupPhase::RestartRequired;
    default: return ECoastalStartupPhase::Stopped;
    }
}
ECoastalStartupResult UCoastalSessionBootstrapComponent::Publish(ECoastalStartupResult Result)
{
    const FString Text = LastReport.ToText();
    UE_LOG(LogTemp, Display, TEXT("Coastal startup: %d | %s"), static_cast<int32>(Result), *Text);
    if (GEngine && !LastReport.bPassed) GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()),
        Result == ECoastalStartupResult::RestartRequired ? 3600.0f : 30.0f, FColor::Red, Text);
    if (GEngine && LastReport.bPassed) GEngine->RemoveOnScreenDebugMessage(GetUniqueID());
    // A Blueprint notification cannot synchronously recurse into another startup attempt.
    TGuardValue<bool> Guard(bPublishing, true); OnStartupNotice.Broadcast(Result); return Result;
}
ECoastalStartupResult UCoastalSessionBootstrapComponent::BindingFailed(const TCHAR* Step, const FString& Detail)
{
    Gate.RequireRestart(); LastReport.Add(FName(Step), TEXT("startup binding"),
        Detail + TEXT(" Exit and relaunch; further campaign writes are blocked."));
    if (IsValid(BoundDirector) && IsValid(BoundDirector->Saves))
        BoundDirector->Saves->StopForIntegrationFailure(Detail);
    if (auto* PC = Cast<APlayerController>(GetOwner()))
    {
        LockedController = PC; PC->SetIgnoreMoveInput(true); PC->SetIgnoreLookInput(true);
    }
    return Publish(ECoastalStartupResult::RestartRequired);
}
ECoastalStartupResult UCoastalSessionBootstrapComponent::StartTestRoom(ACoastalMissionDirector* Director,
    UCoastalInventoryAdapter* Provider, FTransform Checkpoint)
{
    if (!IsInGameThread() || bPublishing) return ECoastalStartupResult::Busy;
    const auto Request = Gate.Begin();
    if (Request == coastal::StartupRequest::AlreadyReady)
    {
        auto* PC = Cast<APlayerController>(GetOwner());
        if (Director != BoundDirector || Provider != BoundProvider || !Checkpoint.Equals(BoundCheckpoint))
            return ECoastalStartupResult::Rejected; // Never silently rebind a different request.
        if (!IsValid(PC) || !IsValid(BoundDirector) || !IsValid(BoundDirector->Saves)
            || !IsValid(BoundCharacter) || PC->GetPawn() != BoundCharacter || !IsValid(BoundProvider) || !IsValid(BoundBridge)
            || !IsValid(BoundUI) || !BoundUI->IsInitialized()
            || (bHadRecovery && (!IsValid(BoundRecovery) || !BoundRecovery->IsInitialized()))
            || (bHadSwimming && (!IsValid(BoundSwimming) || !BoundSwimming->IsInitialized()))
            || (bHadRelay && (!IsValid(BoundRelay) || !BoundRelay->IsInitialized())) || !BoundDirector->Saves->IsConfigured()
            || BoundDirector->Saves->IsRecoveryRequired() || BoundBridge->GetCoordinator() != BoundDirector->Saves)
            return BindingFailed(TEXT("startup.lifetime_lost"), TEXT("The bound session changed or became unavailable. Exit and relaunch."));
        return ECoastalStartupResult::AlreadyStarted;
    }
    if (Request == coastal::StartupRequest::Busy) return ECoastalStartupResult::Busy;
    if (Request == coastal::StartupRequest::RestartRequired) return ECoastalStartupResult::RestartRequired;
    if (Request == coastal::StartupRequest::Stopped) return ECoastalStartupResult::Rejected;
    const bool bPassed = RunPreflight(Director, Provider, Checkpoint, LastReport);
    Gate.FinishChecks(bPassed);
    if (!bPassed) return Publish(ECoastalStartupResult::Blocked);
    auto* PC = CastChecked<APlayerController>(GetOwner());
    BoundDirector = Director; BoundProvider = Provider; BoundCheckpoint = Checkpoint;
    BoundCharacter = CastChecked<ACharacter>(PC->GetPawn());
    BoundBridge = BoundCharacter->FindComponentByClass<UCoastalInteractionBridge>();
    BoundUI = PC->FindComponentByClass<UCoastalUISessionComponent>();
    BoundRelay = PC->FindComponentByClass<UCoastalInteractionRelayComponent>(); bHadRelay = IsValid(BoundRelay);
    BoundRecovery = BoundCharacter->FindComponentByClass<UCoastalPlayerRecoveryComponent>(); bHadRecovery = IsValid(BoundRecovery);
    BoundSwimming = BoundCharacter->FindComponentByClass<UCoastalSwimmingComponent>(); bHadSwimming = IsValid(BoundSwimming);
    const FName MapId = GetWorld()->GetMapName().EndsWith(TEXT("L_FirstSignal"))
        ? TEXT("level.first_signal") : bInventoryCapacityFixture ? TEXT("level.systems_test.capacity") : TEXT("level.systems_test");
    if (!Director->Saves->Configure(Provider, BoundCharacter, MapId))
        return BindingFailed(TEXT("startup.configure_save"), TEXT("Save configuration failed: ") + Director->Saves->LastDetail);
    if (!BoundBridge->Configure(Director->Saves))
        return BindingFailed(TEXT("startup.configure_bridge"), TEXT("Interaction bridge refused its coordinator."));
    if (bHadRelay && !BoundRelay->InitializeRelay(BoundBridge))
        return BindingFailed(TEXT("startup.initialize_relay"), TEXT("Interaction relay initialization failed: ") + BoundRelay->LastDetail);
    if (bHadSwimming && !BoundSwimming->InitializeSwimming(BoundBridge))
        return BindingFailed(TEXT("startup.initialize_swimming"), TEXT("Surface-swimming initialization failed: ") + BoundSwimming->LastDetail);
    if (bHadRecovery && !BoundRecovery->InitializeRecovery(Director->Saves, BoundBridge, Checkpoint))
        return BindingFailed(TEXT("startup.initialize_recovery"), TEXT("Safe-return initialization failed: ") + BoundRecovery->LastDetail);
    if (!BoundUI->InitializeUI(Director->Saves, BoundBridge, Checkpoint))
        return BindingFailed(TEXT("startup.initialize_ui"), TEXT("Native menu/input initialization failed. No campaign was started."));
    Gate.FinishBinding(true); return Publish(ECoastalStartupResult::Started);
}
void UCoastalSessionBootstrapComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Gate.Stop();
    if (IsValid(LockedController))
    { LockedController->SetIgnoreMoveInput(false); LockedController->SetIgnoreLookInput(false); }
    LockedController = nullptr;
    if (GEngine) GEngine->RemoveOnScreenDebugMessage(GetUniqueID());
    Super::EndPlay(Reason);
}
