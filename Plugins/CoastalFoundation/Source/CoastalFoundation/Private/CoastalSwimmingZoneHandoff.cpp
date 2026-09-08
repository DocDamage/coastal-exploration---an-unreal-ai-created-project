#include "CoastalSwimmingComponent.h"

#include "CoastalSwimmingZone.h"
#include "Core/SwimmingRules.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

ACoastalSwimmingZone* UCoastalSwimmingComponent::ResolveCurrentZone()
{
    ACoastalSwimmingZone* Zone = CurrentZone();
    if (IsValid(Zone) || !bSwimming || !IsValid(Character) || !IsValid(Movement)
        || !GetWorld() || Movement->GetPhysicsVolume() != GetWorld()->GetDefaultPhysicsVolume())
        return Zone;

    ACoastalSwimmingZone* Best = nullptr;
    for (TActorIterator<ACoastalSwimmingZone> It(GetWorld()); It; ++It)
    {
        ACoastalSwimmingZone* Candidate = *It;
        const bool bValid = IsValid(Candidate) && Candidate->IsAuthoredCorrectly();
        const bool bOverlaps = bValid && Candidate->OverlapsCharacter(Character);
        if (coastal::AllowsDefaultZoneFallback(bSwimming, true, bValid, bOverlaps)
            && (!IsValid(Best) || Candidate->Priority > Best->Priority))
            Best = Candidate;
    }
    UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
    if (IsValid(Best) && IsValid(Capsule))
        Capsule->SetPhysicsVolume(Best, true);
    return CurrentZone();
}
