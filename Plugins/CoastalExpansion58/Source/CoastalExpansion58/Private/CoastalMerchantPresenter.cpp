#include "CoastalMerchantPresenter.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalUISessionComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const FName VillageJournal(TEXT("journal.coastal_records.village"));
}

ACoastalMerchantPresenter::ACoastalMerchantPresenter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.f;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
    Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MerchantMesh"));
    Mesh->SetupAttachment(SceneRoot);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetGenerateOverlapEvents(false);
    MerchantMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Coastal/M3/Merchant/UE5/SKM_Manny.SKM_Manny")));
    IdleAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Coastal/M3/Merchant/UE5/Animation/AS_IdleBartering.AS_IdleBartering")));
    PitchAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Coastal/M3/Merchant/UE5/Animation/AS_PitchBarter_Rt.AS_PitchBarter_Rt")));
}

void ACoastalMerchantPresenter::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() != NM_Standalone)
    {
        LastDetail = TEXT("Merchant presentation is standalone-only.");
        SetActorTickEnabled(false);
    }
}

bool ACoastalMerchantPresenter::BindingsValid() const
{
    ACharacter* P = Player.Get();
    APlayerController* PC = IsValid(P) ? Cast<APlayerController>(P->GetController()) : nullptr;
    return bInitialized && IsValid(P) && IsValid(PC) && PC->IsLocalController() && PC->GetPawn() == P
        && IsValid(Bridge) && Bridge->GetOwner() == P && IsValid(Saves) && Saves->IsConfigured()
        && Saves->GetPlayerCharacter() == P && IsValid(UI) && UI->GetOwner() == PC;
}

bool ACoastalMerchantPresenter::IsInitialized() const { return BindingsValid(); }

bool ACoastalMerchantPresenter::TryInitialize()
{
    ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0);
    APlayerController* PC = IsValid(P) ? Cast<APlayerController>(P->GetController()) : nullptr;
    auto* NewBridge = IsValid(P) ? P->FindComponentByClass<UCoastalInteractionBridge>() : nullptr;
    auto* NewSaves = IsValid(NewBridge) ? NewBridge->GetCoordinator() : nullptr;
    auto* NewUI = IsValid(PC) ? PC->FindComponentByClass<UCoastalUISessionComponent>() : nullptr;
    if (bPresentationLoadFailed || !IsValid(P) || !IsValid(PC) || !PC->IsLocalController() || PC->GetPawn() != P
        || !IsValid(NewBridge) || !IsValid(NewSaves) || !NewSaves->IsConfigured()
        || NewSaves->GetPlayerCharacter() != P || !IsValid(NewUI))
        return false;
    USkeletalMesh* NewMesh = MerchantMesh.LoadSynchronous();
    UAnimSequence* NewIdle = IdleAnimation.LoadSynchronous();
    UAnimSequence* NewPitch = PitchAnimation.LoadSynchronous();
    if (!IsValid(NewMesh)
        || !IsValid(NewIdle) || !IsValid(NewPitch) || NewMesh->GetSkeleton() != NewIdle->GetSkeleton()
        || NewMesh->GetSkeleton() != NewPitch->GetSkeleton())
    {
        bPresentationLoadFailed = true;
        LastDetail = TEXT("Merchant presentation assets are missing or do not share the imported UE5 skeleton.");
        SetActorTickEnabled(false);
        return false;
    }
    Player = P; Bridge = NewBridge; Saves = NewSaves; UI = NewUI;
    LoadedIdle = NewIdle; LoadedPitch = NewPitch; Epoch = Saves->GetSessionEpoch(); bInitialized = true;
    Mesh->SetSkeletalMesh(NewMesh); Mesh->SetRelativeTransform(MeshRelativeTransform);
    Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    Bridge->OnJournalRequested.AddUniqueDynamic(this, &ACoastalMerchantPresenter::HandleJournalRequested);
    SetPhase(Saves->HasActiveCampaign() ? coastal::MerchantPhase::Idle : coastal::MerchantPhase::Dormant,
        TEXT("Merchant presentation initialized; the village register remains authoritative."));
    return true;
}

void ACoastalMerchantPresenter::PlayIdle()
{
    if (IsValid(LoadedIdle)) Mesh->PlayAnimation(LoadedIdle, true);
}

