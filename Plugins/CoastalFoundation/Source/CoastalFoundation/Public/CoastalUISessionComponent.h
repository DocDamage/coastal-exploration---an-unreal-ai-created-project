#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoastalInteractionBridge.h"
#include "CoastalInventoryView.h"
#include "CoastalPlayerRecoveryComponent.h"
#include "Core/UIFlowRules.h"
#include "Core/PlayerOptionsRules.h"
#include "Core/AudioPlaybackRules.h"
#include "Core/DisplaySettingsRules.h"
#include "CoastalUISessionComponent.generated.h"
class UCoastalDisplaySettings;
class UCoastalLocalOptions;
class UCoastalLookInputComponent;
class UCoastalSprintComponent;
class UCoastalAudioOptionsComponent;
class UCoastalAudioPlaybackComponent;
class UCoastalCampingActionComponent;
class UCoastalPanelWidget;
class UCoastalHUDWidget;
class UCoastalSaveCoordinator;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputComponent;
class APlayerController;
class IInputProcessor;
class UTexture2D;
class UStaticMesh;
class UTextureRenderTarget2D;
class UCoastalItemPreviewComponent;
DECLARE_MULTICAST_DELEGATE_OneParam(FCoastalMenuFeedback, FName);

struct FCoastalUIChoice
{
    FName Command;
    FText Label;
    bool bEnabled = true;
    TObjectPtr<UTexture2D> Icon = nullptr;
};

// Add to the LOCAL PlayerController. One explicit owner of M1's UI/input mode.
UCLASS(Blueprintable, ClassGroup=(Coastal), meta=(BlueprintSpawnableComponent))
class COASTALFOUNDATION_API UCoastalUISessionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCoastalUISessionComponent();
    // Enable only after removing competing host display/settings handlers. Captured at initialization.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|Display") bool bEnableDisplaySettings = false;
    // May show a NotConfigured diagnostic panel before the real adapter is ready.
    // True means the UI attached, not that inventory integration passed.
    UFUNCTION(BlueprintCallable, Category="Coastal|UI")
    bool InitializeUI(UCoastalSaveCoordinator* Coordinator, UCoastalInteractionBridge* Interaction,
        FTransform NewCampaignCheckpoint);
    UFUNCTION(BlueprintCallable, Category="Coastal|UI") void OpenPause();
    UFUNCTION(BlueprintCallable, Category="Coastal|UI") void OpenInventory();
    UFUNCTION(BlueprintCallable, Category="Coastal|UI") void OpenJournal();
    UFUNCTION(BlueprintPure, Category="Coastal|UI") bool HasModal() const { return Flow.Depth() > 0; }
    UFUNCTION(BlueprintPure, Category="Coastal|UI") bool IsInitialized() const { return bInitialized && !bClosing; }
    bool IsPresentingJournalEntry(FName Entry) const;
    virtual void TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    UFUNCTION(BlueprintPure, Category="Coastal|UI") FText Objective() const;
    UFUNCTION(BlueprintPure, Category="Coastal|UI") FText CampaignTitle() const;
    FText Status() const;
    FText PanelStatus(coastal::PanelKind Kind) const;
    FText DisplayConfirmationTitle() const;
    void NoteInputDevice(bool bGamepad) { bUsingGamepad = bGamepad; }
    FText MenuHelp() const;
    FText HUDControls() const;
    // Native read-only presentation projection. Does not expose transcript acknowledgement.
    coastal::AudioPlaybackContext GetAudioPlaybackContext() const;
    int32 FontSize(int32 BaseSize) const;
    FName SelectedSaveSet = TEXT("m1_test_01");
    void SetSaveSetText(const FText& Text);
    void Present(coastal::PanelKind Kind, FText& Title, FText& Body, TArray<FCoastalUIChoice>& Choices);
    void Command(UCoastalPanelWidget* Sender, FName Id);
    FCoastalMenuFeedback OnMenuFeedback;
    void NotifyMenuFocus(UCoastalPanelWidget* Sender);
    void RefreshCharacterCreator() { if (Flow.Contains(coastal::PanelKind::CharacterCreator) || Flow.Contains(coastal::PanelKind::Pause)) bRefreshPending = true; }
    // Optional authored icon lookup for read-only inventory presentation.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|UI")
    TMap<FName, TObjectPtr<UTexture2D>> ItemIcons;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Coastal|UI")
    TMap<FName, TObjectPtr<UStaticMesh>> ItemPreviewMeshes;
    UTextureRenderTarget2D* ItemPreviewTexture() const;
    bool RotateItemPreview(UCoastalPanelWidget* Sender, float YawDegrees, float PitchDegrees);
