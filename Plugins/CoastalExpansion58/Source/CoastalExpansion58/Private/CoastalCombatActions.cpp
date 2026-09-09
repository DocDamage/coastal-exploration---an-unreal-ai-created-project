#include "CoastalCombatComponent.h"
#include "CoastalCombatActors.h"
#include "AdvancedShooterComponent.h"
#include "CoastalCampingActionComponent.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalSwimmingComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

bool UCoastalCombatComponent::HasForeignMontage() const
{
    UAnimInstance* Anim = IsValid(Character) && Character->GetMesh()
        ? Character->GetMesh()->GetAnimInstance() : nullptr;
    UAnimMontage* Active = IsValid(Anim) ? Anim->GetCurrentActiveMontage() : nullptr;
    if (!Active || !IsValid(Weapon)) return Active != nullptr;
    return Active != Weapon->GetFireMontage() && Active != Weapon->GetReloadMontage();
}

coastal::CombatGateSample UCoastalCombatComponent::Sample() const
{
    coastal::CombatGateSample S;
    S.configured = BindingValid() && (!IsEncounterActive() || IsValid(Weapon));
    S.activeCampaign = IsValid(Saves) && Saves->HasActiveCampaign();
    S.encounter = IsEncounterActive();
    S.worldInput = IsValid(Bridge) && Bridge->AllowsWorldInput();
    S.saveBusy = !IsValid(Saves) || Saves->IsBusy();
    S.recoveryRequired = !IsValid(Saves) || Saves->IsRecoveryRequired();
    S.returning = (IsValid(Recovery) && Recovery->IsReturning())
        || (IsValid(Saves) && Saves->IsPlayerReturnActive());
    S.camping = IsValid(Camping) && Camping->IsActionActive();
    S.swimming = IsValid(Swimming) && Swimming->IsSurfaceSwimming();
    S.paused = !GetWorld() || UGameplayStatics::IsGamePaused(this);
    S.foreignAnimation = HasForeignMontage();
    S.epochMatches = IsValid(Saves) && EpochState.epoch == Saves->GetSessionEpoch();
    S.defeated = bDefeated || Health <= 0.f;
    const double Now = UGameplayStatics::GetTimeSeconds(this);
    S.reloading = Now < ReloadUntil;
    S.clip = GetCurrentAmmo(); S.reserve = GetReserveAmmo();
    if (IsValid(Weapon)) S.clipCapacity = Weapon->GetWeaponConfig().MaxClipAmmo;
    const float Interval = IsValid(Weapon) ? Weapon->GetWeaponConfig().TimeBetweenShots : 0.f;
    // The vendor's fire-rate admission uses real time, independent of time dilation.
    S.cooldownReady = UGameplayStatics::GetRealTimeSeconds(this) - LastAcceptedShotTime
        >= FMath::Max(0.01f, Interval);
    return S;
}

bool UCoastalCombatComponent::CanAttack() const
{
    return coastal::FireDecision(Sample()) == coastal::CombatDecision::Allowed;
}

bool UCoastalCombatComponent::CanReceiveHostileAttack() const
{
    return coastal::BaseCombatDecision(Sample()) == coastal::CombatDecision::Allowed;
}

