#include "CoastalInteractionOffer.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalWorldObject.h"
#include "CoastalInventoryAdapter.h"
#include "FirstSignalComponent.h"
#include "CoastalDestinationQuestRules.h"
#include "CoastalStoryLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

bool UCoastalInteractionBridge::AllowsWorldInput() const
{
    if (!IsInGameThread() || !GetWorld() || !GetWorld()->IsGameWorld() || !IsValid(Saves)) return false;
    const auto* Character = Cast<ACharacter>(GetOwner());
    const auto* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
    coastal::InteractionContext C;
    C.configured = IsValid(Saves) && Saves->IsConfigured();
    C.campaign = IsValid(Saves) && Saves->HasActiveCampaign();
    C.recovery = (IsValid(Saves) && Saves->IsRecoveryRequired()) || InputRevision == MAX_uint64;
    C.busy = IsValid(Saves) && Saves->IsBusy();
    C.menu = !UIBlockers.IsEmpty(); C.paused = UGameplayStatics::IsGamePaused(this);
    C.samePawn = IsValid(PC) && PC->IsLocalController() && PC->GetPawn() == Character;
    return coastal::WorldPermissionFor(C) == coastal::WorldPermission::Allowed;
}
FCoastalInteractionOffer UCoastalInteractionBridge::PreviewInteraction(ACoastalWorldObject* Target) const
{
    FCoastalInteractionOffer Offer;
    if (!AllowsWorldInput() || !IsValid(Target) || !Saves->OwnsObject(Target)) return Offer;
    using coastal::OfferKind; using coastal::OfferVerb;
    OfferKind Kind = OfferKind::Unknown;
    switch (Target->Kind)
    {
    case ECoastalObjectKind::Door: Kind = OfferKind::Door; break;
    case ECoastalObjectKind::Pickup: Kind = OfferKind::Pickup; break;
    case ECoastalObjectKind::Storage: Kind = OfferKind::Storage; break;
    case ECoastalObjectKind::Radio: Kind = OfferKind::Radio; break;
    case ECoastalObjectKind::Discovery: Kind = OfferKind::Discovery; break;
    case ECoastalObjectKind::MaintenanceNote: Kind = OfferKind::Note; break;
    default: return Offer;
    }
    const auto State = Saves->GetMission()->ExportSnapshot();
    const auto Verb = coastal::ChooseOffer(Kind, Target->IsActive(), State.bRadioInspected, State.bRadioRepaired, State.bMessageHeard);
    if (Verb == OfferVerb::None) return Offer;
    const TCHAR* Label = TEXT("");
    switch (Verb)
    {
    case OfferVerb::OpenDoor: Label = TEXT("Open door"); break;
    case OfferVerb::CloseDoor: Label = TEXT("Close door"); break;
    case OfferVerb::Take: Label = Target->bPersistentPickupContainer ? TEXT("Collect supplies") : TEXT("Take item"); break;
    case OfferVerb::OpenStorage: Label = TEXT("Open storage"); break;
    case OfferVerb::InspectRadio: Label = TEXT("Inspect radio"); break;
    case OfferVerb::RepairRadio: Label = TEXT("Repair radio"); break;
    case OfferVerb::Listen: Label = TEXT("Listen to signal"); break;
    case OfferVerb::Replay: Label = TEXT("Replay signal"); break;
    case OfferVerb::ReadDiscovery: case OfferVerb::RereadDiscovery:
        Offer.ActionText = UCoastalStoryLibrary::DiscoveryAction(Target->WorldId, Target->IsActive()); break;
    case OfferVerb::ReadNote: Label = TEXT("Read maintenance note"); break;
    case OfferVerb::RereadNote: Label = TEXT("Review maintenance note"); break;
    default: return Offer;
    }
    Offer.WorldId = Target->WorldId;
    if (Offer.ActionText.IsEmpty()) Offer.ActionText = FText::FromString(Label);
    Offer.bVisible = true;
    auto Result = CheckTarget(Target, true);
    const bool bDestinationLocked = Result == ECoastalActionResult::Applied
        && !FCoastalDestinationQuestRules::CanRead(Target->WorldId, State.bMessageHeard, Saves->GetJournal());
    if (bDestinationLocked) Result = ECoastalActionResult::Failed;
    if (Result == ECoastalActionResult::Applied && Verb == OfferVerb::RepairRadio)
    {
        const auto Availability = Saves->GetInventory()->CheckRequirements(Saves->GetMission()->GetRepairRequirements());
        if (Availability == ECoastalAvailability::MissingItems) Result = ECoastalActionResult::MissingItems;
        else if (Availability == ECoastalAvailability::NotConfigured) Result = ECoastalActionResult::NotConfigured;
        else if (Availability != ECoastalAvailability::Available) Result = ECoastalActionResult::Failed;
    }
    if (Result == ECoastalActionResult::Applied &&
        ((Verb == OfferVerb::OpenStorage && !OnStorageRequested.IsBound()) ||
        ((Verb == OfferVerb::Listen || Verb == OfferVerb::Replay) && !OnTranscriptRequested.IsBound())))
        Result = ECoastalActionResult::NotConfigured;
    Offer.bCanInteract = Result == ECoastalActionResult::Applied;
    const TCHAR* Detail = TEXT("");
    if (bDestinationLocked)
    {
        Offer.Detail = NSLOCTEXT("CoastalInteraction", "DestinationLocked",
            "Follow the current journal objective before reading this record.");
    }
    else switch (Result)
    {
    case ECoastalActionResult::Applied: break;
    case ECoastalActionResult::TooFar: Detail = TEXT("Move closer."); break;
    case ECoastalActionResult::Occluded: Detail = TEXT("The path to this object is blocked."); break;
    case ECoastalActionResult::MissingItems: Detail = TEXT("Carry the radio battery and marine fuse in your backpack."); break;
    case ECoastalActionResult::NotConfigured: Detail = TEXT("Development integration is not configured."); break;
    default: Detail = TEXT("This interaction is currently unavailable."); break;
    }
    if (Offer.Detail.IsEmpty()) Offer.Detail = FText::FromString(Detail);
    return Offer;
}
