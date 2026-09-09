#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ItemPreviewRules.h"
#include "CoastalItemPreviewComponent.generated.h"

class APlayerController;
class USceneComponent;
class USceneCaptureComponent2D;
class UStaticMesh;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

// A local inventory-detail preview. It owns only transient rendering objects and
// never reads or writes AGIS, campaign state, world-object state, input mappings,
// the gameplay camera, collision, physics, or a mesh's source transform.
UCLASS(ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalItemPreviewComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalItemPreviewComponent();

    // Add only to the local PlayerController after the UI has established sole
    // UI-only input ownership. A missing local controller fails closed.
    UFUNCTION(BlueprintCallable, Category="Coastal|Item Preview")
    bool InitializePreview(APlayerController* InController);

    // Caller must freshly obtain this exact instance and revision from AGIS before
    // opening. The mesh is a presentation reference; it is never moved or changed.
    UFUNCTION(BlueprintCallable, Category="Coastal|Item Preview")
    bool OpenPreview(FGuid InstanceId, int64 ViewRevision, UStaticMesh* Mesh);

    // Explicit rotation deltas in degrees. UI validates the selected GUID/revision
    // again before every call; stale samples are refused here too.
    UFUNCTION(BlueprintCallable, Category="Coastal|Item Preview")
    bool RotatePreview(FGuid InstanceId, int64 ViewRevision, float YawDegrees, float PitchDegrees);

    // Safe repeatedly and synchronously releases the capture target and components.
    UFUNCTION(BlueprintCallable, Category="Coastal|Item Preview")
    void ClosePreview();

    // A campaign/recovery epoch cannot retain a selected carried-item preview.
    UFUNCTION(BlueprintCallable, Category="Coastal|Item Preview")
    void ResetForEpoch(int64 NewEpoch);

    UFUNCTION(BlueprintPure, Category="Coastal|Item Preview")
    bool IsPreviewOpen() const { return bOpen; }
    UFUNCTION(BlueprintPure, Category="Coastal|Item Preview")
    UTextureRenderTarget2D* GetPreviewTexture() const { return RenderTarget; }
    UFUNCTION(BlueprintPure, Category="Coastal|Item Preview")
    FGuid PreviewedInstance() const { return SelectedInstance; }
    UFUNCTION(BlueprintPure, Category="Coastal|Item Preview")
    int64 PreviewedRevision() const { return SelectedRevision; }

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    static constexpr int32 PreviewPixels = 384;
    static constexpr float PreviewDistanceCm = 300.0f;
    static const FVector PreviewWorldLocation;
    bool CreatePreviewResources();
    void ApplyPose();
    bool Matches(FGuid InstanceId, int64 ViewRevision) const;

    UPROPERTY(Transient) TObjectPtr<APlayerController> Controller;
    UPROPERTY(Transient) TObjectPtr<AActor> RenderOwner;
    UPROPERTY(Transient) TObjectPtr<USceneComponent> PreviewRoot;
    UPROPERTY(Transient) TObjectPtr<USceneComponent> PreviewPivot;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> PreviewMesh;
    UPROPERTY(Transient) TObjectPtr<USceneCaptureComponent2D> PreviewCapture;
    UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> RenderTarget;
    FGuid SelectedInstance;
    int64 SelectedRevision = -1;
    int64 Epoch = -1;
    coastal::ItemPreviewPose Pose;
    bool bInitialized = false;
    bool bOpen = false;
    bool bClosing = false;
};
