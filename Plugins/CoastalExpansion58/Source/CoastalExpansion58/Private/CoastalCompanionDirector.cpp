#include "CoastalCompanionDirector.h"
#include "CoastalCompanionCharacter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "CoastalSaveCoordinator.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ACoastalCompanionDirector::ACoastalCompanionDirector()
{
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickInterval=.25f; CompanionClass=ACoastalCompanionCharacter::StaticClass();
    CompanionMeshAsset=TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/German_Shepherd_3D_Model/Models/SK_GermanShepherd_01.SK_GermanShepherd_01")));
    IdleAnimation=TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/German_Shepherd_3D_Model/Animations/A_type1_Idle_Playing_v01.A_type1_Idle_Playing_v01")));
    WalkAnimation=TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/German_Shepherd_3D_Model/Animations/A_type1_Walk_Loop_v01.A_type1_Walk_Loop_v01")));
    RunAnimation=TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/German_Shepherd_3D_Model/Animations/A_type1_Run_Loop_v01.A_type1_Run_Loop_v01")));
}
void ACoastalCompanionDirector::BeginPlay(){Super::BeginPlay();if(GetNetMode()!=NM_Standalone){LastDetail=TEXT("Coastal companion is standalone-only.");SetActorTickEnabled(false);}}
bool ACoastalCompanionDirector::IsInitialized() const{return IsValid(Companion)&&Companion->IsInitialized()&&Companion->GetPlayerOwner()==BoundPlayer.Get();}
bool ACoastalCompanionDirector::CanCommandCompanion() const{return IsInitialized()&&Companion->CanAcceptCommand();}
bool ACoastalCompanionDirector::IsCompanionFollowing() const{return IsInitialized()&&Companion->IsFollowingRequested();}
bool ACoastalCompanionDirector::SetCompanionFollowing(bool Following){return SetFollowing(Following);}
bool ACoastalCompanionDirector::SetFollowing(bool Following)
{
    if(!CanCommandCompanion()){LastDetail=TEXT("Companion command rejected outside the current ready campaign.");return false;}
    const bool Applied=Companion->SetFollowing(Following);LastDetail=Applied?(Following?TEXT("Companion follow command applied."):TEXT("Companion wait command applied.")):Companion->LastDetail;return Applied;
}
bool ACoastalCompanionDirector::ToggleFollowing(){return SetFollowing(!IsCompanionFollowing());}
void ACoastalCompanionDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);ACharacter* Current=UGameplayStatics::GetPlayerCharacter(this,0);
    if(IsValid(Companion)&&(!Companion->IsInitialized()||BoundPlayer.Get()!=Current))DestroyOwnedCompanion();if(!IsInitialized())TryInitialize();
}
bool ACoastalCompanionDirector::TryInitialize()
{
    ACharacter* P=UGameplayStatics::GetPlayerCharacter(this,0);APlayerController* PC=IsValid(P)?Cast<APlayerController>(P->GetController()):nullptr;
    auto* Bridge=IsValid(P)?P->FindComponentByClass<UCoastalInteractionBridge>():nullptr;auto* Saves=IsValid(Bridge)?Bridge->GetCoordinator():nullptr;
    auto* Recovery=IsValid(P)?P->FindComponentByClass<UCoastalPlayerRecoveryComponent>():nullptr;
    if(!IsValid(P)||!IsValid(PC)||!PC->IsLocalController()||PC->GetPawn()!=P||!IsValid(Bridge)||!IsValid(Saves)||!Saves->IsConfigured()
        ||Saves->GetPlayerCharacter()!=P||!IsValid(Recovery)||!Recovery->IsInitialized()||!CompanionClass||!CompanionMeshAsset.ToSoftObjectPath().IsValid()
        ||!IdleAnimation.ToSoftObjectPath().IsValid()||!WalkAnimation.ToSoftObjectPath().IsValid()||!RunAnimation.ToSoftObjectPath().IsValid())return false;
    for(TActorIterator<ACoastalCompanionDirector> It(GetWorld());It;++It)if(*It!=this&&It->IsInitialized()){LastDetail=TEXT("Another initialized companion director already owns the player.");return false;}
    const FTransform Spawn(P->GetActorRotation(),P->GetActorLocation());auto* Dog=GetWorld()->SpawnActorDeferred<ACoastalCompanionCharacter>(CompanionClass,Spawn,P,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!IsValid(Dog))return false;Dog->CompanionMeshAsset=CompanionMeshAsset;Dog->IdleAnimation=IdleAnimation;Dog->WalkAnimation=WalkAnimation;Dog->RunAnimation=RunAnimation;Dog->MeshRelativeTransform=MeshRelativeTransform;
    UGameplayStatics::FinishSpawningActor(Dog,Spawn);if(!Dog->InitializeCompanion(P,Saves,Recovery)){LastDetail=Dog->LastDetail;Dog->Destroy();return false;}
    Companion=Dog;BoundPlayer=P;LastDetail=TEXT("Transient German Shepherd companion initialized for the existing player.");return true;
}
void ACoastalCompanionDirector::DestroyOwnedCompanion(){if(IsValid(Companion))Companion->Destroy();Companion=nullptr;BoundPlayer.Reset();}
void ACoastalCompanionDirector::EndPlay(const EEndPlayReason::Type Reason){DestroyOwnedCompanion();Super::EndPlay(Reason);}
