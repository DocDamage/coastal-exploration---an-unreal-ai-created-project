#include "CoastalCompanionCharacter.h"
#include "CoastalPlacementLibrary.h"
#include "CoastalSaveCoordinator.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PhysicsVolume.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr float ProbeCm = 115.f;
constexpr float RetrySeconds = .75f;
bool IsWaterAt(UWorld* World, const FVector& Point, float Radius)
{
    if (!World) return true;
    for (TActorIterator<APhysicsVolume> It(World); It; ++It)
        if (It->bWaterVolume && It->EncompassesPoint(Point, Radius)) return true;
    return false;
}
}

ACoastalCompanionCharacter::ACoastalCompanionCharacter()
{
    // CharacterMovement consumes input every frame; a sparse actor tick pulses movement.
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickInterval = 0.f; PrimaryActorTick.TickGroup = TG_PrePhysics;
    bUseControllerRotationYaw = false;
    GetCapsuleComponent()->InitCapsuleSize(30.f, 48.f);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetGenerateOverlapEvents(false);
    auto* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true; Movement->RotationRate = FRotator(0, 500, 0);
    Movement->MaxAcceleration = Movement->BrakingDecelerationWalking = 900.f;
    Movement->bRunPhysicsWithNoController = true;
    Movement->NavAgentProps.bCanSwim = false;
    CompanionMeshAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/German_Shepherd_3D_Model/Models/SK_GermanShepherd_01.SK_GermanShepherd_01")));
    IdleAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/German_Shepherd_3D_Model/Animations/A_type1_Idle_Playing_v01.A_type1_Idle_Playing_v01")));
    WalkAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/German_Shepherd_3D_Model/Animations/A_type1_Walk_Loop_v01.A_type1_Walk_Loop_v01")));
    RunAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/German_Shepherd_3D_Model/Animations/A_type1_Run_Loop_v01.A_type1_Run_Loop_v01")));
    SetActorHiddenInGame(true); GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); Movement->DisableMovement();
}

bool ACoastalCompanionCharacter::SettingsValid() const
{
    return coastal::CompanionFollowBands{StopDistanceCm, RunDistanceCm, CatchupDistanceCm}.Valid()
        && FMath::IsFinite(FollowOffsetCm) && FollowOffsetCm >= 75 && FollowOffsetCm <= 600
        && FMath::IsFinite(SideOffsetCm) && FMath::Abs(SideOffsetCm) <= 300
        && FMath::IsFinite(WalkSpeedCmPerSecond) && WalkSpeedCmPerSecond >= 50
        && FMath::IsFinite(RunSpeedCmPerSecond) && RunSpeedCmPerSecond > WalkSpeedCmPerSecond;
}

bool ACoastalCompanionCharacter::BindingValid() const
{
    ACharacter* P = Player.Get(); APlayerController* PC = IsValid(P) ? Cast<APlayerController>(P->GetController()) : nullptr;
    return bInitialized && !bStopped && IsValid(P) && IsValid(PC) && PC->IsLocalController() && PC->GetPawn() == P
        && GetWorld() && GetWorld()->GetNetMode() == NM_Standalone && IsValid(Saves) && Saves->IsConfigured()
        && Saves->GetPlayerCharacter() == P && IsValid(Recovery) && Recovery->GetOwner() == P && Recovery->IsInitialized();
}
bool ACoastalCompanionCharacter::IsInitialized() const { return BindingValid(); }
bool ACoastalCompanionCharacter::CanAcceptCommand() const
{
    return BindingValid() && Saves->HasActiveCampaign() && Epoch == Saves->GetSessionEpoch() && !Saves->IsBusy()
        && !Saves->IsRecoveryRequired() && !Saves->IsPlayerReturnActive() && !Recovery->IsReturning();
}

bool ACoastalCompanionCharacter::LoadPresentation()
{
    USkeletalMesh* MeshAsset = CompanionMeshAsset.LoadSynchronous();
    LoadedIdle = IdleAnimation.LoadSynchronous(); LoadedWalk = WalkAnimation.LoadSynchronous(); LoadedRun = RunAnimation.LoadSynchronous();
    if (!IsValid(MeshAsset) || !IsValid(LoadedIdle) || !IsValid(LoadedWalk) || !IsValid(LoadedRun)
        || MeshAsset->GetSkeleton() != LoadedIdle->GetSkeleton() || MeshAsset->GetSkeleton() != LoadedWalk->GetSkeleton()
        || MeshAsset->GetSkeleton() != LoadedRun->GetSkeleton()) return false;
    GetMesh()->SetSkeletalMesh(MeshAsset); GetMesh()->SetRelativeTransform(MeshRelativeTransform);
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode); PresentedPace = coastal::CompanionPace::Catchup;
    PresentPace(coastal::CompanionPace::Idle); return true;
}

