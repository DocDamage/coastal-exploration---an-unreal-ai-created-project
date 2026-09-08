#include "CoastalSessionBootstrapComponent.h"
#include "CoastalIntegrationLibrary.h"
#include "CoastalPlacementLibrary.h"
#include "CoastalMissionDirector.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "CoastalSwimmingComponent.h"
#include "CoastalSwimmingZone.h"
#include "CoastalSafetyVolume.h"
#include "GameFramework/WorldSettings.h"
#include "CoastalInteractionRelayComponent.h"
#include "CoastalUISessionComponent.h"
#include "EnhancedPlayerInput.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "FirstSignalComponent.h"

bool UCoastalSessionBootstrapComponent::RunPreflight(ACoastalMissionDirector* Director,
    UCoastalInventoryAdapter* Provider, FTransform Checkpoint, FCoastalIntegrationReport& Report) const
{
    Report = {};
    auto Fail = [&Report](const TCHAR* Code, const TCHAR* Detail)
    { Report.Add(FName(Code), TEXT("startup"), Detail); };
    auto* PC = Cast<APlayerController>(GetOwner());
    if (!IsInGameThread() || !GetWorld() || !GetWorld()->IsGameWorld() || GetWorld()->GetNetMode() != NM_Standalone
        || !IsValid(PC) || !PC->IsLocalController() || !PC->GetLocalPlayer())
    { Fail(TEXT("startup.local_owner"), TEXT("Use one local PlayerController in standalone single-player PIE/game, not editor preview or network play.")); return false; }
    if (Gate.Phase() != coastal::StartupPhase::Idle && Gate.Phase() != coastal::StartupPhase::Blocked
        && Gate.Phase() != coastal::StartupPhase::Checking)
    { Fail(TEXT("startup.already_used"), TEXT("Preflight is for an unbound startup, not a running or partly bound campaign.")); return false; }
    auto* Character = Cast<ACharacter>(PC->GetPawn());
    if (!IsValid(Character) || Character->GetController() != PC || Character->GetWorld() != GetWorld())
    { Fail(TEXT("startup.pawn"), TEXT("The local controller must possess the actual Third Person character first.")); return false; }
    int32 Controllers = 0;
    for (TActorIterator<APlayerController> It(GetWorld()); It; ++It) ++Controllers;
    if (Controllers != 1) Fail(TEXT("startup.controller_count"), TEXT("This M1 shell supports exactly one PlayerController."));
    TArray<UCoastalSessionBootstrapComponent*> Bootstraps; PC->GetComponents(Bootstraps);
    TArray<UCoastalUISessionComponent*> UIs; PC->GetComponents(UIs);
    TArray<UCoastalInteractionBridge*> Bridges; Character->GetComponents(Bridges);
    TArray<UCoastalInteractionRelayComponent*> Relays; PC->GetComponents(Relays);
    TArray<UCoastalPlayerRecoveryComponent*> Recoveries; Character->GetComponents(Recoveries);
    TArray<UCoastalSwimmingComponent*> Swimmers; Character->GetComponents(Swimmers);
    if (Swimmers.Num() > 1 || (Swimmers.Num() == 1 && (!Swimmers[0]->IsRegistered()
        || Swimmers[0]->IsInitialized() || !Swimmers[0]->SettingsValid())))
        Fail(TEXT("startup.surface_swimming"), TEXT("Use at most one registered, uninitialized character swimming component with finite settings."));
    if (Recoveries.Num() > 1 || (Recoveries.Num() == 1 && (!Recoveries[0]->IsRegistered()
        || Recoveries[0]->IsInitialized() || !Recoveries[0]->SettingsValid())))
        Fail(TEXT("startup.safe_return"), TEXT("Use zero or one registered, uninitialized character recovery component with finite settings."));
    if (Recoveries.Num() == 1 && Recoveries[0]->bEnableFallBoundary
        && GetWorld()->GetWorldSettings()->bEnableWorldBoundsChecks
        && Recoveries[0]->FallBoundaryZ <= GetWorld()->GetWorldSettings()->KillZ + 500.0f)
        Fail(TEXT("startup.return_boundary"), TEXT("Safe-return boundary must be at least 500 cm above KillZ; test actual fall speeds and hitches."));
    for (TActorIterator<ACoastalSafetyVolume> It(GetWorld()); It; ++It)
    {
        if (Recoveries.Num() != 1 || !It->IsAuthoredCorrectly())
            Fail(TEXT("startup.safety_volume"), TEXT("Safety volumes require valid upright boxes and one character recovery owner."));
        else if (It->Kind == ECoastalSafetyKind::DryCheckpoint
            && !UCoastalPlacementLibrary::IsDryDestination(Character, It->Destination()))
            Fail(TEXT("startup.return_point"), TEXT("Each dry checkpoint ReturnPoint must be a valid dry capsule-center destination."));
    }
    for (TActorIterator<ACoastalSwimmingZone> It(GetWorld()); It; ++It)
    {
        if (Swimmers.Num() != 1 || !It->IsAuthoredCorrectly())
            Fail(TEXT("startup.swimming_zone"), TEXT("Each swimming zone requires one character swimming owner and a valid aligned water brush."));
    }
    // Zero preserves explicitly wired M1.3 hosts. A present relay must be owned exactly once.
    if (Relays.Num() > 1 || (Relays.Num() == 1 && (!Relays[0]->IsRegistered() || Relays[0]->IsInitialized()
        || (Relays[0]->InputOwner != ECoastalInteractInputOwner::VendorEvents
            && Relays[0]->InputOwner != ECoastalInteractInputOwner::NativeEnhancedInput))))
        Fail(TEXT("startup.interaction_relay"), TEXT("Use at most one registered, uninitialized interaction relay with a valid input owner."));
    if (!IsRegistered() || Bootstraps.Num() != 1 || Bootstraps[0] != this)
        Fail(TEXT("startup.bootstrap_count"), TEXT("Keep exactly one bootstrap component on this controller."));
    if (UIs.Num() != 1 || !UIs[0]->IsRegistered() || UIs[0]->IsInitialized())
        Fail(TEXT("startup.ui_owner"), TEXT("Keep one registered, uninitialized CoastalUISessionComponent on this controller."));
    if (Bridges.Num() != 1 || !Bridges[0]->IsRegistered() || Bridges[0]->GetCoordinator())
        Fail(TEXT("startup.bridge_owner"), TEXT("Keep one registered, unbound interaction bridge on the possessed character."));
    if (!Cast<UEnhancedPlayerInput>(PC->PlayerInput)
        || !ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        Fail(TEXT("startup.enhanced_input"), TEXT("EnhancedPlayerInput and its local-player subsystem must exist before menu initialization."));
    if (!IsValid(Director) || Director->GetWorld() != GetWorld() || !IsValid(Director->Saves)
        || Director->Saves->HasConfiguration() || Director->Saves->IsRecoveryRequired())
        Fail(TEXT("startup.director"), TEXT("Supply the single same-world, unbound native director; do not mix legacy Configure calls with bootstrap."));
    else
    {
        TArray<UCoastalSaveCoordinator*> Coordinators; Director->GetComponents(Coordinators);
        TArray<UFirstSignalComponent*> Missions; Director->GetComponents(Missions);
        if (Coordinators.Num() != 1 || Missions.Num() != 1 || Coordinators[0] != Director->Saves
            || !Director->Saves->IsRegistered() || !Missions[0]->IsRegistered())
            Fail(TEXT("startup.director_components"), TEXT("Director must own exactly its one save coordinator and one mission component."));
    }
    if (!IsValid(Provider) || Provider->GetWorld() != GetWorld() || !Provider->IsRegistered())
        Fail(TEXT("startup.provider_world"), TEXT("Supply the registered adapter instance in this play world, not a class default or editor actor."));
    if (!UCoastalPlacementLibrary::IsDryDestination(Character, Checkpoint))
        Fail(TEXT("startup.checkpoint"), TEXT("Initial checkpoint must be finite/unit-scale, unobstructed and over a valid dry walkable floor."));
    if (!UCoastalPlacementLibrary::IsDryDestination(Character, Character->GetActorTransform()))
        Fail(TEXT("startup.current_position"), TEXT("Wait until the actual character has a valid dry-floor spawn before starting."));
    FCoastalIntegrationReport Room;
    if (bInventoryCapacityFixture && !GetWorld()->GetMapName().EndsWith(TEXT("L_SystemsTest_Capacity")))
        Fail(TEXT("startup.capacity_map"), TEXT("Capacity profile requires its separate authored acceptance map."));
    UCoastalIntegrationLibrary::AuditTestRoom(this, Room, bInventoryCapacityFixture); Report.Issues.Append(Room.Issues);
    // Do not call any real provider method when structural ownership checks have already failed.
    if (Report.Issues.IsEmpty())
    {
        FCoastalIntegrationReport Inventory;
        UCoastalIntegrationLibrary::AuditProvisionalProvider(Provider, Inventory); Report.Issues.Append(Inventory.Issues);
    }
    Report.Finish(); return Report.bPassed;
}
