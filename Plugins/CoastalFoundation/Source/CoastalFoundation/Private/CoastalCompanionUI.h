#pragma once

#include "CoastalCompanionCommands.h"
#include "EngineUtils.h"

inline ICoastalCompanionCommands* FindCoastalCompanionCommands(UWorld* World)
{
    if (!World) return nullptr;
    ICoastalCompanionCommands* Found = nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (auto* Commands = Cast<ICoastalCompanionCommands>(*It))
        {
            // Ambiguous authority must not offer a command to an arbitrary dog.
            if (Found) return nullptr;
            Found = Commands;
        }
    }
    return Found;
}
