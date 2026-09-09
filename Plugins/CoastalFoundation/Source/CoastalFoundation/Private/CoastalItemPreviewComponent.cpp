#include "CoastalItemPreviewComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

const FVector UCoastalItemPreviewComponent::PreviewWorldLocation(0.0f, 0.0f, -1000000.0f);

UCoastalItemPreviewComponent::UCoastalItemPreviewComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UCoastalItemPreviewComponent::InitializePreview(APlayerController* InController)
{
    if (bInitialized || bClosing || !IsRegistered() || GetOwner() != InController
        || !IsValid(InController) || !InController->IsLocalController() || InController->GetNetMode() != NM_Standalone
        || !IsValid(InController->GetRootComponent())) return false;
    Controller = InController;
    bInitialized = true;
    return true;
}

bool UCoastalItemPreviewComponent::OpenPreview(FGuid InstanceId, int64 ViewRevision, UStaticMesh* Mesh)
{
    if (!bInitialized || bClosing || bOpen || !IsRegistered() || GetOwner() != Controller
        || !IsValid(Controller) || !Controller->IsLocalController() || Controller->GetNetMode() != NM_Standalone
        || !InstanceId.IsValid()
        || !coastal::ValidPreviewRevision(ViewRevision) || !IsValid(Mesh)) return false;
    if (!CreatePreviewResources()) { ClosePreview(); return false; }
    PreviewMesh->SetStaticMesh(Mesh);
    const float Radius = FMath::Max(1.0f, Mesh->GetBounds().SphereRadius);
    const float Scale = FMath::Clamp(60.0f / Radius, 0.1f, 20.0f);
    PreviewMesh->SetRelativeScale3D(FVector(Scale));
    PreviewMesh->SetRelativeLocation(-Mesh->GetBounds().Origin * Scale);
    PreviewMesh->SetVisibleInSceneCaptureOnly(true);
    PreviewCapture->ClearShowOnlyComponents();
    PreviewCapture->ShowOnlyComponent(PreviewMesh);
    SelectedInstance = InstanceId;
    SelectedRevision = ViewRevision;
    Pose = {};
    bOpen = true;
    ApplyPose();
    return true;
}

bool UCoastalItemPreviewComponent::RotatePreview(FGuid InstanceId, int64 ViewRevision,
    float YawDegrees, float PitchDegrees)
{
    if (!Matches(InstanceId, ViewRevision) || !Pose.Apply(YawDegrees, PitchDegrees)) return false;
    ApplyPose();
    return true;
}

void UCoastalItemPreviewComponent::ClosePreview()
{
    if (bClosing) return;
    bClosing = true;
    bOpen = false;
    SelectedInstance.Invalidate();
    SelectedRevision = -1;
    Pose = {};
    if (PreviewCapture)
    {
        PreviewCapture->ClearShowOnlyComponents();
        PreviewCapture->TextureTarget = nullptr;
        PreviewCapture->DestroyComponent();
        PreviewCapture = nullptr;
    }
    if (PreviewMesh)
    {
        PreviewMesh->SetStaticMesh(nullptr);
        PreviewMesh->SetVisibleInSceneCaptureOnly(false);
        PreviewMesh->DestroyComponent();
        PreviewMesh = nullptr;
    }
    if (PreviewPivot)
    {
        PreviewPivot->DestroyComponent();
        PreviewPivot = nullptr;
    }
    if (PreviewRoot)
    {
        PreviewRoot->DestroyComponent();
        PreviewRoot = nullptr;
    }
    if (RenderTarget)
    {
        RenderTarget->ReleaseResource();
        RenderTarget = nullptr;
    }
    if (RenderOwner)
    {
        RenderOwner->Destroy();
        RenderOwner = nullptr;
    }
    bClosing = false;
}

void UCoastalItemPreviewComponent::ResetForEpoch(int64 NewEpoch)
{
    if (NewEpoch == Epoch) return;
    Epoch = NewEpoch;
    ClosePreview();
}

void UCoastalItemPreviewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClosePreview();
    bInitialized = false;
    Controller = nullptr;
    Super::EndPlay(EndPlayReason);
}

