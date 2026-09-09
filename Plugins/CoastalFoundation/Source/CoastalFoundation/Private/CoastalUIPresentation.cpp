#include "CoastalUISessionComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalStoryLibrary.h"
#include "CoastalLocalOptions.h"
#include "CoastalCampingActionComponent.h"
#include "CoastalCompanionUI.h"

void UCoastalUISessionComponent::Present(coastal::PanelKind Kind, FText& Title, FText& Body,
    TArray<FCoastalUIChoice>& Choices)
{
    using K = coastal::PanelKind;
    const auto Add = [&Choices](const TCHAR* Id, const TCHAR* Label, bool Enabled = true)
    { Choices.Add({FName(Id), FText::FromString(Label), Enabled}); };
    const bool Active = IsValid(Saves) && Saves->HasActiveCampaign();
    const bool Ready = IntegrationReady() && !Saves->IsBusy();
    FString Text;
    switch (Kind)
    {
    case K::Display: case K::ConfirmDisplay:
        PresentDisplay(Kind, Title, Body, Choices); return;
    case K::Settings:
        PresentOptions(Title, Body, Choices); return;
    case K::Session:
        Title = FText::FromString(TEXT("FIRST SIGNAL")); RefreshSaveSets();
        Text = TEXT("Return to the coast, or begin a new campaign. Use a fresh name to start without replacing an existing save.\n") + CatalogueNotice;
        if (!Ready) Text += TEXT("\nNOT CONFIGURED: the real AGIS adapter and interaction bridge must be connected.");
        Add(TEXT("start"), TEXT("Start new campaign with this name"), Ready);
        Add(TEXT("continue"), TEXT("Continue selected campaign"), Ready);
        Add(TEXT("next_save"), TEXT("Select next saved campaign"), !SaveSets.IsEmpty());
        Add(TEXT("fresh_name"), TEXT("Create a new campaign name"));
        if (Active) Add(TEXT("back"), TEXT("Back to current campaign"));
        Add(TEXT("options"), TEXT("Player options"), IsValid(Options) && !Saves->IsBusy());
        Add(TEXT("display_options"), TEXT("Display settings"));
        Add(TEXT("quit"), TEXT("Exit...")); break;
    case K::Pause:
        Title = FText::Format(NSLOCTEXT("CoastalUI", "PausedCampaign", "PAUSED | {0}"), CampaignTitle()); Text = Objective().ToString();
        Add(TEXT("back"), TEXT("Resume")); Add(TEXT("save"), TEXT("Save campaign now"), Ready && Active);
        Add(TEXT("journal"), TEXT("Journal")); Add(TEXT("inventory"), TEXT("Inventory"));
        if (auto* Companion = FindCoastalCompanionCommands(GetWorld()))
        {
            Add(TEXT("companion_toggle"), Companion->IsCompanionFollowing()
                ? TEXT("Dog: wait here") : TEXT("Dog: follow me"),
                Active && Ready && Companion->CanCommandCompanion());
        }
        if (IsValid(CampingActions) && CampingActions->IsNearShelter())
        {
            Text += TEXT("\n\nTake a short rest here. Move, jump, or open a menu to stop.");
            Add(TEXT("shelter_rest"), TEXT("Rest in shelter"),
                CampingActions->CanOfferAction(ECoastalCampingAction::RestInShelter));
        }
        if (IsValid(CampingActions) && CampingActions->IsNearCampsite())
        {
            Text += TEXT("\n\nCampsite actions resume the game and end when you move, jump, or open a menu.");
            const bool bCanWarm = CampingActions->CanOfferAction(ECoastalCampingAction::WarmHands);
            const bool bCanRest = CampingActions->CanOfferAction(ECoastalCampingAction::RestByFire);
            if (!bCanWarm || !bCanRest)
                Text += TEXT("\nA disabled action needs an unobstructed dry marker, grounded player, and no other montage.");
            Add(TEXT("camping_warm_hands"), TEXT("Warm hands by the fire"), bCanWarm);
            Add(TEXT("camping_rest"), TEXT("Rest by the fire"), bCanRest);
        }
        Add(TEXT("options"), TEXT("Player options"), IsValid(Options) && !Saves->IsBusy());
        Add(TEXT("display_options"), TEXT("Display settings"));
        Add(TEXT("sessions"), TEXT("Campaigns...")); Add(TEXT("quit"), TEXT("Exit...")); break;
    case K::Journal:
        PresentJournal(Title, Body, Choices); return;
    case K::Transcript:
        Title = FText::FromString(TEXT("RADIO TRANSMISSION")); Text = TranscriptText.ToString();
        Text += TEXT("\n\nContinue records this message and the North Reach lead. Back cancels without completing it.");
        Add(TEXT("acknowledge"), TEXT("Continue: record the message")); Add(TEXT("back"), TEXT("Cancel")); break;
    case K::Inventory: case K::Storage:
        PresentInventory(Kind == K::Storage, Title, Body, Choices); return;
    case K::ItemDetails:
        Title = FText::FromString(TEXT("ITEM DETAILS")); Text = ItemDetail.ToString(); Add(TEXT("back"), TEXT("Back to selection")); break;
    case K::ConfirmSession:
        Title = FText::FromString(TEXT("REPLACE THE LIVE SESSION?"));
        Text = TEXT("This will replace the current in-memory campaign. Unverified changes may be lost. Existing disk saves are not deleted.\nSelected set: ") + PendingSaveSet.ToString();
        Add(TEXT("confirm_session"), TEXT("Replace live session"), Ready); Add(TEXT("back"), TEXT("Cancel")); break;
    case K::ConfirmExit:
        Title = FText::FromString(TEXT("EXIT FIRST SIGNAL"));
        Text = TEXT("Save and Exit only exits after a verified save. Exit Without Saving can lose progress since the last verified generation.");
        Add(TEXT("save_exit"), TEXT("Save and Exit"), Ready && Active);
        Add(TEXT("exit_without_save"), TEXT("Exit Without Saving")); Add(TEXT("back"), TEXT("Cancel")); break;
    case K::Recovery:
        Title = FText::FromString(TEXT("RECOVERY REQUIRED"));
        Text = TEXT("The live campaign cannot safely continue. Saving and gameplay remain blocked. Close the game, relaunch, and load a validated save. No new campaign has been substituted.");
        Add(TEXT("exit_without_save"), TEXT("Exit WITHOUT writing the damaged live state")); break;
    }
    Body = FText::FromString(Text);
}
