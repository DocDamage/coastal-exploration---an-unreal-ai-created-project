#include "CoastalCampingActionComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"

void UCoastalCampingActionComponent::LoadShelterAssets()
{
    // Missing optional content must not disable the existing campsite actions.
    LoadedShelterMesh = ShelterPresentationMesh.IsNull() ? nullptr : ShelterPresentationMesh.LoadSynchronous();
    LoadedShelterRest = ShelterRestAnimation.IsNull() ? nullptr : ShelterRestAnimation.LoadSynchronous();
    if (!IsValid(LoadedShelterMesh) || !IsValid(LoadedShelterRest)
        || !IsValid(LoadedShelterMesh->GetSkeleton())
        || LoadedShelterRest->GetSkeleton() != LoadedShelterMesh->GetSkeleton()
        || !FMath::IsFinite(LoadedShelterRest->GetPlayLength())
        || LoadedShelterRest->GetPlayLength() <= 0.0f || LoadedShelterRest->bEnableRootMotion)
    {
        LoadedShelterMesh = nullptr;
        LoadedShelterRest = nullptr;
    }
}

bool UCoastalCampingActionComponent::IsNearShelter() const
{
    return FindNearbyCampsite(ECoastalCampingAction::RestInShelter) != nullptr;
}

UAnimSequence* UCoastalCampingActionComponent::AnimationFor(ECoastalCampingAction Action) const
{
    switch (Action)
    {
    case ECoastalCampingAction::WarmHands: return LoadedWarmHands.Get();
    case ECoastalCampingAction::RestByFire: return LoadedRestByFire.Get();
    case ECoastalCampingAction::RestInShelter: return LoadedShelterRest.Get();
    default: return nullptr;
    }
}

USkeletalMesh* UCoastalCampingActionComponent::MeshFor(ECoastalCampingAction Action) const
{
    switch (Action)
    {
    case ECoastalCampingAction::WarmHands:
    case ECoastalCampingAction::RestByFire: return LoadedPresentationMesh.Get();
    case ECoastalCampingAction::RestInShelter: return LoadedShelterMesh.Get();
    default: return nullptr;
    }
}