bool UCoastalCombatComponent::SpawnWeapon()
{
    if (IsValid(Weapon)) return true;
    if (!BindingValid() || !Character->HasAuthority() || !Saves->HasActiveCampaign()
        || !IsEncounterActive() || bDefeated || Saves->IsBusy() || Saves->IsRecoveryRequired()
        || Saves->IsPlayerReturnActive() || Recovery->IsReturning() || !Character->GetMesh()
        || !Character->GetMesh()->DoesSocketExist(WeaponAttachSocket))
    {
        LastDetail = TEXT("Transient weapon spawn rejected invalid authority, encounter, or hand socket.");
        return false;
    }
    LoadedWeaponVisual = WeaponVisualAsset.LoadSynchronous();
    LoadedTracerMesh = TracerAsset.LoadSynchronous();
    LoadedTracerMaterial = TracerMaterial.LoadSynchronous();
    if (!IsValid(LoadedWeaponVisual) || !IsValid(LoadedTracerMesh))
    {
        LastDetail = TEXT("Combat weapon and tracer require real loaded mesh assets.");
        return false;
    }
    const FTransform SpawnTransform(Character->GetActorTransform());
    AWeaponBase* Spawned = GetWorld()->SpawnActorDeferred<AWeaponBase>(WeaponClass,
        SpawnTransform, Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!IsValid(Spawned)) return false;
    ACoastalSidearmWeapon* CoastalWeapon = Cast<ACoastalSidearmWeapon>(Spawned);
    if (!CoastalWeapon)
    {
        Spawned->Destroy();
        LastDetail = TEXT("Configured weapon must expose the coastal visual and Muzzle anchor contract.");
        return false;
    }
    CoastalWeapon->ConfigurePresentation(LoadedWeaponVisual, WeaponVisualTransform, MuzzleOffset);
    UGameplayStatics::FinishSpawningActor(Spawned, SpawnTransform);
    const FWeaponConfig Config = Spawned->GetWeaponConfig();
    if (!CoastalWeapon->IsPresentationReady() || Config.MaxClipAmmo <= 0 || Config.MaxTotalAmmo < 0
        || !FMath::IsFinite(Config.BaseDamage) || Config.BaseDamage <= 0.f
        || !FMath::IsFinite(Config.TimeBetweenShots) || Config.TimeBetweenShots < 0.05f)
    {
        Spawned->Destroy();
        LastDetail = TEXT("Weapon mesh, Muzzle anchor, or authoritative weapon config is invalid.");
        return false;
    }
    Weapon = Spawned;
    Shooter->WeaponAttachSocket = WeaponAttachSocket;
    Shooter->EquipWeapon(Weapon);
    ReloadUntil = -1.0; LastAcceptedShotTime = -1000.0;
    LastDetail = TEXT("Transient encounter weapon equipped.");
    PublishState(true);
    return true;
}

void UCoastalCombatComponent::DestroyWeapon()
{
    if (!IsValid(Weapon)) { Weapon = nullptr; return; }
    if (IsValid(Shooter) && Character && Character->HasAuthority()) Shooter->UnequipWeapon();
    Weapon->Destroy();
    Weapon = nullptr; ReloadUntil = -1.0; LastAcceptedShotTime = -1000.0;
    PublishState(true);
}

int32 UCoastalCombatComponent::GetCurrentAmmo() const
{
    return IsValid(Weapon) ? Weapon->GetCurrentAmmo() : 0;
}

int32 UCoastalCombatComponent::GetReserveAmmo() const
{
    return IsValid(Weapon) ? Weapon->GetTotalCarriedAmmo() : 0;
}

ECoastalCombatResult UCoastalCombatComponent::TryFire()
{
    // Vendor damage/ammo delegates may call gameplay synchronously during a shot.
    if (bSubmittingShot) return ECoastalCombatResult::Busy;
    const auto Decision = coastal::FireDecision(Sample());
    if (Decision != coastal::CombatDecision::Allowed)
        return Emit(Decision, TEXT("Fire rejected by the current combat lifecycle gate."));
    TGuardValue<bool> SubmittingShot(bSubmittingShot, true);
    AWeaponBase* const FiredWeapon = Weapon;
    UAdvancedShooterComponent* const FiredShooter = Shooter;
    const uint64 FiredEpoch = EpochState.epoch;
    const int32 Before = GetCurrentAmmo();
    int32 Receipts = 0;
    FVector ShotStart = FVector::ZeroVector, ShotEnd = FVector::ZeroVector;
    double ShotTime = -1000.0;
    // Standalone RPC execution is synchronous. Scope this receipt to exactly this call;
    // rejected shots, later callbacks and another weapon cannot reuse an old trace.
    const FDelegateHandle Receipt = FiredShooter->OnAuthoritativeShot.AddLambda(
        [&](AWeaponBase* Source, const FVector& Start, const FVector& End, double Time)
        {
            if (Source != FiredWeapon) return;
            ++Receipts; ShotStart = Start; ShotEnd = End; ShotTime = Time;
        });
    FiredShooter->LocalFirePressed();
    FiredShooter->OnAuthoritativeShot.Remove(Receipt);
    if (!BindingValid() || Weapon != FiredWeapon || !IsValid(FiredWeapon)
        || Saves->GetSessionEpoch() != FiredEpoch || Receipts != 1
        || !coastal::AcceptedAuthoritativeShot(Before, GetCurrentAmmo())
        || ShotStart.ContainsNaN() || ShotEnd.ContainsNaN() || !FMath::IsFinite(ShotTime))
        return Emit(ECoastalCombatResult::Rejected, TEXT("No intact authoritative shot receipt; tracer was not predicted."));
    LastAcceptedShotTime = ShotTime;
    SpawnTracer(ShotStart, ShotEnd);
    PublishState(true);
    return Emit(ECoastalCombatResult::Applied, TEXT("Authoritative shot applied."));
}

