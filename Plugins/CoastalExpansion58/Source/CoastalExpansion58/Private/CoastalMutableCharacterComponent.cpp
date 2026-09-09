#include "CoastalMutableCharacterComponent.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalRetargetAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "MuCO/CustomizableObject.h"
#include "MuCO/CustomizableObjectInstance.h"
#include "MuCO/CustomizableObjectInstanceUsage.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SkeletalMesh.h"
#include "CoastalCombatComponent.h"
#include "CoastalGrappleComponent.h"
#include "Retargeter/IKRetargeter.h"
#include "CoastalUISessionComponent.h"

UCoastalMutableCharacterComponent::UCoastalMutableCharacterComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UCoastalMutableCharacterComponent::BeginPlay()
{
    Super::BeginPlay();
    Character = Cast<ACharacter>(GetOwner());
    if (!Character) return;
    // Action interruption must observe this frame's movement, including a long
    // frame after capture/streaming, rather than the previous stationary state.
    if (auto* Movement = Character->GetCharacterMovement()) AddTickPrerequisiteComponent(Movement);
    Bridge = Character->FindComponentByClass<UCoastalInteractionBridge>();
    Saves = Bridge ? Bridge->GetCoordinator() : nullptr;
    if (Bridge) InteractionHandle = Bridge->OnActionFeedback.AddUObject(this, &UCoastalMutableCharacterComponent::InteractionFeedback);
    if (auto* Mesh = Character->GetMesh())
    { bOriginalHidden = Mesh->bHiddenInGame; OriginalTickOption = Mesh->VisibilityBasedAnimTickOption; }
}

bool UCoastalMutableCharacterComponent::BindingValid() const
{
    auto* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
    return IsRegistered() && GetWorld() && GetWorld()->GetNetMode() == NM_Standalone && IsValid(Character)
        && IsValid(PC) && PC->IsLocalController() && PC->GetPawn() == Character && GetOwner() == Character
        && IsValid(Character->GetMesh()) && IsValid(Bridge) && Bridge->GetOwner() == Character
        && IsValid(Saves) && Bridge->GetCoordinator() == Saves && Saves->HasActiveCampaign()
        && !Saves->IsRecoveryRequired() && !Saves->IsPlayerReturnActive() && !Saves->IsBusy()
        && Saves->GetCampaignId() == Campaign && Saves->GetSessionEpoch() == Epoch;
}

void UCoastalMutableCharacterComponent::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Type, Function);
    if (!IsValid(Definition) || !IsValid(Saves) || !IsValid(Character)) return;
    auto* Graph = Definition->DefaultInstance ? Definition->DefaultInstance->GetCustomizableObject() : nullptr;
    if (!Graph) return;
    if (!Graph->IsCompiled())
    {
#if WITH_EDITOR
        if (!bCompileRequested)
        {
            bCompileRequested = true;
            FCompileParams Params; Params.bAsync = true; Params.bSkipIfCompiled = true;
            Params.CallbackNative.BindWeakLambda(this, [this](const FCompileCallbackParams& Result)
            {
                if (!Result.bCompiled) LastDetail = TEXT("Character data could not compile. Existing appearance saves were preserved.");
                if (Character && Character->GetController())
                    if (auto* UI = Character->GetController()->FindComponentByClass<UCoastalUISessionComponent>()) UI->RefreshCharacterCreator();
            });
            Graph->Compile(Params);
            LastDetail = TEXT("Preparing character customization...");
        }
#else
        LastDetail = TEXT("Cooked character data is unavailable.");
#endif
        return;
    }
    if (Campaign != Saves->GetCampaignId() || Epoch != Saves->GetSessionEpoch())
    {
        EndCharacterEdit(); ClearGenerated();
        Campaign = Saves->GetCampaignId(); Epoch = Saves->GetSessionEpoch();
        bGenerationRequested = false;
        if (Saves->HasActiveCampaign() && Campaign.IsValid())
        {
            if (LoadAppearance()) bGenerationRequested = true;
            else if (LastDetail.IsEmpty()) LastDetail = TEXT("Character definition is not ready or is incompatible.");
        }
    }
    if (!BindingValid()) { if (bEditing) EndCharacterEdit(); StopAction(); UpdateWeaponPose(false); NextIdleTime = GetWorld()->GetTimeSeconds() + 18.0; return; }
    if (bGenerationRequested && !bGenerating)
    { bGenerationRequested = false; RequestGeneration(bEditing ? Draft : Applied); }
    RefreshPresentation();
    UpdateGestures();
    UpdateActions();
    if (bEditing) UpdatePreview();
}

