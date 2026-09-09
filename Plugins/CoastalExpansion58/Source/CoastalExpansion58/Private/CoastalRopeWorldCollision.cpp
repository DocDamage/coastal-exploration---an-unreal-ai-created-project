#include "CoastalRopeWorldCollision.h"
#include "Collision/RopeController.h"
#include "Collision/RopeStaticBodyProvider.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PhysicsEngine/BodySetup.h"

bool ConfigureCoastalRopeWorldCollision(UWorld* World)
{
    if (!World || World->GetNetMode() != NM_Standalone) return false;
    bool Configured = false;
    for (TActorIterator<ARopeController> Controller(World); Controller; ++Controller)
    {
        if (!Controller->StaticBodyProvider) continue;
        // This terrain deliberately uses triangle collision. The plugin extracts
        // its obsolete aggregate hull anyway, filling the playable coastline.
        // Keep the terrain's real native collision and the plugin's world distance
        // field; exclude only that unused hull from its analytic provider.
        for (TActorIterator<AStaticMeshActor> Actor(World); Actor; ++Actor)
        {
            auto* Component = Actor->GetStaticMeshComponent();
            UStaticMesh* Mesh = Component ? Component->GetStaticMesh().Get() : nullptr;
            if (Mesh && Mesh->GetPathName() == TEXT("/Game/Coastal/M3/Geometry/SM_M3CoastalTerrain_CollisionFixed.SM_M3CoastalTerrain_CollisionFixed")
                && Mesh->GetBodySetup() && Mesh->GetBodySetup()->CollisionTraceFlag == CTF_UseComplexAsSimple)
                Controller->StaticBodyProvider->IgnoredComponents.AddUnique(Component);
        }
        Configured = true;
    }
    return Configured;
}
