#include "CoastalCombatActors.h"
#include "CoastalCombatComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

ACoastalSidearmWeapon::ACoastalSidearmWeapon()
{
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoastalWeaponVisual"));
    VisualMesh->SetupAttachment(WeaponMesh);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetGenerateOverlapEvents(false);
    MuzzleAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
    MuzzleAnchor->SetupAttachment(WeaponMesh);
}

void ACoastalSidearmWeapon::ConfigurePresentation(UStaticMesh* Mesh,
    FTransform VisualTransform, FVector MuzzleOffset)
{
    VisualMesh->SetStaticMesh(Mesh);
    VisualMesh->SetRelativeTransform(VisualTransform);
    MuzzleAnchor->SetRelativeLocation(MuzzleOffset);
}

bool ACoastalSidearmWeapon::IsPresentationReady() const
{
    return IsValid(VisualMesh) && IsValid(VisualMesh->GetStaticMesh()) && IsValid(MuzzleAnchor);
}

FVector ACoastalSidearmWeapon::GetMuzzleSocketLocation() const
{
    return IsValid(MuzzleAnchor) ? MuzzleAnchor->GetComponentLocation()
        : AWeaponBase::GetMuzzleSocketLocation();
}

ACoastalCombatTracer::ACoastalCombatTracer()
{
    PrimaryActorTick.bCanEverTick = false;
    TracerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TracerMesh"));
    SetRootComponent(TracerMesh);
    TracerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TracerMesh->SetGenerateOverlapEvents(false);
    SetActorEnableCollision(false);
}

bool ACoastalCombatTracer::Configure(UStaticMesh* Mesh, UMaterialInterface* Material,
    const FVector& Start, const FVector& End, float RadiusCm, float LifetimeSeconds)
{
    const FVector Delta = End - Start;
    const float Length = Delta.Size();
    if (!IsValid(Mesh) || Length < 1.f || RadiusCm <= 0.f || LifetimeSeconds <= 0.f) return false;
    TracerMesh->SetStaticMesh(Mesh);
    if (IsValid(Material)) TracerMesh->SetMaterial(0, Material);
    SetActorLocation((Start + End) * 0.5f);
    SetActorRotation(FQuat::FindBetweenNormals(FVector::UpVector, Delta / Length));
    // Engine/project tracer meshes are authored as a 100 cm Z-axis segment.
    TracerMesh->SetWorldScale3D(FVector(RadiusCm / 50.f, RadiusCm / 50.f, Length / 100.f));
    SetLifeSpan(LifetimeSeconds);
    return true;
}

ACoastalCombatTarget::ACoastalCombatTarget()
{
    PrimaryActorTick.bCanEverTick = false;
    TargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetMesh"));
    SetRootComponent(TargetMesh);
    TargetMesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
    TargetMesh->SetGenerateOverlapEvents(false);
}

void ACoastalCombatTarget::BeginPlay()
{
    Super::BeginPlay();
    Health = FMath::Max(1.f, MaxHealth);
    if (UStaticMesh* Mesh = PresentationAsset.LoadSynchronous()) TargetMesh->SetStaticMesh(Mesh);
    OnTakeAnyDamage.AddUniqueDynamic(this, &ACoastalCombatTarget::HandleDamage);
}

void ACoastalCombatTarget::HandleDamage(AActor* DamagedActor, float Damage,
    const UDamageType*, AController*, AActor*)
{
    if (DamagedActor != this || bDefeated || !FMath::IsFinite(Damage) || Damage <= 0.f) return;
    Health = FMath::Max(0.f, Health - Damage);
    if (Health > 0.f) return;
    bDefeated = true;
    TargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TargetMesh->SetVisibility(false, true);
    SetLifeSpan(1.f);
}

ACoastalCombatSentry::ACoastalCombatSentry()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ACoastalCombatSentry::BeginPlay()
{
    Super::BeginPlay();
    Home = GetActorLocation();
    if (GetNetMode() == NM_Standalone && AttackIntervalSeconds >= 0.25f)
        GetWorldTimerManager().SetTimer(AttackTimer, this,
            &ACoastalCombatSentry::AttemptAttack, AttackIntervalSeconds, true);
}

