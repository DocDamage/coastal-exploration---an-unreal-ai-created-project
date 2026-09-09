#include "CoastalCombatComponent.h"
#include "CoastalCombatActors.h"
#include "AdvancedShooterComponent.h"
#include "CoastalCampingActionComponent.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalSwimmingComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

UCoastalCombatComponent::UCoastalCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
    FireKeyboardKey = EKeys::LeftMouseButton;
    FireGamepadKey = EKeys::Gamepad_RightTrigger;
    ReloadKeyboardKey = EKeys::R;
    ReloadGamepadKey = EKeys::Gamepad_LeftShoulder;
    WeaponClass = ACoastalSidearmWeapon::StaticClass();
    WeaponVisualTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(.25f, .06f, .10f));
}

bool UCoastalCombatComponent::InitializeCombat(UCoastalInteractionBridge* Interaction,
    UCoastalSaveCoordinator* Coordinator, UCoastalPlayerRecoveryComponent* RecoveryOwner,
    UCoastalCampingActionComponent* CampingOwner, UCoastalSwimmingComponent* SwimmingOwner)
{
    Character = Cast<ACharacter>(GetOwner());
    Controller = IsValid(Character) ? Cast<APlayerController>(Character->GetController()) : nullptr;
    if (!IsInGameThread() || bInitialized || bStopped || !IsRegistered()
        || !IsValid(Character) || !IsValid(Controller) || !Controller->IsLocalController()
        || Controller->GetPawn() != Character || !GetWorld() || GetWorld()->GetNetMode() != NM_Standalone
        || !IsValid(Interaction) || Interaction->GetOwner() != Character
        || !IsValid(Coordinator) || !Coordinator->IsConfigured()
        || Interaction->GetCoordinator() != Coordinator || Coordinator->GetPlayerCharacter() != Character
        || !IsValid(RecoveryOwner) || RecoveryOwner->GetOwner() != Character || !RecoveryOwner->IsInitialized()
        || !IsValid(CampingOwner) || CampingOwner->GetOwner() != Character
        || !IsValid(SwimmingOwner) || SwimmingOwner->GetOwner() != Character
        || !WeaponClass || WeaponAttachSocket.IsNone() || !WeaponVisualAsset.ToSoftObjectPath().IsValid()
        || !TracerAsset.ToSoftObjectPath().IsValid() || !FMath::IsFinite(MaxHealth) || MaxHealth <= 0.f
        || !FMath::IsFinite(MaxShield) || MaxShield < 0.f)
    {
        LastDetail = TEXT("Combat initialization rejected incomplete standalone bindings or presentation assets.");
        return false;
    }
    TArray<UCoastalCombatComponent*> Owners; Character->GetComponents(Owners);
    TArray<UAdvancedShooterComponent*> Shooters; Character->GetComponents(Shooters);
    if (Owners.Num() != 1 || Owners[0] != this || Shooters.Num() != 1)
    {
        LastDetail = TEXT("Combat requires exactly one coastal and one vendor shooter component.");
        return false;
    }
    Shooter = Shooters[0]; Bridge = Interaction; Saves = Coordinator; Recovery = RecoveryOwner;
    Camping = CampingOwner; Swimming = SwimmingOwner;
    Shooter->WeaponAttachSocket = WeaponAttachSocket;
    EpochState.epoch = Saves->GetSessionEpoch();
    Health = MaxHealth; Shield = MaxShield;
    Character->OnTakeAnyDamage.AddUniqueDynamic(this, &UCoastalCombatComponent::HandleDamage);
    Recovery->OnReturnNotice.AddUniqueDynamic(this, &UCoastalCombatComponent::HandleReturn);
    bInitialized = true;
    if (!InstallInput())
    {
        bInitialized = false;
        Character->OnTakeAnyDamage.RemoveDynamic(this, &UCoastalCombatComponent::HandleDamage);
        Recovery->OnReturnNotice.RemoveDynamic(this, &UCoastalCombatComponent::HandleReturn);
        LastDetail = TEXT("Combat could not install its low-priority Enhanced Input owner.");
        return false;
    }
    LastDetail = TEXT("Transient standalone combat initialized; no inventory or save state was created.");
    PublishState(true);
    return true;
}

bool UCoastalCombatComponent::BindingValid() const
{
    return bInitialized && !bStopped && IsValid(Character) && IsValid(Controller)
        && Controller->GetPawn() == Character && IsValid(Shooter) && IsValid(Bridge)
        && IsValid(Saves) && Bridge->GetCoordinator() == Saves && Saves->GetPlayerCharacter() == Character
        && IsValid(Recovery) && IsValid(Camping) && IsValid(Swimming);
}

void UCoastalCombatComponent::ReleaseCombat()
{
    if (bStopped) return;
    bStopped = true;
    RemoveInput();
    DestroyWeapon();
    EncounterSources.Reset();
    if (IsValid(Character)) Character->OnTakeAnyDamage.RemoveDynamic(this, &UCoastalCombatComponent::HandleDamage);
    if (IsValid(Recovery)) Recovery->OnReturnNotice.RemoveDynamic(this, &UCoastalCombatComponent::HandleReturn);
    bInitialized = false;
    PublishState(true);
}

void UCoastalCombatComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseCombat();
    Super::EndPlay(Reason);
}

void UCoastalCombatComponent::EnterEncounter(AActor* Source)
{
    if (!BindingValid() || !IsValid(Source)) return;
    EncounterSources.Add(Source);
    if (Saves->HasActiveCampaign() && !bDefeated && !Saves->IsPlayerReturnActive()
        && !Recovery->IsReturning()) SpawnWeapon();
}

void UCoastalCombatComponent::LeaveEncounter(AActor* Source)
{
    EncounterSources.Remove(Source);
    if (EncounterSources.Num() == 0) DestroyWeapon();
}

bool UCoastalCombatComponent::IsEncounterActive() const
{
    for (const TWeakObjectPtr<AActor>& Source : EncounterSources)
        if (Source.IsValid()) return true;
    return false;
}

void UCoastalCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* Tick)
{
    Super::TickComponent(DeltaTime, TickType, Tick);
    if (!BindingValid()) return;
    if (bAimHeld && !CanReceiveHostileAttack())
    { bAimHeld = false; bAimNeedsRelease = true; }
    if (bAimNeedsRelease && !Controller->IsInputKeyDown(EKeys::RightMouseButton)
        && !Controller->IsInputKeyDown(EKeys::Gamepad_LeftTrigger)) bAimNeedsRelease = false;
    for (auto It = EncounterSources.CreateIterator(); It; ++It)
        if (!It->IsValid()) It.RemoveCurrent();
    const uint64 CurrentEpoch = Saves->GetSessionEpoch();
    if (EpochState.Observe(CurrentEpoch))
    {
        DestroyWeapon();
        EncounterSources.Reset();
        for (TActorIterator<ACoastalCombatEncounterVolume> It(GetWorld()); It; ++It)
            if (It->IsAuthoredCorrectly() && It->Bounds->IsOverlappingActor(Character)) EncounterSources.Add(*It);
        bDefeated = false; Health = MaxHealth; Shield = MaxShield;
        ReloadUntil = -1.0; LastAcceptedShotTime = -1000.0;
        if (Saves->HasActiveCampaign() && IsEncounterActive() && !Saves->IsPlayerReturnActive()
            && !Recovery->IsReturning()) SpawnWeapon();
        LastDetail = TEXT("Campaign epoch changed; transient combat state reset.");
        PublishState(true);
    }
    if (!Saves->HasActiveCampaign() || Saves->IsRecoveryRequired()
        || Saves->IsPlayerReturnActive() || Recovery->IsReturning()) DestroyWeapon();
    else if (IsEncounterActive() && !bDefeated && !IsValid(Weapon)) SpawnWeapon();
    RefreshWeaponAttachment();
    PublishState();
}

void UCoastalCombatComponent::HandleReturn(ECoastalReturnNotice Result, FString)
{
    if (Result == ECoastalReturnNotice::Returning || Result == ECoastalReturnNotice::RestartRequired)
    {
        DestroyWeapon();
        return;
    }
    if (Result == ECoastalReturnNotice::Returned)
    {
        bDefeated = false; Health = MaxHealth; Shield = MaxShield;
        ReloadUntil = -1.0; LastAcceptedShotTime = -1000.0;
        if (IsEncounterActive()) SpawnWeapon();
        PublishState(true);
    }
}

void UCoastalCombatComponent::HandleDamage(AActor* DamagedActor, float Damage,
    const UDamageType*, AController* InstigatedBy, AActor* DamageCauser)
{
    if (DamagedActor != Character || !CanReceiveHostileAttack()
        || !FMath::IsFinite(Damage) || Damage <= 0.f) return;
    coastal::DamageState State{Health, Shield, bDefeated};
    State = coastal::ApplyDamage(State, Damage);
    Health = State.health; Shield = State.shield; bDefeated = State.defeated;
    PublishState(true);
    if (!bDefeated)
    {
        // Preserve the accepted-hit audio cue; its location now carries the source
        // bearing for presentation. Missing or self damage falls back to front.
        const AActor* Source = IsValid(DamageCauser) && DamageCauser != Character ? DamageCauser
            : IsValid(InstigatedBy) && InstigatedBy->GetPawn() != Character ? InstigatedBy->GetPawn() : nullptr;
        OnCombatPresentation.Broadcast(TEXT("player_hit"), Source ? Source->GetActorLocation() : Character->GetActorLocation());
    }
    if (!bDefeated) return;
    DestroyWeapon();
    if (!Recovery->RequestDefeatReturn())
    {
        LastDetail = TEXT("Defeat return was rejected; combat remains safely disabled.");
        OnCombatNotice.Broadcast(ECoastalCombatResult::Failed, LastDetail);
    }
}