void ACoastalMerchantPresenter::SetPhase(coastal::MerchantPhase NewPhase, const FString& Detail)
{
    Phase = NewPhase; LastDetail = Detail;
    switch (Phase)
    {
    case coastal::MerchantPhase::Dormant:
        Presentation = ECoastalMerchantPresentation::Dormant; Mesh->Stop(); Mesh->SetVisibility(false, true); break;
    case coastal::MerchantPhase::Idle:
        Presentation = ECoastalMerchantPresentation::Idle; Mesh->SetVisibility(true, true); PlayIdle(); break;
    case coastal::MerchantPhase::AwaitJournalClose: Presentation = ECoastalMerchantPresentation::AwaitingJournalClose; break;
    case coastal::MerchantPhase::Pitching:
        Presentation = ECoastalMerchantPresentation::Pitching;
        Mesh->PlayAnimation(LoadedPitch, false);
        break;
    }
}

void ACoastalMerchantPresenter::HandleJournalRequested(FName JournalEntryId)
{
    if (JournalEntryId != VillageJournal || !BindingsValid() || !Saves->HasActiveCampaign()
        || Saves->GetSessionEpoch() != Epoch || Saves->IsBusy() || Saves->IsRecoveryRequired()
        || Saves->IsPlayerReturnActive() || Phase != coastal::MerchantPhase::Idle)
        return;
    QueueDeadline = UGameplayStatics::GetRealTimeSeconds(this) + FMath::Clamp(JournalCloseTimeoutSeconds, 5.f, 120.f);
    bSawJournalModal = false;
    SetPhase(coastal::MerchantPhase::AwaitJournalClose,
        TEXT("Village register interaction queued one presentation until its journal closes."));
}

void ACoastalMerchantPresenter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!BindingsValid())
    {
        ResetBindings();
        const double Now = UGameplayStatics::GetRealTimeSeconds(this);
        if (Now >= NextInitializeTime)
        {
            NextInitializeTime = Now + .5;
            TryInitialize();
        }
        return;
    }
    const uint64 CurrentEpoch = Saves->GetSessionEpoch();
    const double Now = UGameplayStatics::GetRealTimeSeconds(this);
    coastal::MerchantSample Sample;
    Sample.configured = true;
    Sample.campaign = Saves->HasActiveCampaign();
    Sample.epochMatches = CurrentEpoch == Epoch;
    Sample.paused = UGameplayStatics::IsGamePaused(this);
    Sample.recovery = Saves->IsRecoveryRequired();
    Sample.returning = Saves->IsPlayerReturnActive();
    Sample.busy = Saves->IsBusy();
    Sample.modal = UI->IsPresentingJournalEntry(VillageJournal);
    Sample.unexpectedModal = UI->HasModal() && !Sample.modal;
    if (Phase == coastal::MerchantPhase::AwaitJournalClose && Sample.modal) bSawJournalModal = true;
    Sample.worldInput = Bridge->AllowsWorldInput();
    Sample.expectedModalSeen = bSawJournalModal;
    Sample.clipFinished = Phase == coastal::MerchantPhase::Pitching && IsValid(LoadedPitch)
        && Mesh->GetPosition() + KINDA_SMALL_NUMBER >= LoadedPitch->GetPlayLength();
    Sample.queueExpired = Phase == coastal::MerchantPhase::AwaitJournalClose && Now >= QueueDeadline;
    const coastal::MerchantPhase Next = coastal::AdvanceMerchant(Phase, Sample);
    if (CurrentEpoch != Epoch)
    {
        Epoch = CurrentEpoch;
        SetPhase(Sample.campaign ? coastal::MerchantPhase::Idle : coastal::MerchantPhase::Dormant,
            TEXT("Merchant presentation reset for the current campaign epoch."));
    }
    else if (Next != Phase)
    {
        const FString Detail = Next == coastal::MerchantPhase::Pitching
            ? TEXT("Merchant presents an offer after the village journal closes; no trade is performed.")
            : TEXT("Merchant returned to its idle presentation.");
        SetPhase(Next, Detail);
    }
}

void ACoastalMerchantPresenter::ResetBindings()
{
    if (IsValid(Bridge)) Bridge->OnJournalRequested.RemoveDynamic(this, &ACoastalMerchantPresenter::HandleJournalRequested);
    Mesh->Stop(); Mesh->SetVisibility(false, true);
    Bridge = nullptr; Saves = nullptr; UI = nullptr; Player.Reset(); LoadedIdle = nullptr; LoadedPitch = nullptr;
    bInitialized = false; bSawJournalModal = false; Epoch = 0; QueueDeadline = 0;
    Phase = coastal::MerchantPhase::Dormant; Presentation = ECoastalMerchantPresentation::Dormant;
}

void ACoastalMerchantPresenter::EndPlay(const EEndPlayReason::Type Reason)
{
    ResetBindings();
    Super::EndPlay(Reason);
}
