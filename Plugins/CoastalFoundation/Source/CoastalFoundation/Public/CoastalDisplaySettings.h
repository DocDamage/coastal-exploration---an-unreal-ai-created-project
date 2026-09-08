#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/DisplaySettingsRules.h"
#include "Scalability.h"
#include "CoastalDisplaySettings.generated.h"
class APlayerController;
class UGameUserSettings;
class UGameViewportClient;
class FViewport;

// Internal UI-owned adapter, not another global preferences/input owner.
UCLASS()
class COASTALFOUNDATION_API UCoastalDisplaySettings : public UObject
{
    GENERATED_BODY()
public:
    bool Initialize(APlayerController* Player, bool bOptedIn);
    bool RefreshCatalogue();
    bool Ready() const;
    bool Begin(coastal::DisplayMode Candidate, coastal::PanelTicket Ticket);
    void Tick(coastal::PanelTicket Top, bool bPresented, bool bSessionHealthy);
    bool CanKeep(coastal::PanelTicket Top, bool bPresented) const;
    bool Keep(coastal::PanelTicket Top, bool bPresented, bool bRequestSave);
    void Cancel();
    void Release();
    FString Status() const;
    static FString Describe(coastal::DisplayMode Mode);
    coastal::DisplayMode Current() const;
    const std::vector<coastal::DisplayMode>& Modes() const { return Catalogue; }
    const coastal::DisplayTrial& Trial() const { return Model; }
    FString LastDetail;
    void ReadGraphics();
    bool GraphicsReady() const;
    bool GraphicsChanged() const;
    void CycleGraphicsPreset();
    void ToggleGraphicsVSync();
    void CycleGraphicsCap();
    bool ApplyGraphics(bool bSave);
    FString DescribeGraphics() const;
protected:
    virtual void BeginDestroy() override;
private:
    TWeakObjectPtr<APlayerController> Controller;
    TWeakObjectPtr<UGameViewportClient> ViewportClient;
    TWeakObjectPtr<UGameUserSettings> Settings;
    FViewport* BoundViewport = nullptr; // Compare only; dereference after the live client has matched.
    coastal::DisplayTrial Model;
    std::vector<coastal::DisplayMode> Catalogue;
    coastal::DisplayMode CapturedSettings;
    bool bInitialized = false, bStopped = false, bFaultAfterRestore = false, bWriting = false;
    bool Binding() const;
    Scalability::FQualityLevels GraphicsBefore, GraphicsQualityDraft;
    int32 GraphicsPreset = -1;
    float GraphicsCap = 0, GraphicsBeforeCap = 0;
    bool bGraphicsVSync = false, bGraphicsBeforeVSync = false, bGraphicsDraft = false;
    bool SettingsUnchanged() const;
    coastal::DisplayObservation Observe(coastal::PanelTicket Top, bool bPresented) const;
    void Dispatch(coastal::DisplayCommand Command);
};
