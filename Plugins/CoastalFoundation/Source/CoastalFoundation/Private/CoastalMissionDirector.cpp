#include "CoastalMissionDirector.h"
#include "FirstSignalComponent.h"
#include "CoastalSaveCoordinator.h"
#include "Components/SceneComponent.h"
ACoastalMissionDirector::ACoastalMissionDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    FirstSignal = CreateDefaultSubobject<UFirstSignalComponent>(TEXT("FirstSignal"));
    Saves = CreateDefaultSubobject<UCoastalSaveCoordinator>(TEXT("CampaignSaves"));
}
