#include "CoastalHostGameEngine.h"
#include "Slate/SceneViewport.h"
#include "UnrealEngine.h"

void UCoastalHostGameEngine::Init(IEngineLoop* InEngineLoop)
{
    Super::Init(InEngineLoop);
    // UE 5.7 UGameEngine::Init binds one resize handler to this object. Its
    // OnViewportResized also stages and confirms preferences on every resize.
    // Replace only our inherited binding; other owners' delegates remain intact.
    const int32 Removed=FViewport::ViewportResizedEvent.RemoveAll(this);
    checkf(Removed==1,TEXT("Re-audit the engine resize bindings before using this host."));
    FViewport::ViewportResizedEvent.AddUObject(this,&UCoastalHostGameEngine::ResizeRuntimeViewport);
}
void UCoastalHostGameEngine::ResizeRuntimeViewport(FViewport* Viewport,uint32)
{
    const auto Window=GameViewportWindow.Pin();
    if(!Viewport || Viewport!=SceneViewport.Get() || !Window.IsValid() || Window->GetWindowMode()!=EWindowMode::Windowed)return;
    const auto Size=Viewport->GetSizeXY();
    if(Size.X<=0 || Size.Y<=0)return;
    // Preserve native runtime bookkeeping, including a player dragging the
    // window edge. Saved/confirmed fields change only through explicit Keep.
    GSystemResolution.ResX=Size.X;GSystemResolution.ResY=Size.Y;
    FSystemResolution::RequestResolutionChange(Size.X,Size.Y,EWindowMode::Windowed);
}
