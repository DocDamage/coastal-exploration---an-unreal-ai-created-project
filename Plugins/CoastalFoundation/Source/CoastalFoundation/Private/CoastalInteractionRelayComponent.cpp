#include "CoastalInteractionRelayComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalWorldObject.h"
#include "GameFramework/PlayerController.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Templates/UnrealTemplate.h"

UCoastalInteractionRelayComponent::UCoastalInteractionRelayComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}
bool UCoastalInteractionRelayComponent::LiveBinding() const
{
    return bInitialized && !bStopped && IsRegistered() && IsValid(Controller) && Controller->IsLocalController()
        && IsValid(Bridge) && Bridge->IsRegistered() && IsValid(Bridge->GetCoordinator()) && Bridge->GetOwner() == Controller->GetPawn();
}
bool UCoastalInteractionRelayComponent::InitializeRelay(UCoastalInteractionBridge* ActualBridge)
{
    auto* PC = Cast<APlayerController>(GetOwner());
    if (!IsInGameThread() || bInitialized || bStopped || !IsRegistered()
        || !GetWorld() || !GetWorld()->IsGameWorld() || GetWorld()->GetNetMode() != NM_Standalone || !IsValid(PC)
        || !PC->IsLocalController() || !PC->GetLocalPlayer() || !IsValid(ActualBridge)
        || ActualBridge->GetOwner() != PC->GetPawn() || ActualBridge->GetWorld() != GetWorld()
        || !IsValid(ActualBridge->GetCoordinator()) || !ActualBridge->GetCoordinator()->IsConfigured())
    { LastDetail = TEXT("Initialize one registered local-controller relay after bridge configuration."); return false; }
    TArray<UCoastalInteractionRelayComponent*> Owners; PC->GetComponents(Owners);
    if (Owners.Num() != 1 || Owners[0] != this ||
        (InputOwner != ECoastalInteractInputOwner::VendorEvents && InputOwner != ECoastalInteractInputOwner::NativeEnhancedInput))
    { LastDetail = TEXT("Duplicate relay or unsupported input owner."); return false; }
    Controller = PC; Bridge = ActualBridge; BoundInputOwner = InputOwner;
    if (BoundInputOwner == ECoastalInteractInputOwner::NativeEnhancedInput && !InstallNativeInput())
    { RemoveNativeInput(); Controller = nullptr; Bridge = nullptr; return false; }
    bInitialized = true; Synchronize();
    LastDetail = TEXT("Relay initialized. Real Hyper focus and prompt wiring remain required.");
    return true;
}
bool UCoastalInteractionRelayComponent::Synchronize()
{
    const bool bLive = LiveBinding();
    Epoch = bLive ? Bridge->GetCoordinator()->GetSessionEpoch() : 0;
    Revision = bLive ? Bridge->GetInputRevision() : 0;
    const bool bAllowed = bLive && Bridge->AllowsWorldInput();
    if (Intent.Synchronize(Epoch, Revision, bAllowed, GFrameCounter))
    { Lease.Clear(); FocusedTarget.Reset(); }
    return bAllowed;
}
bool UCoastalInteractionRelayComponent::UpdateFocusedTarget(ACoastalWorldObject* Target)
{
    if (!IsInGameThread() || bPublishing || bDispatching) return false;
    if (!Synchronize()) return false;
    if (!IsValid(Target)) { Lease.Clear(); FocusedTarget.Reset(); return Target == nullptr; }
    if (!Bridge->GetCoordinator()->OwnsObject(Target))
    { Lease.Clear(); FocusedTarget.Reset(); return false; }
    if (!Lease.Set(Target->GetUniqueID(), Epoch, Revision, GFrameCounter)) return false;
    FocusedTarget = Target; return true;
}
void UCoastalInteractionRelayComponent::ClearFocusedTarget(ACoastalWorldObject* ExpectedTarget)
{
    if (!IsInGameThread() || bPublishing || bDispatching || !IsValid(ExpectedTarget)) return;
    if (Lease.ClearExpected(ExpectedTarget->GetUniqueID())) FocusedTarget.Reset();
}
FCoastalInteractionOffer UCoastalInteractionRelayComponent::GetCurrentOffer() const
{
    auto* Target = FocusedTarget.Get();
    if (!IsInGameThread() || !LiveBinding() || !Intent.Enabled() || !IsValid(Target)) return {};
    auto* Saves = Bridge->GetCoordinator();
    if (!Lease.Valid(Target->GetUniqueID(), Saves->GetSessionEpoch(), Bridge->GetInputRevision(), GFrameCounter)) return {};
    return Bridge->PreviewInteraction(Target);
}
ECoastalActionResult UCoastalInteractionRelayComponent::Press()
{
    if (!IsInGameThread() || bPublishing || bDispatching) return ECoastalActionResult::SuppressedInput;
    Synchronize();
    if (!Intent.Press(GFrameCounter)) return ECoastalActionResult::SuppressedInput;
    auto* Target = FocusedTarget.Get();
    if (!IsValid(Target) || !Lease.Valid(Target->GetUniqueID(), Epoch, Revision, GFrameCounter))
    { LastDetail = TEXT("Hyper focus sample is absent or expired. Supply the current target, release, then press again."); return ECoastalActionResult::StaleFocus; }
    // Prompt data is advisory. Revalidate reach/UI/session/provider in the bridge at commit time.
    TGuardValue<bool> Dispatch(bDispatching, true);
    return Bridge->TryInteract(Target);
}
ECoastalActionResult UCoastalInteractionRelayComponent::SubmitExternalInput(bool bIsDown)
{
    if (!IsInGameThread() || bPublishing || bDispatching || !bInitialized ||
        BoundInputOwner != ECoastalInteractInputOwner::VendorEvents) return ECoastalActionResult::SuppressedInput;
    if (bIsDown) return Press();
    Synchronize(); Intent.Released(); return ECoastalActionResult::SuppressedInput;
}
void UCoastalInteractionRelayComponent::PublishOffer()
{
    const auto Offer = GetCurrentOffer();
    if (Offer.SamePresentation(PublishedOffer)) return;
    PublishedOffer = Offer;
    TGuardValue<bool> Publishing(bPublishing, true);
    OnOfferChanged.Broadcast(Offer);
}
void UCoastalInteractionRelayComponent::TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Delta, TickType, TickFunction);
    if (!bInitialized || bStopped) return;
    Synchronize();
    if (BoundInputOwner == ECoastalInteractInputOwner::NativeEnhancedInput && !NativeKeysDown()) Intent.Released();
    PublishOffer();
}
void UCoastalInteractionRelayComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    bStopped = true; Lease.Clear(); FocusedTarget.Reset(); Intent.Cancel();
    RemoveNativeInput();
    if (bInitialized)
    {
        PublishedOffer = {};
        TGuardValue<bool> Publishing(bPublishing, true);
        OnOfferChanged.Broadcast(PublishedOffer);
    }
    bInitialized = false; Controller = nullptr; Bridge = nullptr;
    Super::EndPlay(Reason);
}
