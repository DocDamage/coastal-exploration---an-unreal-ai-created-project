#include "CoastalMutableCharacterComponent.h"
#include "CoastalRetargetAnimInstance.h"
#include "MuCO/CustomizableObject.h"
#include "MuCO/CustomizableObjectInstance.h"
#include "MuCO/CustomizableObjectInstanceUsage.h"
#include "GameFramework/Character.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SceneCaptureComponent2D.h"
#include "CoastalUISessionComponent.h"
#include "CoastalSaveCoordinator.h"
#include "GameFramework/PlayerController.h"

void UCoastalMutableCharacterComponent::RequestGeneration(const TArray<int32>& Values)
{
    if (!BindingValid() || bGenerating || !ValidateSelections(Values)) return;
    PendingInstance = Definition->DefaultInstance->Clone();
    if (!PendingInstance) { LastDetail = TEXT("Unable to create the character appearance."); return; }
    ClearPendingParts();
    auto* Graph = PendingInstance->GetCustomizableObject();
    if (Graph->GetComponentCount() <= 0 || Graph->GetComponentCount() > 16) return;
    for (int32 I = 0; I < Graph->GetComponentCount(); ++I)
    {
        auto* Part = NewObject<USkeletalMeshComponent>(Character);
        Part->SetupAttachment(Character->GetMesh());
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false); Part->SetHiddenInGame(true);
        Part->SetTickableWhenPaused(true);
        Part->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Character->AddInstanceComponent(Part); Part->RegisterComponent();
        Part->AddTickPrerequisiteComponent(Character->GetMesh());
        auto* Usage = NewObject<UCustomizableObjectInstanceUsage>(this);
        Usage->SetSkipSetReferenceSkeletalMesh(true);
        Usage->SetCustomizableObjectInstance(PendingInstance);
        Usage->SetComponentName(Graph->GetComponentName(I)); Usage->AttachTo(Part);
        PendingParts.Add(Part); PendingUsages.Add(Usage);
    }
    for (int32 I = 0; I < Values.Num(); ++I)
    {
        const auto& Field = Definition->Controls[I];
        if (Field.Kind == ECoastalAppearanceField::Choice)
        {
            PendingInstance->SetEnumParameterSelectedOption(Field.Parameter, Field.Choices[Values[I]]);
            if (!Field.LinkedParameter.IsEmpty()) PendingInstance->SetEnumParameterSelectedOption(Field.LinkedParameter, Field.Choices[Values[I]]);
        }
        else if (Field.Kind == ECoastalAppearanceField::Color)
            PendingInstance->SetColorParameterSelectedOption(Field.Parameter, Field.Colors[Values[I]]);
        else PendingInstance->SetFloatParameterSelectedOption(Field.Parameter, Field.Minimum + Field.Step * Values[I]);
    }
    PendingSelections = Values; PendingCampaign = Campaign; PendingEpoch = Epoch;
    bGenerating = true; LastDetail = TEXT("Generating appearance...");
    FInstanceUpdateDelegate Callback;
    Callback.BindDynamic(this, &UCoastalMutableCharacterComponent::Generated);
    PendingInstance->UpdateSkeletalMeshAsyncResult(Callback, true, true);
}

void UCoastalMutableCharacterComponent::Generated(const FUpdateContext& Result)
{
    if (Result.Instance != PendingInstance) return;
    auto* Completed = PendingInstance.Get();
    PendingInstance = nullptr; bGenerating = false;
    if (Character && Character->GetController())
        if (auto* UI = Character->GetController()->FindComponentByClass<UCoastalUISessionComponent>()) UI->RefreshCharacterCreator();
    if (!BindingValid() || PendingCampaign != Campaign || PendingEpoch != Epoch)
    {
        ClearPendingParts();
        if (IsRegistered() && Saves && !Saves->IsRecoveryRequired() && PendingCampaign == Campaign && PendingEpoch == Epoch)
            bGenerationRequested = true;
        return;
    }
    if (PendingSelections != (bEditing ? Draft : Applied)) { ClearPendingParts(); bGenerationRequested = true; return; }
    if (Result.UpdateResult != EUpdateResult::Success || !Completed)
    { ClearPendingParts(); LastDetail = TEXT("Appearance generation failed. The previous character is retained."); return; }
    auto* Graph = Completed->GetCustomizableObject();
    const int32 Count = Graph ? Graph->GetComponentCount() : 0;
    if (Count <= 0 || Count > 16) { LastDetail = TEXT("Generated character has no supported mesh components."); return; }
    for (int32 I = 0; I < Count; ++I)
    {
        const FName Name = Graph->GetComponentName(I);
        auto* Asset = Completed->GetSkeletalMeshComponentSkeletalMesh(Name);
        if (!Asset || !Asset->GetSkeleton() || !PendingParts.IsValidIndex(I))
        {
            ClearPendingParts();
            LastDetail = TEXT("Generated mesh has no skeleton. Previous character retained."); return;
        }
        auto* Part = PendingParts[I].Get();
        Part->SetSkeletalMesh(Asset);
        const auto Materials = Completed->GetSkeletalMeshComponentOverrideMaterials(Name);
        for (int32 M = 0; M < Materials.Num(); ++M) Part->SetMaterial(M, Materials[M]);
        Part->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        Part->SetAnimInstanceClass(UCoastalRetargetAnimInstance::StaticClass());
        Part->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    }
    ClearGenerated();
    CurrentInstance = Completed; Parts = MoveTemp(PendingParts); Usages = MoveTemp(PendingUsages);
    GeneratedSelections = PendingSelections; bReady = true;
    bGenerationRequested = false;
    LastDetail = !bCanWrite ? TEXT("A damaged or incompatible appearance file was preserved. A valid earlier appearance was restored; saving is disabled.")
        : bEditing ? TEXT("Preview ready. Apply and save to keep this appearance.") : TEXT("Saved character appearance restored.");
    RefreshPresentation();
    if (Capture) for (const auto& Part : Parts)
    {
        Capture->ShowOnlyComponent(Part);
        TArray<USceneComponent*> Children; Part->GetChildrenComponents(true, Children);
        for (auto* Child : Children) if (Child->GetOwner() == Character)
            if (auto* Primitive = Cast<UPrimitiveComponent>(Child)) Capture->ShowOnlyComponent(Primitive);
    }
}

void UCoastalMutableCharacterComponent::ClearPendingParts()
{
    for (const auto& Usage : PendingUsages) if (Usage) { Usage->AttachTo(nullptr); Usage->SetCustomizableObjectInstance(nullptr); }
    PendingUsages.Reset();
    for (const auto& Part : PendingParts) if (Part)
    {
        TArray<USceneComponent*> Children; Part->GetChildrenComponents(true, Children);
        for (auto* Child : Children) if (Child->GetOwner() == Character) Child->DestroyComponent();
        Part->DestroyComponent();
    }
    PendingParts.Reset();
}
