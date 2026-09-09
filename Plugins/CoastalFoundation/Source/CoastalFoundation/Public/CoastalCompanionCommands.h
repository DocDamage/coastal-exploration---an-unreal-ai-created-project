#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CoastalCompanionCommands.generated.h"

// Optional expansion command surface; Foundation does not depend on the dog plugin.
UINTERFACE(MinimalAPI)
class UCoastalCompanionCommands : public UInterface
{
    GENERATED_BODY()
};

class COASTALFOUNDATION_API ICoastalCompanionCommands
{
    GENERATED_BODY()
public:
    virtual bool CanCommandCompanion() const = 0;
    virtual bool IsCompanionFollowing() const = 0;
    virtual bool SetCompanionFollowing(bool bFollowing) = 0;
};