void ACoastalCombatSentry::AttemptAttack()
{
    if (bDefeated || !IsValid(TargetMesh)) return;
    ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    UCoastalCombatComponent* Combat = IsValid(Player)
        ? Player->FindComponentByClass<UCoastalCombatComponent>() : nullptr;
    if (!IsValid(Combat) || !Combat->CanReceiveHostileAttack() || !IsValid(PC)) return;
    if (bRequireRecentlyRendered)
    {
        FVector2D Screen;
        int32 Width = 0, Height = 0;
        PC->GetViewportSize(Width, Height);
        if (!TargetMesh->WasRecentlyRendered(0.3f)
            || !PC->ProjectWorldLocationToScreen(TargetMesh->GetComponentLocation(), Screen, false)
            || Width <= 0 || Height <= 0 || Screen.X < 0 || Screen.Y < 0
            || Screen.X > Width || Screen.Y > Height) return;
    }
    const FVector Target = Player->GetActorLocation() + FVector(0, 0, 40.f);
    const FVector Start = GetActorLocation() + FVector(0, 0, 60.f);
    if (FVector::DistSquared(Home, Target) > FMath::Square(EncounterRadiusCm)
        || FVector::DistSquared(Start, Target) > FMath::Square(EngagementRangeCm)) return;
    FHitResult Hit;
    FCollisionQueryParams Params(TEXT("CoastalSentryLOS"), true, this);
    Params.AddIgnoredActor(this);
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, Target, ECC_Visibility, Params)
        || Hit.GetActor() != Player) return;
    const FVector Direction = (Target - Start).GetSafeNormal();
    UGameplayStatics::ApplyPointDamage(Player, AttackDamage, Direction, Hit,
        nullptr, this, UDamageType::StaticClass());
    if (UStaticMesh* Mesh = TracerAsset.LoadSynchronous())
    {
        auto* Tracer = GetWorld()->SpawnActor<ACoastalCombatTracer>(Start, FRotator::ZeroRotator);
        if (!IsValid(Tracer) || !Tracer->Configure(Mesh, nullptr, Start, Hit.ImpactPoint, 1.25f, 0.08f))
            if (IsValid(Tracer)) Tracer->Destroy();
    }
}

ACoastalCombatEncounterVolume::ACoastalCombatEncounterVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("EncounterBounds"));
    SetRootComponent(Bounds);
    Bounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Bounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    Bounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Bounds->SetGenerateOverlapEvents(true);
}

bool ACoastalCombatEncounterVolume::IsAuthoredCorrectly() const
{
    return IsValid(Bounds) && Bounds->GetCollisionEnabled() == ECollisionEnabled::QueryOnly
        && Bounds->GetGenerateOverlapEvents() && Bounds->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Overlap
        && Bounds->GetScaledBoxExtent().GetMin() >= 100.f;
}

void ACoastalCombatEncounterVolume::BeginPlay()
{
    Super::BeginPlay();
    if (!IsAuthoredCorrectly()) { SetActorTickEnabled(false); return; }
    Bounds->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACoastalCombatEncounterVolume::Enter);
    Bounds->OnComponentEndOverlap.AddUniqueDynamic(this, &ACoastalCombatEncounterVolume::Leave);
    TArray<AActor*> Existing;
    Bounds->GetOverlappingActors(Existing, ACharacter::StaticClass());
    for (AActor* Actor : Existing)
        if (auto* Combat = Actor->FindComponentByClass<UCoastalCombatComponent>()) Combat->EnterEncounter(this);
}

void ACoastalCombatEncounterVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    TArray<AActor*> Existing;
    Bounds->GetOverlappingActors(Existing, ACharacter::StaticClass());
    for (AActor* Actor : Existing)
        if (auto* Combat = Actor->FindComponentByClass<UCoastalCombatComponent>()) Combat->LeaveEncounter(this);
    Super::EndPlay(EndPlayReason);
}

void ACoastalCombatEncounterVolume::Enter(UPrimitiveComponent*, AActor* Other,
    UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (auto* Combat = IsValid(Other) ? Other->FindComponentByClass<UCoastalCombatComponent>() : nullptr)
        Combat->EnterEncounter(this);
}

void ACoastalCombatEncounterVolume::Leave(UPrimitiveComponent*, AActor* Other,
    UPrimitiveComponent*, int32)
{
    if (auto* Combat = IsValid(Other) ? Other->FindComponentByClass<UCoastalCombatComponent>() : nullptr)
        Combat->LeaveEncounter(this);
}
