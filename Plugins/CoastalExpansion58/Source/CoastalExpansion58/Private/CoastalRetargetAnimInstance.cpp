#include "CoastalRetargetAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_RetargetPoseFromMesh.h"
#include "AnimNodes/AnimNode_Slot.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "AnimNodes/AnimNode_LayeredBoneBlend.h"
#include "Animation/AnimSequence.h"

namespace
{
struct FCoastalRetargetProxy : FAnimInstanceProxy
{
    FAnimNode_RetargetPoseFromMesh Node;
    FAnimNode_Slot Slot;
    FAnimNode_SequencePlayer_Standalone WeaponPose;
    FAnimNode_Slot WeaponSlot;
    FAnimNode_LayeredBoneBlend UpperBody;
    FAnimNode_SequencePlayer_Standalone ActivityPose;
    FAnimNode_LayeredBoneBlend ActivityBody;
    explicit FCoastalRetargetProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance)
    {
        Node.RetargetFrom = ERetargetSourceMode::CustomSkeletalMeshComponent;
        Slot.SlotName = TEXT("CoastalAction");
        Slot.bAlwaysUpdateSourcePose = true;
        WeaponPose.SetLoopAnimation(true);
        WeaponSlot.SlotName = TEXT("CoastalUpperBody");
        WeaponSlot.bAlwaysUpdateSourcePose = true;
        WeaponSlot.Source.SetLinkNode(&WeaponPose);
        ActivityPose.SetLoopAnimation(true);
        ActivityBody.BasePose.SetLinkNode(&Node);
        ActivityBody.AddPose();
        ActivityBody.BlendPoses[0].SetLinkNode(&ActivityPose);
        FBranchFilter FullBody;
        FullBody.BoneName = TEXT("root"); FullBody.BlendDepth = 0;
        ActivityBody.LayerSetup[0].BranchFilters.Add(FullBody);
        ActivityBody.BlendWeights[0] = 0.f;
        UpperBody.BasePose.SetLinkNode(&ActivityBody);
        UpperBody.AddPose();
        UpperBody.BlendPoses[0].SetLinkNode(&WeaponSlot);
        FBranchFilter Spine;
        Spine.BoneName = TEXT("spine_01"); Spine.BlendDepth = 3;
        UpperBody.LayerSetup[0].BranchFilters.Add(Spine);
        UpperBody.bMeshSpaceRotationBlend = true;
        UpperBody.BlendWeights[0] = 0.f;
        Slot.Source.SetLinkNode(&UpperBody);
    }
    // Register a real graph root so Unreal advances graph counters and slot
    // evaluation through the same lifecycle as an animation blueprint.
    virtual FAnimNode_Base* GetCustomRootNode() override { return &Slot; }
    virtual void PreUpdate(UAnimInstance* Instance, float Delta) override
    {
        FAnimInstanceProxy::PreUpdate(Instance, Delta);
        auto* Coastal = CastChecked<UCoastalRetargetAnimInstance>(Instance);
        Node.SourceMeshComponent = Coastal->SourceMesh;
        Node.IKRetargeterAsset = Coastal->Retargeter;
        Node.PreUpdate(Instance);
        if (Coastal->ActivityPose && ActivityPose.GetSequence() != Coastal->ActivityPose.Get())
        {
            ActivityPose.SetSequence(Coastal->ActivityPose);
            ActivityPose.SetAccumulatedTime(0.f);
        }
        Coastal->ActivityWeight = FMath::FInterpConstantTo(ActivityBody.BlendWeights[0],
            Coastal->ActivityPose ? 1.f : 0.f, Delta, 6.f);
        ActivityBody.BlendWeights[0] = Coastal->ActivityWeight;
        ActivityPose.SetLoopAnimation(!Coastal->bActivityTimeDriven);
        if (Coastal->bActivityTimeDriven)
            ActivityPose.SetAccumulatedTime(FMath::Max(0.f, Coastal->ActivityTime - Delta));
        WeaponPose.SetLoopAnimation(Coastal->bWeaponPoseLooping);
        if (Coastal->WeaponPose && WeaponPose.GetSequence() != Coastal->WeaponPose.Get())
        {
            WeaponPose.SetSequence(Coastal->WeaponPose);
            WeaponPose.SetAccumulatedTime(0.f);
        }
        Coastal->WeaponPoseTime = WeaponPose.GetAccumulatedTime();
        UpperBody.BlendWeights[0] = FMath::FInterpConstantTo(UpperBody.BlendWeights[0],
            Coastal->WeaponPose ? Coastal->WeaponWeight : 0.f, Delta, 8.f);
    }
};
}
FAnimInstanceProxy* UCoastalRetargetAnimInstance::CreateAnimInstanceProxy()
{ return new FCoastalRetargetProxy(this); }
void UCoastalRetargetAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy)
{ delete Proxy; }