bool ACoastalCompanionCharacter::InitializeCompanion(ACharacter* P, UCoastalSaveCoordinator* C, UCoastalPlayerRecoveryComponent* R)
{
    APlayerController* PC = IsValid(P) ? Cast<APlayerController>(P->GetController()) : nullptr;
    if (!IsInGameThread() || bInitialized || bStopped || !SettingsValid() || !IsValid(P) || !IsValid(PC)
        || !PC->IsLocalController() || PC->GetPawn() != P || !GetWorld() || GetWorld()->GetNetMode() != NM_Standalone
        || !IsValid(C) || !C->IsConfigured() || C->GetWorld() != GetWorld() || C->GetPlayerCharacter() != P
        || !IsValid(R) || !R->IsInitialized() || R->GetOwner() != P || !LoadPresentation())
    { LastDetail = TEXT("Companion rejected incomplete standalone bindings, movement, or presentation assets."); return false; }
    Player = P; SetOwner(P); Saves = C; Recovery = R; Epoch = Saves->GetSessionEpoch(); bInitialized = true;
    Recovery->OnReturnNotice.AddUniqueDynamic(this, &ACoastalCompanionCharacter::HandlePlayerReturn);
    LastProgressLocation = GetActorLocation(); bNeedsRegroup = Saves->HasActiveCampaign(); SetActive(false);
    LastDetail = TEXT("Transient companion initialized on the existing local player; no save state was created."); return true;
}

bool ACoastalCompanionCharacter::SetFollowing(bool bShouldFollow)
{
    if (!CanAcceptCommand() || UGameplayStatics::IsGamePaused(this)) return false;
    bFollowRequested = bShouldFollow; bNeedsRegroup = bShouldFollow && IsHidden(); StopLocomotion();
    SetMode(bShouldFollow ? ECoastalCompanionMode::Following : ECoastalCompanionMode::Waiting,
        bShouldFollow ? TEXT("Companion will follow.") : TEXT("Companion will wait here.")); return true;
}
bool ACoastalCompanionCharacter::ToggleFollowing() { return SetFollowing(!bFollowRequested); }

void ACoastalCompanionCharacter::SetMode(ECoastalCompanionMode NewMode, const FString& Detail)
{
    const bool Changed = Mode != NewMode || LastDetail != Detail; Mode = NewMode; LastDetail = Detail;
    if (Changed) OnCompanionModeChanged.Broadcast(Mode, LastDetail);
}
void ACoastalCompanionCharacter::SetActive(bool Active)
{
    SetActorHiddenInGame(!Active); GetCapsuleComponent()->SetCollisionEnabled(Active ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    if (!Active) { StopLocomotion(); GetCharacterMovement()->DisableMovement(); }
    else if (GetCharacterMovement()->MovementMode == MOVE_None) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}
FVector ACoastalCompanionCharacter::FollowTarget() const
{
    ACharacter* P = Player.Get(); if (!IsValid(P)) return GetActorLocation(); const FRotator Yaw(0, P->GetActorRotation().Yaw, 0);
    return P->GetActorLocation() - Yaw.Vector() * FollowOffsetCm + FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y) * SideOffsetCm;
}

bool ACoastalCompanionCharacter::FindDryLocationNearPlayer(FTransform& Out) const
{
    ACharacter* P = Player.Get(); if (!IsValid(P) || !GetWorld()) return false;
    const FRotator Yaw(0, P->GetActorRotation().Yaw, 0); const FVector F = Yaw.Vector(), R = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Base = P->GetActorLocation();
    const FVector2D Offsets[] = {{-FollowOffsetCm, SideOffsetCm}, {-FollowOffsetCm, -SideOffsetCm}, {-FollowOffsetCm * 1.35f, 0}, {0, FollowOffsetCm}, {0, -FollowOffsetCm}};
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CoastalCompanionDryRegroup), false, this); Params.AddIgnoredActor(P);
    const float HH = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    for (const FVector2D& O : Offsets)
    {
        const FVector XY = Base + F * O.X + R * O.Y; FHitResult Floor;
        if (!GetWorld()->LineTraceSingleByChannel(Floor, XY + FVector(0,0,250), XY - FVector(0,0,500), ECC_Visibility, Params)
            || !Floor.bBlockingHit || Floor.ImpactNormal.Z < GetCharacterMovement()->GetWalkableFloorZ()
            || (Floor.GetActor() && Floor.GetActor()->ActorHasTag(TEXT("Coastal.UnsafeCheckpoint")))) continue;
        const FTransform Candidate(Yaw, Floor.ImpactPoint + FVector(0,0,HH + 2), FVector::OneVector);
        if (!IsWaterAt(GetWorld(), Candidate.GetLocation(), GetCapsuleComponent()->GetScaledCapsuleRadius())
            && UCoastalPlacementLibrary::IsDryDestination(const_cast<ACoastalCompanionCharacter*>(this), Candidate)) { Out = Candidate; return true; }
    }
    return false;
}