ECoastalCombatResult UCoastalCombatComponent::TryReload()
{
    const auto Decision = coastal::ReloadDecision(Sample());
    if (Decision != coastal::CombatDecision::Allowed)
        return Emit(Decision, TEXT("Reload rejected by the current combat lifecycle gate."));
    const UAnimMontage* Montage = Weapon->GetReloadMontage();
    const float Duration = Montage ? Montage->GetPlayLength() : 2.f;
    ReloadUntil = UGameplayStatics::GetTimeSeconds(this) + FMath::Max(0.1f, Duration);
    Shooter->LocalReloadPressed();
    return Emit(ECoastalCombatResult::Applied, TEXT("Authoritative reload started; HUD awaits actual ammo values."));
}

void UCoastalCombatComponent::SpawnTracer(const FVector& Start, const FVector& End)
{
    if (!IsValid(Weapon)) return;
    auto* Tracer = GetWorld()->SpawnActor<ACoastalCombatTracer>(Start, FRotator::ZeroRotator);
    if (!IsValid(Tracer) || !Tracer->Configure(LoadedTracerMesh, LoadedTracerMaterial,
        Start, End, TracerRadiusCm, 0.08f))
        if (IsValid(Tracer)) Tracer->Destroy();
}

void UCoastalCombatComponent::PublishState(bool Force)
{
    const int32 Clip = GetCurrentAmmo(), Reserve = GetReserveAmmo();
    if (!Force && Clip == LastPublishedClip && Reserve == LastPublishedReserve) return;
    LastPublishedClip = Clip; LastPublishedReserve = Reserve;
    OnCombatStateChanged.Broadcast(Health, Shield, Clip, Reserve, IsValid(Weapon));
}

ECoastalCombatResult UCoastalCombatComponent::Emit(coastal::CombatDecision Decision, const FString& Detail)
{
    using D = coastal::CombatDecision;
    switch (Decision)
    {
    case D::Allowed: return Emit(ECoastalCombatResult::Applied, Detail);
    case D::NotConfigured: return Emit(ECoastalCombatResult::NotConfigured, Detail);
    case D::NoCampaign: return Emit(ECoastalCombatResult::NoCampaign, Detail);
    case D::OutsideEncounter: return Emit(ECoastalCombatResult::OutsideEncounter, Detail);
    case D::InputBlocked: return Emit(ECoastalCombatResult::InputBlocked, Detail);
    case D::SaveBusy: return Emit(ECoastalCombatResult::Busy, Detail);
    case D::Recovering: return Emit(ECoastalCombatResult::Recovering, Detail);
    case D::Camping: return Emit(ECoastalCombatResult::ActionActive, Detail);
    case D::Swimming: return Emit(ECoastalCombatResult::Swimming, Detail);
    case D::Paused: return Emit(ECoastalCombatResult::Paused, Detail);
    case D::ForeignAnimation: return Emit(ECoastalCombatResult::ForeignAnimation, Detail);
    case D::WrongEpoch: return Emit(ECoastalCombatResult::WrongEpoch, Detail);
    case D::Defeated: return Emit(ECoastalCombatResult::Defeated, Detail);
    case D::Reloading: return Emit(ECoastalCombatResult::Reloading, Detail);
    case D::Cooldown: return Emit(ECoastalCombatResult::Cooldown, Detail);
    case D::NoAmmo: return Emit(ECoastalCombatResult::NoAmmo, Detail);
    case D::MagazineFull: return Emit(ECoastalCombatResult::MagazineFull, Detail);
    case D::NoReserve: return Emit(ECoastalCombatResult::NoReserve, Detail);
    }
    return Emit(ECoastalCombatResult::Failed, Detail);
}

ECoastalCombatResult UCoastalCombatComponent::Emit(ECoastalCombatResult Result, const FString& Detail)
{
    LastDetail = Detail;
    OnCombatNotice.Broadcast(Result, Detail);
    return Result;
}
