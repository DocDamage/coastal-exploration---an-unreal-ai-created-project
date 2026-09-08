#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CoastalCampaignTypes.h"
#include "CoastalCampaignSave.generated.h"

UCLASS()
class COASTALFOUNDATION_API UCoastalCampaignSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame)
    FCoastalCampaignSnapshot Snapshot;
};