bool ACoastalCompanionCharacter::TryRegroup()
{
    if (!CanAcceptCommand()) return false; const double Now = UGameplayStatics::GetRealTimeSeconds(this);
    if (Now < NextRegroupTime) return false; NextRegroupTime = Now + RetrySeconds; FTransform Destination;
    if (!FindDryLocationNearPlayer(Destination)) { SetActive(false); SetMode(ECoastalCompanionMode::Regrouping, TEXT("Companion is waiting for a verified dry regroup location.")); return false; }
    SetActive(false); if (!SetActorTransform(Destination, false, nullptr, ETeleportType::TeleportPhysics)) return false;
    GetCharacterMovement()->StopMovementImmediately(); SetActive(true); bNeedsRegroup = false; StuckSeconds = 0;
    LastProgressLocation = GetActorLocation(); NextRegroupTime = Now + 2.; SetMode(ECoastalCompanionMode::Following, TEXT("Companion regrouped at a verified dry location.")); return true;
}

bool ACoastalCompanionCharacter::CanAdvance(const FVector& Direction, float Step) const
{
    if (!GetWorld() || Direction.IsNearlyZero()) return false; const float Rad = GetCapsuleComponent()->GetScaledCapsuleRadius(), HH = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Start = GetActorLocation(), End = Start + Direction.GetSafeNormal2D() * Step;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CoastalCompanionSteering), false, this); Params.AddIgnoredActor(Player.Get()); FHitResult Hit;
    if (GetWorld()->SweepSingleByProfile(Hit, Start, End, FQuat::Identity, TEXT("Pawn"), FCollisionShape::MakeCapsule(Rad, HH), Params) && Hit.bBlockingHit) return false;
    FHitResult Floor; if (!GetWorld()->LineTraceSingleByChannel(Floor, End + FVector(0,0,HH+60), End - FVector(0,0,HH+140), ECC_Visibility, Params)
        || !Floor.bBlockingHit || Floor.ImpactNormal.Z < GetCharacterMovement()->GetWalkableFloorZ()
        || (Floor.GetActor() && Floor.GetActor()->ActorHasTag(TEXT("Coastal.UnsafeCheckpoint")))) return false;
    const FTransform Dry(FRotator(0, GetActorRotation().Yaw, 0), Floor.ImpactPoint + FVector(0,0,HH+2), FVector::OneVector);
    return !IsWaterAt(GetWorld(), Dry.GetLocation(), Rad)
        && UCoastalPlacementLibrary::IsDryDestination(const_cast<ACoastalCompanionCharacter*>(this), Dry);
}
bool ACoastalCompanionCharacter::ChooseSteeringDirection(const FVector& Target, FVector& Out)
{
    const FVector Desired = (Target - GetActorLocation()).GetSafeNormal2D(); if (Desired.IsNearlyZero()) return false;
    const float Angles[] = {0, 38*AvoidanceSign, 76*AvoidanceSign, -38*AvoidanceSign, -76*AvoidanceSign, 112*AvoidanceSign};
    for (float A : Angles) { const FVector C = Desired.RotateAngleAxis(A, FVector::UpVector).GetSafeNormal2D(); if (CanAdvance(C, ProbeCm)) { Out=C; return true; } }
    return false;
}
void ACoastalCompanionCharacter::StopLocomotion() { if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately(); PresentPace(coastal::CompanionPace::Idle); }
void ACoastalCompanionCharacter::PresentPace(coastal::CompanionPace Pace)
{
    if (Pace == coastal::CompanionPace::Catchup) Pace = coastal::CompanionPace::Idle; if (Pace == PresentedPace) return;
    UAnimSequence* A = Pace == coastal::CompanionPace::Run ? LoadedRun.Get() : Pace == coastal::CompanionPace::Walk ? LoadedWalk.Get() : LoadedIdle.Get();
    if (IsValid(A)) GetMesh()->PlayAnimation(A, true); PresentedPace = Pace;
}

void ACoastalCompanionCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); if (!BindingValid()) { SetActive(false); SetMode(ECoastalCompanionMode::Dormant, TEXT("Companion binding is no longer valid.")); return; }
    const uint64 CurrentEpoch=Saves->GetSessionEpoch(); if (Epoch != CurrentEpoch) { Epoch=CurrentEpoch; bFollowRequested=true; bNeedsRegroup=true; NextRegroupTime=0; StuckSeconds=0; SetActive(false); }
    if (!IsHidden() && (GetCharacterMovement()->IsSwimming()
        || IsWaterAt(GetWorld(), GetActorLocation() - FVector(0, 0,
            GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), GetCapsuleComponent()->GetScaledCapsuleRadius())))
    { bNeedsRegroup = true; SetActive(false); }
    coastal::CompanionLifecycleSample S{true,Saves->HasActiveCampaign(),bFollowRequested,true,Saves->IsBusy(),Saves->IsRecoveryRequired(),Saves->IsPlayerReturnActive()||Recovery->IsReturning(),UGameplayStatics::IsGamePaused(this),bNeedsRegroup||IsHidden()};
    switch (coastal::CompanionDecision(S)) {
    case coastal::CompanionDisposition::Dormant: SetActive(false); SetMode(ECoastalCompanionMode::Dormant,TEXT("Companion is dormant until a campaign is active.")); return;
    case coastal::CompanionDisposition::Regroup: SetActive(false); bNeedsRegroup=true; SetMode(ECoastalCompanionMode::Regrouping,TEXT("Companion suspended for campaign recovery or regroup.")); if(!S.recoveryRequired&&!S.returning) TryRegroup(); return;
    case coastal::CompanionDisposition::Hold: StopLocomotion(); SetMode(bFollowRequested?ECoastalCompanionMode::Following:ECoastalCompanionMode::Waiting,bFollowRequested?TEXT("Companion follow is temporarily held."):TEXT("Companion is waiting here.")); return;
    case coastal::CompanionDisposition::Follow: break; }
    const FVector Target=FollowTarget(); const float Distance=FVector::Dist2D(GetActorLocation(),Target); const auto Pace=coastal::CompanionPaceForDistance(Distance,{StopDistanceCm,RunDistanceCm,CatchupDistanceCm});
    if(Pace==coastal::CompanionPace::Catchup){bNeedsRegroup=true;TryRegroup();return;} if(Pace==coastal::CompanionPace::Idle){StopLocomotion();StuckSeconds=0;LastProgressLocation=GetActorLocation();SetMode(ECoastalCompanionMode::Following,TEXT("Companion is holding formation."));return;}
    FVector Direction; if(!ChooseSteeringDirection(Target,Direction)){StopLocomotion();StuckSeconds+=DeltaSeconds;if(StuckSeconds>=1.25f){AvoidanceSign*=-1;StuckSeconds=0;}SetMode(ECoastalCompanionMode::Following,TEXT("Companion is finding a dry route around an obstacle."));return;}
    if(FVector::Dist2D(GetActorLocation(),LastProgressLocation)<8) StuckSeconds+=DeltaSeconds; else{StuckSeconds=0;LastProgressLocation=GetActorLocation();} if(StuckSeconds>=1.25f){AvoidanceSign*=-1;StuckSeconds=0;}
    GetCharacterMovement()->MaxWalkSpeed=Pace==coastal::CompanionPace::Run?RunSpeedCmPerSecond:WalkSpeedCmPerSecond; AddMovementInput(Direction,1,true); PresentPace(Pace); SetMode(ECoastalCompanionMode::Following,Pace==coastal::CompanionPace::Run?TEXT("Companion is catching up on foot."):TEXT("Companion is following."));
}
void ACoastalCompanionCharacter::HandlePlayerReturn(ECoastalReturnNotice Result, FString)
{
    if(Result==ECoastalReturnNotice::Returning||Result==ECoastalReturnNotice::RestartRequired){bNeedsRegroup=true;SetActive(false);SetMode(ECoastalCompanionMode::Regrouping,TEXT("Companion suspended during safe return."));}
    else if(Result==ECoastalReturnNotice::Returned){bNeedsRegroup=true;NextRegroupTime=0;}
}
void ACoastalCompanionCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    bStopped=true;if(IsValid(Recovery))Recovery->OnReturnNotice.RemoveDynamic(this,&ACoastalCompanionCharacter::HandlePlayerReturn);StopLocomotion();Player.Reset();Saves=nullptr;Recovery=nullptr;Super::EndPlay(Reason);
}