bool UCoastalItemPreviewComponent::CreatePreviewResources()
{
    if (!IsValid(Controller) || !IsValid(Controller->GetRootComponent())) return false;
    // PlayerControllers are hidden actors. Their primitives are culled even
    // when a scene capture explicitly includes them. Keep rendering ownership
    // on one transient, collision-free actor without changing controller flags.
    FActorSpawnParameters Params;
    Params.Owner = Controller;
    Params.ObjectFlags |= RF_Transient;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    RenderOwner = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
    if (!IsValid(RenderOwner)) return false;
    RenderOwner->SetActorEnableCollision(false);
    RenderOwner->SetActorTickEnabled(false);
    PreviewRoot = NewObject<USceneComponent>(RenderOwner, NAME_None, RF_Transient);
    RenderOwner->SetRootComponent(PreviewRoot);
    // Keep preview primitives away from the player and any gameplay capture.
    // The mesh is additionally scene-capture-only and this capture uses ShowOnly.
    PreviewRoot->SetWorldLocation(PreviewWorldLocation);
    PreviewRoot->RegisterComponent();

    PreviewPivot = NewObject<USceneComponent>(RenderOwner, NAME_None, RF_Transient);
    PreviewPivot->SetupAttachment(PreviewRoot);
    PreviewPivot->RegisterComponent();

    PreviewMesh = NewObject<UStaticMeshComponent>(RenderOwner, NAME_None, RF_Transient);
    PreviewMesh->SetupAttachment(PreviewPivot);
    PreviewMesh->SetMobility(EComponentMobility::Movable);
    PreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PreviewMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    PreviewMesh->SetGenerateOverlapEvents(false);
    PreviewMesh->SetSimulatePhysics(false);
    PreviewMesh->SetCastShadow(false);
    PreviewMesh->SetVisibleInSceneCaptureOnly(true);
    PreviewMesh->RegisterComponent();

    RenderTarget = NewObject<UTextureRenderTarget2D>(this, NAME_None, RF_Transient);
    RenderTarget->RenderTargetFormat = RTF_RGBA8;
    RenderTarget->ClearColor = FLinearColor::Transparent;
    RenderTarget->InitAutoFormat(PreviewPixels, PreviewPixels);
    RenderTarget->UpdateResourceImmediate(true);

    PreviewCapture = NewObject<USceneCaptureComponent2D>(RenderOwner, NAME_None, RF_Transient);
    PreviewCapture->SetupAttachment(PreviewRoot);
    PreviewCapture->SetRelativeLocation(FVector(-PreviewDistanceCm, 0.0f, 0.0f));
    PreviewCapture->SetRelativeRotation(FRotator::ZeroRotator);
    PreviewCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
    PreviewCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    PreviewCapture->TextureTarget = RenderTarget;
    // Refresh while the modal is open, including while the gameplay world is
    // paused. Imported materials may finish shader/texture work after opening;
    // a single capture would permanently retain that temporary fallback image.
    PreviewCapture->bCaptureEveryFrame = true;
    PreviewCapture->PrimaryComponentTick.bTickEvenWhenPaused = true;
    PreviewCapture->bCaptureOnMovement = false;
    PreviewCapture->FOVAngle = 30.0f;
    // Keep final-color processing and fixed exposure for the unlit materials.
    // OnRegister resets ShowFlags from the archetype. Persist these overrides
    // through SetShowFlagSettings instead of changing the temporary flags.
    TArray<FEngineShowFlagsSetting> Flags;
    for (const TCHAR* Name : {TEXT("Fog"), TEXT("Atmosphere"), TEXT("PostProcessing")})
    {
        FEngineShowFlagsSetting Flag;
        Flag.ShowFlagName = Name;
        Flag.Enabled = Flag.ShowFlagName == TEXT("PostProcessing");
        Flags.Add(Flag);
    }
    PreviewCapture->SetShowFlagSettings(Flags);
    PreviewCapture->PostProcessSettings.bOverride_AutoExposureMethod = true;
    PreviewCapture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
    PreviewCapture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    PreviewCapture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    PreviewCapture->PostProcessSettings.bOverride_AutoExposureBias = true;
    PreviewCapture->PostProcessSettings.AutoExposureBias = 0.0f;
    PreviewCapture->PostProcessSettings.bOverride_BloomIntensity = true;
    PreviewCapture->PostProcessSettings.BloomIntensity = 0.0f;
    PreviewCapture->RegisterComponent();
    return IsValid(PreviewRoot) && IsValid(PreviewPivot) && IsValid(PreviewMesh)
        && IsValid(PreviewCapture) && IsValid(RenderTarget);
}

void UCoastalItemPreviewComponent::ApplyPose()
{
    if (PreviewPivot)
    {
        PreviewPivot->SetRelativeRotation(FRotator(Pose.pitch, Pose.yaw, 0.0f));
    }
}

bool UCoastalItemPreviewComponent::Matches(FGuid InstanceId, int64 ViewRevision) const
{
    return bOpen && InstanceId.IsValid() && InstanceId == SelectedInstance
        && ViewRevision == SelectedRevision && coastal::ValidPreviewRevision(ViewRevision)
        && IsValid(RenderOwner) && IsValid(PreviewPivot) && IsValid(PreviewMesh) && IsValid(PreviewCapture) && IsValid(RenderTarget);
}
