#include "CoastalCombatDirector.h"
#include "CoastalCombatActors.h"
#include "CoastalCombatComponent.h"
#include "CoastalCombatAudioComponent.h"
#include "CoastalCombatOverlay.h"
#include "AdvancedShooterComponent.h"
#include "CoastalCampingActionComponent.h"
#include "CoastalInteractionBridge.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalUISessionComponent.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalSwimmingComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ACoastalCombatDirector::ACoastalCombatDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
    WeaponClass = ACoastalSidearmWeapon::StaticClass();
    WeaponVisualTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(.25f, .06f, .10f));
}

void ACoastalCombatDirector::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() != NM_Standalone) { LastDetail = TEXT("Coastal combat is standalone-only."); SetActorTickEnabled(false); }
}

bool ACoastalCombatDirector::IsInitialized() const
{
    return IsValid(Combat) && Combat->IsInitialized();
}

void ACoastalCombatDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // Stay at a low cadence so player replacement can receive a fresh transient owner.
    if (IsInitialized()) return;
    TryInitialize();
}

bool ACoastalCombatDirector::TryInitialize()
{
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    APlayerController* PC = IsValid(Player) ? Cast<APlayerController>(Player->GetController()) : nullptr;
    if (!IsValid(Player) || !IsValid(PC) || !PC->IsLocalController()) return false;
    auto* Bridge = Player->FindComponentByClass<UCoastalInteractionBridge>();
    auto* Saves = IsValid(Bridge) ? Bridge->GetCoordinator() : nullptr;
    auto* Recovery = Player->FindComponentByClass<UCoastalPlayerRecoveryComponent>();
    auto* Camping = Player->FindComponentByClass<UCoastalCampingActionComponent>();
    auto* Swimming = Player->FindComponentByClass<UCoastalSwimmingComponent>();
    if (!IsValid(Bridge) || !IsValid(Saves) || !Saves->IsConfigured()
        || !IsValid(Recovery) || !Recovery->IsInitialized() || !IsValid(Camping) || !IsValid(Swimming)) return false;

    TArray<UAdvancedShooterComponent*> Shooters; Player->GetComponents(Shooters);
    if (Shooters.Num() > 1) { LastDetail = TEXT("Multiple vendor shooter owners found."); return false; }
    if (Shooters.IsEmpty())
    {
        auto* Shooter = NewObject<UAdvancedShooterComponent>(Player, TEXT("CoastalVendorShooter"));
        if (!Shooter) return false;
        Player->AddInstanceComponent(Shooter);
        Shooter->RegisterComponent();
        bCreatedShooter = true;
    }
    TArray<UCoastalCombatComponent*> Components; Player->GetComponents(Components);
    if (Components.Num() > 1) { LastDetail = TEXT("Multiple coastal combat owners found."); return false; }
    if (Components.IsEmpty())
    {
        Combat = NewObject<UCoastalCombatComponent>(Player, TEXT("CoastalCombat"));
        if (!Combat) return false;
        Player->AddInstanceComponent(Combat);
        Combat->RegisterComponent();
        bCreatedCombat = true;
    }
    else Combat = Components[0];

    Combat->WeaponClass = WeaponClass;
    Combat->WeaponAttachSocket = WeaponAttachSocket;
    Combat->WeaponVisualAsset = WeaponVisualAsset;
    Combat->WeaponVisualTransform = WeaponVisualTransform;
    Combat->MuzzleOffset = MuzzleOffset;
    Combat->TracerAsset = TracerAsset;
    Combat->TracerMaterial = TracerMaterial;
    Combat->MaxHealth = MaxHealth; Combat->MaxShield = MaxShield;
    if (!Combat->InitializeCombat(Bridge, Saves, Recovery, Camping, Swimming))
    { LastDetail = Combat->LastDetail; return false; }

    TArray<UCoastalCombatAudioComponent*> AudioOwners; PC->GetComponents(AudioOwners);
    if (AudioOwners.Num() <= 1)
    {
        if (AudioOwners.IsEmpty())
        {
            CombatAudio = NewObject<UCoastalCombatAudioComponent>(PC, TEXT("CoastalCombatAudio"));
            if (CombatAudio) { PC->AddInstanceComponent(CombatAudio); CombatAudio->RegisterComponent(); bCreatedCombatAudio = true; }
        }
        else CombatAudio = AudioOwners[0];
        auto* UI = PC->FindComponentByClass<UCoastalUISessionComponent>();
        auto* Routing = PC->FindComponentByClass<UCoastalAudioOptionsComponent>();
        if (IsValid(CombatAudio)) CombatAudio->InitializeAudio(Combat, UI, Routing);
    }
    for (TActorIterator<ACoastalCombatEncounterVolume> It(GetWorld()); It; ++It)
        if (It->IsAuthoredCorrectly() && It->Bounds->IsOverlappingActor(Player)) Combat->EnterEncounter(*It);
    if (bShowCombatHUD)
    {
        Overlay = CreateWidget<UCoastalCombatOverlay>(PC, UCoastalCombatOverlay::StaticClass());
        if (!IsValid(Overlay) || !Overlay->BindCombat(Combat))
        { LastDetail = TEXT("Combat initialized but its project-owned HUD could not bind."); return false; }
        Overlay->AddToPlayerScreen(5);
    }
    LastDetail = TEXT("Coastal combat initialized on the existing local player.");
    return true;
}

void ACoastalCombatDirector::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsValid(Overlay)) Overlay->RemoveFromParent();
    if (IsValid(CombatAudio)) CombatAudio->ReleaseAudio();
    if (IsValid(Combat)) Combat->ReleaseCombat();
    if (bCreatedCombat && IsValid(Combat)) Combat->DestroyComponent();
    if (bCreatedCombatAudio && IsValid(CombatAudio)) CombatAudio->DestroyComponent();
    if (bCreatedShooter)
        if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0))
            if (auto* Shooter = Player->FindComponentByClass<UAdvancedShooterComponent>()) Shooter->DestroyComponent();
    Combat = nullptr; CombatAudio = nullptr; Overlay = nullptr;
    Super::EndPlay(Reason);
}
