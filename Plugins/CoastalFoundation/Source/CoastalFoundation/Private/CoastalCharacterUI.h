#pragma once
#include "CoastalCharacterCreator.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

inline ICoastalCharacterCreator* FindCoastalCharacterCreator(AActor* Player)
{
    if (!IsValid(Player)) return nullptr;
    ICoastalCharacterCreator* Found = nullptr;
    for (auto* Component : Player->GetComponents())
        if (auto* Creator = Cast<ICoastalCharacterCreator>(Component))
        {
            if (Found) return nullptr;
            Found = Creator;
        }
    return Found;
}
