#include "CoastalSoundscape.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalUISessionComponent.h"
#include "CoastalWorldObject.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"

bool UCoastalSoundscape::Initialize(UCoastalUISessionComponent* UI,UCoastalAudioOptionsComponent* Routing)
{
    auto* PC=Cast<APlayerController>(GetOwner()); auto* World=GetWorld();
    if(bReady || bReleased || !IsRegistered() || !IsComponentTickEnabled() || !PC || !PC->IsLocalController()
        || !PC->GetLocalPlayer() || !PC->GetPawn() || !World || !World->IsGameWorld()
        || World->GetNetMode()!=NM_Standalone || World->GetNumPlayerControllers()!=1
        || !World->GetMapName().EndsWith(TEXT("L_FirstSignal")) || !IsValid(UI) || UI->GetOwner()!=PC
        || !UI->IsInitialized() || !IsValid(Routing) || Routing->GetOwner()!=PC || !Routing->IsAudioReady())return false;
    TArray<UCoastalSoundscape*> Owners; PC->GetComponents(Owners);
    if(Owners.Num()!=1)return false;
    for(const TCHAR* Id:{TEXT("coast"),TEXT("cabin"),TEXT("village"),TEXT("camp"),TEXT("workshop"),
        TEXT("prison"),TEXT("ruins"),TEXT("thunder_far"),TEXT("thunder_soft")})
    {
        const FString Path=FString::Printf(TEXT("/Game/Coastal/Audio/M3Soundscape/SW_M3_%s.SW_M3_%s"),Id,Id);
        auto* Sound=LoadObject<USoundBase>(nullptr,*Path);
        const bool ThunderClip=FString(Id).StartsWith(TEXT("thunder_"));
        if(!IsValid(Sound) || !Sound->IsPlayable() || !Sound->IsPlayWhenSilent() || Sound->IsLooping()==ThunderClip
            || (!Sound->IsLooping() && (!FMath::IsFinite(Sound->GetDuration()) || Sound->GetDuration()<=0 || Sound->GetDuration()>60)))
        {Clips.Reset();return false;}
        Clips.Add(FName(Id),Sound);
    }
    Zones.Reset();
    struct FBinding { const TCHAR* Id; const TCHAR* Score; float Radius; };
    const FBinding Bindings[]={
        {TEXT("world.coastal_records.village"),TEXT("village"),3600},
        {TEXT("world.coastal_records.prison"),TEXT("prison"),2500},
        {TEXT("world.coastal_records.baelo"),TEXT("ruins"),2400},
        {TEXT("world.outer_coast.atlantis"),TEXT("ruins"),4500},
        {TEXT("world.outer_coast.station"),TEXT("prison"),2300},
        {TEXT("world.north_reach.powell_order"),TEXT("workshop"),2200},
        {TEXT("world.test.radio"),TEXT("cabin"),1100}};
    for(const auto& Binding:Bindings)
    {
        ACoastalWorldObject* Found=nullptr;
        for(TActorIterator<ACoastalWorldObject> It(World);It;++It)
            if(It->WorldId==FName(Binding.Id)){if(Found){Clips.Reset();return false;}Found=*It;}
        if(!Found){Clips.Reset();return false;}
        Zones.Add({FName(Binding.Score),Found->GetActorLocation(),Binding.Radius});
    }
    // The existing authored campsite marker is the location authority.
    AActor* Camp=nullptr;
    for(TActorIterator<AActor> It(World);It;++It)
        if(It->ActorHasTag(TEXT("Coastal.Campsite"))){if(Camp){Clips.Reset();return false;}Camp=*It;}
    if(!Camp){Clips.Reset();return false;}
    Zones.Insert({FName(TEXT("camp")),Camp->GetActorLocation(),700},0);
    Device=World->GetAudioDevice(); if(!Device.IsValid()){Clips.Reset();return false;}
    Menus=UI; Options=Routing; Pawn=PC->GetPawn();
    MusicBus=Routing->GetAmbienceClassForPlayback(); ThunderBus=Routing->GetEffectsClassForPlayback();
    ReleaseHandle=Routing->OnRoutingReleased.AddUObject(this,&UCoastalSoundscape::Release);
    AddTickPrerequisiteComponent(UI); bReady=true;
    return true;
}