private:
    UPROPERTY() TObjectPtr<UCoastalItemPreviewComponent> ItemPreview;
    FGuid PreviewInstance;
    FName PreviewContainer, PreviewItem;
    int64 PreviewRevision = -1;
    void InspectSelectedItem();
    bool ValidateItemPreview();
    void CloseItemPreview();
    bool bUsingGamepad = false;
    TSharedPtr<IInputProcessor> InputHints;
    void InstallInputHints();
    void RemoveInputHints();
    UPROPERTY() TObjectPtr<UCoastalDisplaySettings> DisplaySettings;
    coastal::DisplayMode DisplayDraft;
    uint64 DisplayPaintRevision = 0;
    bool bDisplayKeepAvailable = false;
    void InitializeDisplay();
    void TickDisplay();
    void CancelDisplay();
    void PresentDisplay(coastal::PanelKind Kind, FText& Title, FText& Body, TArray<FCoastalUIChoice>& Choices);
    void DisplayCommand(FName Id);
    void SelectDisplayMode(int32 ModeStep, int32 ResolutionStep);
    UPROPERTY() TObjectPtr<UCoastalLocalOptions> Options;
    UPROPERTY() TObjectPtr<UCoastalLookInputComponent> LookInput;
    UPROPERTY() TObjectPtr<UCoastalSprintComponent> SprintInput;
    UPROPERTY() TObjectPtr<UCoastalAudioOptionsComponent> AudioOptions;
    UPROPERTY() TObjectPtr<UCoastalAudioPlaybackComponent> AudioPlayback;
    UPROPERTY() TObjectPtr<UCoastalCampingActionComponent> CampingActions;
    FString AudioPlaybackNotice;
    void InitializeAudioPlayback();
    void RefreshAudioPlayback();
    void SuspendAudioPlayback();
    void InitializeCampingActions();
    void CancelCampingAction();
    coastal::PlayerOptions OptionsDraft;
    int32 SelectedOption = 0;
    FString OptionsNotice;
    bool InitializeOptions();
    void PresentOptions(FText& Title, FText& Body, TArray<FCoastalUIChoice>& Choices);
    void OptionsCommand(FName Id);
    bool OptionAvailable(coastal::OptionField Field) const;
    UPROPERTY() TObjectPtr<UCoastalSaveCoordinator> Saves;
    UPROPERTY() TObjectPtr<UCoastalInteractionBridge> Bridge;
    UPROPERTY() TObjectPtr<APlayerController> Controller;
    UPROPERTY() TObjectPtr<UCoastalHUDWidget> HUD;
    UPROPERTY() TObjectPtr<UCoastalPlayerRecoveryComponent> Recovery;
    UPROPERTY() TArray<TObjectPtr<UCoastalPanelWidget>> Panels;
    UPROPERTY() TObjectPtr<UInputMappingContext> MenuContext;
    UPROPERTY() TObjectPtr<UEnhancedInputComponent> MenuInput;
    UPROPERTY() TArray<TObjectPtr<UInputAction>> MenuActions;
    UPROPERTY() FCoastalContainerView BackpackView;
    UPROPERTY() FCoastalContainerView StorageView;
    coastal::UIFlow Flow;
    FTransform InitialCheckpoint;
    FName StorageId, PendingStorageId, PendingSaveSet, PendingSessionCommand, PendingJournalEntry;
    FGuid TranscriptToken, PendingTranscriptToken;
    FText TranscriptText, PendingTranscriptText, ItemDetail;
    FCoastalTransferRequest PendingTransfer;
    TArray<FName> SaveSets;
    uint64 Epoch = 0;
    int32 SelectedRow = -1;
    bool bStorageSide = false, bViewsValid = false, bInitialized = false;
    bool bInputOwned = false, bPauseOwned = false, bPreviousCursor = false;
    bool bSessionReset = false, bRecoveryPending = false, bRefreshPending = false;
    bool bTransferPending = false, bClosing = false;
    FString SaveNotice, ActionNotice, UIError, CatalogueNotice, SafetyNotice;
    UFUNCTION() void ReturnNotified(ECoastalReturnNotice Result, FString Detail);
    UFUNCTION() void SaveNotified(ECoastalSaveResult Result, FString Detail);
    UFUNCTION() void ActionNotified(ECoastalActionResult Result);
    UFUNCTION() void StorageRequested(FName Id);
    UFUNCTION() void TranscriptRequested(FText Text, FGuid Token);
    UFUNCTION() void JournalRequested(FName EntryId);
    void ShutdownUI();
    bool InstallMenuInput();
    void RemoveMenuInput();
    void ApplyInputOwnership();
    void ReleaseInputOwnership();
    bool Push(coastal::PanelKind Kind, bool bFeedback = true);
    void CloseTop(bool bFeedback = false);
    void ClearPanels();
    void RefreshTop();
    void EnterRecovery();
    bool IntegrationReady() const;
    void RefreshSaveSets();
    bool ReadViews();
    const FCoastalContainerView& SelectedView() const;
    void MoveSelection(int32 Direction);
    void SelectInventoryItem(FName Command);
    void PresentInventory(bool bStorage, FText& Title, FText& Body, TArray<FCoastalUIChoice>& Choices);
    void PresentJournal(FText& Title, FText& Body, TArray<FCoastalUIChoice>& Choices);
    void SelectJournalEntry(FName Command);
    FName JournalSelection;
    bool bReadTextPending = false;
    void TransferSelected(bool bRetry);
    void CompleteSessionCommand();
    void QuitWithoutSave();
};
