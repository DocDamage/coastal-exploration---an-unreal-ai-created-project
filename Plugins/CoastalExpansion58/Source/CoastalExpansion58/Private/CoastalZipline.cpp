#include "CoastalZipline.h"
#include "CoastalRopeWorldCollision.h"
#include "CoastalGrappleAnchor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

bool UCoastalZiplineRope::CanWrapTarget(const USceneComponent* Mesh, FName) const
{ return IsValid(Mesh) && Endpoint.IsValid() && Mesh == Endpoint.Get(); }

ACoastalZipline::ACoastalZipline()
{
    PrimaryActorTick.bCanEverTick = true;
    StartAnchor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartAnchor"));
    SetRootComponent(StartAnchor);
    StartAnchor->SetMobility(EComponentMobility::Static);
    StartAnchor->SetCollisionProfileName(TEXT("BlockAll"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Mesh.Succeeded()) StartAnchor->SetStaticMesh(Mesh.Object);
    Cable = CreateDefaultSubobject<UCoastalZiplineRope>(TEXT("DynamicZiplineCable"));
    Cable->SetupAttachment(StartAnchor);
    Cable->SetRelativeLocation(FVector(80,0,-60));
    Cable->ResolveMode = ERopeWrapResolveMode::GuaranteedWrap;
    Cable->NumParticles = 32;
    Cable->SolverConfig.MaxStretchRatio = 1.f;
    Cable->SolverConfig.Iterations = 32;
    Cable->SolverConfig.bEnableDistanceLOD = false;
    Cable->Radius = 1.f;
    // This fixed, capsule-validated route uses the analytic world colliders.
    // Coarse camera-dependent terrain distance fields distort a far-away cable.
    Cable->bUseWorldGDF = false;
    Cable->RopeLength = 2500.f;
    Cable->MinRopeLength = 100.f;
    Cable->ThrowParams.FrameMode = ERopeThrowFrameMode::World;
    Cable->HoldConfig.bEnforceWielderLengthConstraint = false;
    Tags.Add(TEXT("Coastal.Zipline"));
}

void ACoastalZipline::BeginPlay()
{
    if (IsValid(EndAnchor))
    {
        Cable->Endpoint = EndAnchor->AnchorMesh;
        Cable->RopeLength = FMath::Clamp(FVector::Distance(GetActorLocation(), EndAnchor->GetActorLocation()) * 1.03f, 200.f, 10000.f);
    }
    Super::BeginPlay();
    RetryAt = GetWorld()->GetTimeSeconds() + 1.0;
}

bool ACoastalZipline::IsReady() const
{
    if (!bTensioned || !IsValid(EndAnchor) || !IsValid(Cable)
        || Cable->GetPhase() != ERopePhase::Wrapped || Cable->GetNodeCount() <= 2) return false;
    const float Rest = Cable->GetCurrentRopeLength() / (Cable->GetNodeCount() - 1);
    for (int32 I = 1; I < Cable->GetNodeCount(); ++I)
        if (FVector::Distance(Cable->GetNodePosition(I - 1), Cable->GetNodePosition(I)) > Rest * 1.1f)
            return false;
    return true;
}

void ACoastalZipline::Tick(float Delta)
{
    Super::Tick(Delta);
    if (!bWorldCollisionConfigured) bWorldCollisionConfigured = ConfigureCoastalRopeWorldCollision(GetWorld());
    if (!IsValid(EndAnchor) || GetWorld()->GetNetMode() != NM_Standalone) return;
    if (!bTensioned && Cable->GetPhase() == ERopePhase::Wrapped && Cable->GetNodeCount() > 2)
    {
        const float Span = FVector::Distance(Cable->GetNodePosition(0), Cable->GetNodePosition(Cable->GetNodeCount()-1));
        Cable->SetRopeLength(Span * 1.0002f);
        // The placed component's preview proxy predates the runtime wrap. Rebuild
        // once against the settled simulation; otherwise the cached tube can stay invisible.
        Cable->MarkRenderStateDirty();
        bTensioned = true;
    }
    if (IsReady()) { LastDetail = TEXT("Zipline ready."); return; }
    if (bPrepared)
    {
        bPrepared = false;
        if (bPreparedEndpoint) Cable->RequestExecuteQueuedGuaranteedAimThrow();
        else
        {
            Cable->CancelQueuedGuaranteedAimThrow(); bRequested = false;
            RetryAt = GetWorld()->GetTimeSeconds() + 1.0;
            LastDetail = TEXT("Zipline endpoint path is blocked or unavailable.");
        }
    }
    if (bRequested || GetWorld()->GetTimeSeconds() < RetryAt) return;
    if (Cable->GetPhase() == ERopePhase::Free) Cable->EnterLoaded();
    if (Cable->GetPhase() != ERopePhase::Loaded) return;
    Cable->Endpoint = EndAnchor->AnchorMesh;
    FRopeAimRayThrowRequest Request;
    Request.RayOrigin = Cable->GetComponentLocation();
    Request.RayDirection = (EndAnchor->GetActorLocation() - Request.RayOrigin).GetSafeNormal();
    Request.RayLength = Cable->RopeLength;
    Request.ReachOrigin = Request.RayOrigin; Request.ReachLength = Cable->RopeLength;
    Request.QueryRadius = 2.f;
    Request.BaseContext.Origin = Request.RayOrigin;
    Request.BaseContext.FrameForward = Request.RayDirection;
    Request.BaseContext.FrameMode = ERopeThrowFrameMode::World;
    Request.BaseContext.ThrowSpeed = 12000.f;
    Request.OnRejected.BindWeakLambda(this, [this]() { bRequested = false; RetryAt = GetWorld()->GetTimeSeconds() + 1.0; LastDetail = TEXT("Zipline endpoint could not be reached."); });
    Request.OnPrepared.BindWeakLambda(this, [this](const FRopePreparedThrowPreview& Prepared)
    {
        // Consume next actor tick so the plugin delegate is not destroyed in its own callback.
        bPreparedEndpoint = Prepared.IsValid() && Prepared.Mesh.Get() == Cable->Endpoint.Get();
        bPrepared = true;
    });
    bRequested = Cable->QueueGuaranteedAimThrow(Request, false);
    if (!bRequested) RetryAt = GetWorld()->GetTimeSeconds() + 1.0;
}

bool ACoastalZipline::Sample(float Fraction, FVector& Point, FVector& Tangent, float& Length) const
{
    if (!IsReady()) return false;
    Length = 0.f;
    for (int32 I = 1; I < Cable->GetNodeCount(); ++I)
        Length += FVector::Distance(Cable->GetNodePosition(I - 1), Cable->GetNodePosition(I));
    if (!FMath::IsFinite(Length) || Length < 100.f) return false;
    float Remaining = FMath::Clamp(Fraction, 0.f, 1.f) * Length;
    for (int32 I = 1; I < Cable->GetNodeCount(); ++I)
    {
        const FVector A = Cable->GetNodePosition(I - 1), B = Cable->GetNodePosition(I);
        const float Span = FVector::Distance(A, B);
        if (Span > KINDA_SMALL_NUMBER && (Remaining <= Span || I == Cable->GetNodeCount() - 1))
        {
            Point = FMath::Lerp(A, B, FMath::Clamp(Remaining / Span, 0.f, 1.f));
            Tangent = (B - A).GetSafeNormal();
            return !Point.ContainsNaN();
        }
        Remaining -= Span;
    }
    return false;
}
