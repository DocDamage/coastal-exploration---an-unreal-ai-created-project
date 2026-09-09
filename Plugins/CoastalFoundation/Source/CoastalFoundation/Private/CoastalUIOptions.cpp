#include "CoastalUISessionComponent.h"
#include "CoastalLocalOptions.h"
#include "CoastalLookInputComponent.h"
#include "CoastalSprintComponent.h"
#include "CoastalAudioOptionsComponent.h"
#include "CoastalSaveCoordinator.h"
#include "GameFramework/PlayerController.h"

bool UCoastalUISessionComponent::InitializeOptions()
{
    Options = NewObject<UCoastalLocalOptions>(this);
    if (!Options || !Options->Initialize()) return false;
    TArray<UCoastalLookInputComponent*> Consumers;
    Bridge->GetOwner()->GetComponents<UCoastalLookInputComponent>(Consumers);
    if (Consumers.Num() == 1 && Consumers[0]->InitializeOptions(Options, Bridge)) LookInput = Consumers[0];
    else if (!Consumers.IsEmpty())
        OptionsNotice = TEXT("Camera options unavailable: use one registered look component on the fixed character with one active perspective camera. Text options remain available.");
    TArray<UCoastalSprintComponent*> SprintOwners;
    Bridge->GetOwner()->GetComponents<UCoastalSprintComponent>(SprintOwners);
    if (SprintOwners.Num() == 1 && SprintOwners[0]->InitializeOptions(Options, Bridge)) SprintInput = SprintOwners[0];
    else if (!SprintOwners.IsEmpty())
        OptionsNotice += TEXT("\nSprint options unavailable: use one opted-in character sprint component and route actual host Sprint state.");
    TArray<UCoastalAudioOptionsComponent*> AudioOwners;
    Controller->GetComponents<UCoastalAudioOptionsComponent>(AudioOwners);
    if (AudioOwners.Num() == 1 && AudioOwners[0]->InitializeOptions(Options)) AudioOptions = AudioOwners[0];
    else if (!AudioOwners.IsEmpty())
        OptionsNotice += TEXT("\nAudio options unavailable: use one opted-in controller audio owner with three actual dedicated sound classes.");
    return true;
}
int32 UCoastalUISessionComponent::FontSize(int32 BaseSize) const
{
    const bool Preview = Flow.Top() && Flow.Top()->ticket.kind == coastal::PanelKind::Settings;
    const int Percent = Preview ? OptionsDraft.textPercent : (IsValid(Options) ? Options->Get().textPercent : 100);
    return coastal::OptionFontSize(BaseSize, Percent);
}
bool UCoastalUISessionComponent::OptionAvailable(coastal::OptionField Field) const
{
    if (!IsValid(Options)) return false;
    if (coastal::IsAudioOption(Field)) return IsValid(AudioOptions) && AudioOptions->IsAudioReady();
    if (Field == coastal::OptionField::Text) return true;
    if (Field == coastal::OptionField::FieldOfView) return IsValid(LookInput) && LookInput->IsCameraReady();
    if (Field == coastal::OptionField::SprintMode) return IsValid(SprintInput) && SprintInput->IsSprintReady();
    if (Field == coastal::OptionField::Mouse || Field == coastal::OptionField::Stick || Field == coastal::OptionField::InvertY)
        return IsValid(LookInput) && LookInput->IsLookReady();
    return false;
}
void UCoastalUISessionComponent::PresentOptions(FText& Title, FText& Body, TArray<FCoastalUIChoice>& Choices)
{
    using F = coastal::OptionField;
    Title = FText::FromString(TEXT("PLAYER OPTIONS"));
    const auto Add = [&Choices](const TCHAR* Id, const TCHAR* Label, bool Enabled = true)
    { Choices.Add({FName(Id), FText::FromString(Label), Enabled}); };
    FString Text = TEXT("Select a setting, then decrease/increase it. Text size previews here; other changes apply only when chosen below. Back discards unapplied edits.\n\n");
    const TArray<FString> Rows = {
        FString::Printf(TEXT("Mouse look: %d%%"), OptionsDraft.mousePercent),
        FString::Printf(TEXT("Gamepad look: %d%%"), OptionsDraft.stickPercent),
        FString::Printf(TEXT("Camera field of view: %d degrees"), OptionsDraft.fieldOfView),
        FString::Printf(TEXT("Text size: %d%%"), OptionsDraft.textPercent),
        FString::Printf(TEXT("Invert look Y: %s"), OptionsDraft.invertY ? TEXT("On") : TEXT("Off")),
        FString::Printf(TEXT("Sprint input: %s"), OptionsDraft.sprintToggle ? TEXT("Toggle") : TEXT("Hold")),
        FString::Printf(TEXT("Master volume (routed game sounds): %d%%"), OptionsDraft.masterPercent),
        FString::Printf(TEXT("Ambience volume: %d%%"), OptionsDraft.ambiencePercent),
        FString::Printf(TEXT("Effects volume: %d%%"), OptionsDraft.effectsPercent),
        FString::Printf(TEXT("Radio volume: %d%%"), OptionsDraft.radioPercent)};
    for (int32 I = 0; I < Rows.Num(); ++I)
        Text += FString(I == SelectedOption ? TEXT("> ") : TEXT("  ")) + Rows[I]
            + (OptionAvailable(static_cast<F>(I)) ? TEXT("\n") : TEXT(" [host binding unavailable]\n"));
    if (!OptionAvailable(F::FieldOfView)) Text += TEXT("\nCamera FOV needs the optional Coastal look component and a compatible active camera.");
    if (!OptionAvailable(F::Mouse)) Text += TEXT("\nLook options need explicit host mouse/stick routing; this screen does not create input mappings.");
    if (!OptionAvailable(F::SprintMode)) Text += TEXT("\nSprint mode needs the optional character Sprint component with real host input routing.");
    else Text += TEXT("\nToggle selects sprint speed; it is not auto-run. Menus, loads, jumps and recovery cancel sprint. Release Sprint after returning, then press again.");
    if (IsValid(SprintInput) && !SprintInput->LastDetail.IsEmpty()) Text += TEXT("\n") + SprintInput->LastDetail;
    if (!OptionAvailable(F::Master)) Text += TEXT("\nVolume controls need actual dedicated sound-class routing; no audio is supplied or automatically connected.");
    else Text += TEXT("\nVolumes apply on Apply, not while editing. 0% requests mute. Master affects the three routed game classes, not system audio. Radio text and Continue remain available while muted.");
    if (IsValid(AudioOptions) && !AudioOptions->LastDetail.IsEmpty()) Text += TEXT("\n") + AudioOptions->LastDetail;
    Text += TEXT("\n\n") + (IsValid(Options) ? Options->Notice() : TEXT("Local options unavailable."));
    if (IsValid(Options) && OptionsDraft != Options->Get()) Text += TEXT("\nUnapplied changes are present.");
    const bool Idle = IsValid(Saves) && !Saves->IsBusy();
    const auto Field = static_cast<F>(SelectedOption);
    auto Less = OptionsDraft, More = OptionsDraft;
    const bool CanLess = coastal::AdjustOption(Less, Field, -1);
    const bool CanMore = coastal::AdjustOption(More, Field, 1);
    Add(TEXT("option_previous"), TEXT("Previous setting")); Add(TEXT("option_next"), TEXT("Next setting"));
    if (Field == F::InvertY) Add(TEXT("option_increase"), TEXT("Toggle invert Y"), OptionAvailable(Field) && Idle);
    else if (Field == F::SprintMode) Add(TEXT("option_increase"), TEXT("Switch Hold / Toggle sprint"), OptionAvailable(Field) && Idle);
    else
    {
        Add(TEXT("option_decrease"), TEXT("Decrease selected setting"), CanLess && OptionAvailable(Field) && Idle);
        Add(TEXT("option_increase"), TEXT("Increase selected setting"), CanMore && OptionAvailable(Field) && Idle);
    }
    Add(TEXT("option_defaults"), TEXT("Defaults for available settings (draft only)"), Idle);
    Add(TEXT("option_session"), TEXT("Apply for this session only"), Idle);
    Add(TEXT("option_save"), TEXT("Apply and save local options"), Idle && IsValid(Options) && Options->CanWrite());
    Add(TEXT("back"), TEXT("Back / discard unapplied edits"));
    Body = FText::FromString(Text);
}
void UCoastalUISessionComponent::OptionsCommand(FName Id)
{
    if (!IsValid(Options) || !IsValid(Saves) || Saves->IsBusy()) return;
    if (Id == TEXT("options"))
    { OptionsDraft = Options->Get(); SelectedOption = 0; Push(coastal::PanelKind::Settings); return; }
    if (Id == TEXT("option_previous") || Id == TEXT("option_next"))
    {
        SelectedOption = coastal::CycleSelection(SelectedOption, Id == TEXT("option_next") ? 1 : -1, coastal::OptionFieldCount);
        OnMenuFeedback.Broadcast(TEXT("select"));
    }
    else if (Id == TEXT("option_increase") || Id == TEXT("option_decrease"))
    {
        const auto Field = static_cast<coastal::OptionField>(SelectedOption);
        if (OptionAvailable(Field) && coastal::AdjustOption(OptionsDraft, Field, Id == TEXT("option_increase") ? 1 : -1))
            OnMenuFeedback.Broadcast(TEXT("select"));
    }
    else if (Id == TEXT("option_defaults"))
    {
        const coastal::PlayerOptions Defaults;
        OptionsDraft.textPercent = Defaults.textPercent;
        if (OptionAvailable(coastal::OptionField::Master)) coastal::CopyAudioOptions(OptionsDraft, Defaults);
        if (OptionAvailable(coastal::OptionField::SprintMode)) OptionsDraft.sprintToggle = Defaults.sprintToggle;
        if (OptionAvailable(coastal::OptionField::FieldOfView)) OptionsDraft.fieldOfView = Defaults.fieldOfView;
        if (OptionAvailable(coastal::OptionField::Mouse))
        { OptionsDraft.mousePercent = Defaults.mousePercent; OptionsDraft.stickPercent = Defaults.stickPercent; OptionsDraft.invertY = Defaults.invertY; }
    }
    else if (Id == TEXT("option_session") || Id == TEXT("option_save"))
    {
        // A lost consumer cannot apply previously editable camera/look fields unnoticed.
        const auto& Live = Options->Get();
        if ((!OptionAvailable(coastal::OptionField::FieldOfView) && OptionsDraft.fieldOfView != Live.fieldOfView)
            || (!OptionAvailable(coastal::OptionField::Mouse) && (OptionsDraft.mousePercent != Live.mousePercent
                || OptionsDraft.stickPercent != Live.stickPercent || OptionsDraft.invertY != Live.invertY))
            || (!OptionAvailable(coastal::OptionField::SprintMode) && OptionsDraft.sprintToggle != Live.sprintToggle)
            || (!OptionAvailable(coastal::OptionField::Master) && !coastal::SameAudioOptions(OptionsDraft, Live)))
        { OptionsNotice = TEXT("Camera/look/sprint/audio binding changed. Close and reopen options; no changes were applied."); bRefreshPending = true; return; }
        const bool Applied = Id == TEXT("option_save") ? Options->SaveAndApply(OptionsDraft) : Options->ApplySession(OptionsDraft);
        OptionsNotice = Options->Notice();
        if (Applied)
        {
            OptionsDraft = Options->Get();
            if (IsValid(LookInput) && LookInput->IsCameraReady()) LookInput->ApplyCameraOptions();
            if (IsValid(SprintInput)) SprintInput->ApplySprintOptions();
            if (IsValid(AudioOptions) && !AudioOptions->ApplyAudioOptions())
                OptionsNotice += TEXT("\nPreferences applied, but audio commands could not be submitted. The audio feature is unavailable; inspect its binding and relaunch.");
        }
        OnMenuFeedback.Broadcast(Applied ? TEXT("confirm") : TEXT("error"));
    }
    bRefreshPending = true;
}