USkeletalMeshComponent* UCoastalMutableCharacterComponent::GetGeneratedBody() const
{ return Parts.IsEmpty() ? nullptr : Parts[0].Get(); }

void UCoastalMutableCharacterComponent::RefreshPresentation()
{
    auto* Source = Character ? Character->GetMesh() : nullptr;
    auto* SourceAsset = Source ? Source->GetSkeletalMeshAsset() : nullptr;
    auto* Found = SourceAsset && Definition ? Definition->Retargeters.Find(SourceAsset->GetSkeleton()) : nullptr;
    const bool Visible = bReady && Found && IsValid(Found->Get());
    if (Source)
    {
        Source->SetHiddenInGame(Visible ? true : bOriginalHidden, false);
        Source->VisibilityBasedAnimTickOption = Visible ? EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones : OriginalTickOption;
    }
    for (const auto& Part : Parts)
    {
        Part->SetHiddenInGame(!Visible, false);
        // The modal's two portrait lights illuminate only the generated art.
        Part->SetLightingChannels(!bEditing, bEditing, false);
        TArray<USceneComponent*> Children; Part->GetChildrenComponents(true, Children);
        for (auto* Child : Children) if (Child->GetOwner() == Character)
        {
            // Rope and hook visibility belongs to the gameplay owner. They are
            // attached to this body for contact, but are not generated body art.
            bool RopeOwned = false;
            for (auto* Parent = Child; Parent && Parent != Part; Parent = Parent->GetAttachParent())
                if (Parent->IsA<URopeComponent>()) { RopeOwned = true; break; }
            if (RopeOwned) continue;
            if (auto* Primitive = Cast<UPrimitiveComponent>(Child))
            { Primitive->SetHiddenInGame(!Visible, false); Primitive->SetLightingChannels(!bEditing, bEditing, false); }
        }
        if (auto* Animation = Cast<UCoastalRetargetAnimInstance>(Part->GetAnimInstance()))
        { Animation->SourceMesh = Source; Animation->Retargeter = Visible ? Found->Get() : nullptr; }
    }
    // Regeneration can finish while gameplay is paused and combat is not ticking.
    if (Combat) Combat->RefreshWeaponAttachment();
    if (Character) if (auto* Grapple = Character->FindComponentByClass<UCoastalGrappleComponent>()) Grapple->RefreshHandAttachment();
}

void UCoastalMutableCharacterComponent::ClearGenerated()
{
    bReady = false;
    if (Combat) Combat->RefreshWeaponAttachment();
    // Move gameplay-owned children off the old generated mesh before its groom
    // children are destroyed. The rope must survive preview, discard and reload.
    if (Character) if (auto* Grapple = Character->FindComponentByClass<UCoastalGrappleComponent>()) Grapple->RefreshHandAttachment();
    StopAction();
    if (Capture) Capture->ClearShowOnlyComponents();
    for (const auto& Usage : Usages) if (Usage) { Usage->AttachTo(nullptr); Usage->SetCustomizableObjectInstance(nullptr); }
    Usages.Reset();
    for (const auto& Part : Parts) if (Part)
    {
        TArray<USceneComponent*> Children; Part->GetChildrenComponents(true, Children);
        for (auto* Child : Children) if (Child->GetOwner() == Character) Child->DestroyComponent();
        Part->DestroyComponent();
    }
    Parts.Reset(); CurrentInstance = nullptr; GeneratedSelections.Reset();
    if (Character && Character->GetMesh())
    { Character->GetMesh()->SetHiddenInGame(bOriginalHidden, false); Character->GetMesh()->VisibilityBasedAnimTickOption = OriginalTickOption; }
}

void UCoastalMutableCharacterComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    RemoveGestureInput();
    EndCharacterEdit(); ClearGenerated(); ClearPendingParts();
    if (Bridge) Bridge->OnActionFeedback.Remove(InteractionHandle);
    if (Combat) Combat->OnCombatPresentation.Remove(CombatHandle);
    PendingInstance = nullptr; bGenerating = false; Campaign.Invalidate();
    Super::EndPlay(Reason);
}
