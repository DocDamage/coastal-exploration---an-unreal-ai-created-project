#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/OptionsSession.h"
#include "CoastalLocalOptions.generated.h"

// Local machine/user-0 preference bytes, never part of a campaign or the AGIS payload.
UCLASS()
class COASTALFOUNDATION_API UCoastalLocalOptions : public UObject
{
    GENERATED_BODY()
public:
    bool Initialize();
    bool IsInitialized() const { return State.IsInitialized(); }
    bool ApplySession(const coastal::PlayerOptions& Values);
    bool SaveAndApply(const coastal::PlayerOptions& Values);
    const coastal::PlayerOptions& Get() const { return State.Get(); }
    bool CanWrite() const { return State.CanWrite(); }
    FString Notice() const;
private:
    coastal::OptionsSession State;
    coastal::OptionsImage Read(int32 Index) const;
    static FString SlotName(int32 Index);
};
