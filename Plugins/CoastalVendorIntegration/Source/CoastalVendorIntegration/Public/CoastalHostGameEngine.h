#pragma once
#include "Engine/GameEngine.h"
#include "CoastalHostGameEngine.generated.h"

// The M1 host uses Coastal's explicit display confirmation as its sole settings writer.
UCLASS()
class COASTALVENDORINTEGRATION_API UCoastalHostGameEngine : public UGameEngine
{
    GENERATED_BODY()
public:
    virtual void Init(IEngineLoop* InEngineLoop) override;
private:
    void ResizeRuntimeViewport(FViewport* Viewport,uint32 Unused);
};
