#include "CoastalCharacterSubsystem.h"
#include "CoastalMutableCharacterComponent.h"
#include "CoastalGrappleComponent.h"
#include "CoastalZiplineRiderComponent.h"
#include "CoastalProneComponent.h"
#include "CoastalInteractionBridge.h"
#include "CoastalSaveCoordinator.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

bool UCoastalCharacterSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{ return Type == EWorldType::Game || Type == EWorldType::PIE; }

TStatId UCoastalCharacterSubsystem::GetStatId() const
{ RETURN_QUICK_DECLARE_CYCLE_STAT(CoastalCharacterSubsystem, STATGROUP_Tickables); }

void UCoastalCharacterSubsystem::Tick(float Delta)
{
    if (bAttempted || !GetWorld() || !GetWorld()->HasBegunPlay() || GetWorld()->GetNetMode() != NM_Standalone) return;
    auto* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
    auto* Bridge = Player ? Player->FindComponentByClass<UCoastalInteractionBridge>() : nullptr;
    auto* Saves = Bridge ? Bridge->GetCoordinator() : nullptr;
    if (!Saves || !Saves->IsConfigured() || !Saves->HasActiveCampaign()) return;
    bAttempted = true;
    if (Player->FindComponentByClass<UCoastalMutableCharacterComponent>()) return;
    auto* Definition = LoadObject<UCoastalCharacterDefinition>(nullptr,
        TEXT("/Game/Coastal/Character/DA_CoastalCharacter.DA_CoastalCharacter"));
    if (!Definition) return;
    auto* Component = NewObject<UCoastalMutableCharacterComponent>(Player);
    Component->Definition = Definition;
    Player->AddInstanceComponent(Component); Component->RegisterComponent();
    if (!Player->FindComponentByClass<UCoastalGrappleComponent>())
    {
        auto* Grapple = NewObject<UCoastalGrappleComponent>(Player);
        Player->AddInstanceComponent(Grapple); Grapple->RegisterComponent();
    }
    if (!Player->FindComponentByClass<UCoastalZiplineRiderComponent>())
    {
        auto* Rider = NewObject<UCoastalZiplineRiderComponent>(Player);
        Player->AddInstanceComponent(Rider); Rider->RegisterComponent();
        Component->AddTickPrerequisiteComponent(Rider);
    }
    if (!Player->FindComponentByClass<UCoastalProneComponent>())
    {
        auto* Prone = NewObject<UCoastalProneComponent>(Player);
        Player->AddInstanceComponent(Prone); Prone->RegisterComponent();
        Component->AddTickPrerequisiteComponent(Prone);
    }
}
