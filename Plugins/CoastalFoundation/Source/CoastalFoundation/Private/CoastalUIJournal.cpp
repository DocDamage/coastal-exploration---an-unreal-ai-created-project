#include "CoastalUISessionComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalStoryLibrary.h"

void UCoastalUISessionComponent::PresentJournal(FText& Title, FText& Body, TArray<FCoastalUIChoice>& Choices)
{
    Title = FText::FromString(TEXT("JOURNAL"));
    FString Text = TEXT("Current objective\n") + Objective().ToString();
    const auto Entries = Saves->GetJournal();
    // Selection is local reading state, never a new discovery or acknowledgement.
    if (!Entries.Contains(JournalSelection)) JournalSelection = Entries.IsEmpty() ? NAME_None : Entries[0];
    if (Entries.IsEmpty()) Text += TEXT("\n\nNo discoveries recorded yet. Notes and the radio message will appear here as you find them.");
    else
    {
        Text += TEXT("\n\n") + UCoastalStoryLibrary::JournalTitle(JournalSelection).ToString()
            + TEXT("\n\n") + UCoastalStoryLibrary::JournalText(JournalSelection).ToString();
        for (FName Entry : Entries)
        {
            const FString Id = TEXT("read_entry.") + Entry.ToString();
            const FString Label = (Entry == JournalSelection ? FString(TEXT("Reading: ")) : FString())
                + UCoastalStoryLibrary::JournalTitle(Entry).ToString();
            Choices.Add({FName(*Id), FText::FromString(Label), true});
        }
    }
    Choices.Add({TEXT("back"), FText::FromString(TEXT("Close journal")), true});
    Body = FText::FromString(Text);
}
void UCoastalUISessionComponent::SelectJournalEntry(FName Command)
{
    const FName Requested(*Command.ToString().RightChop(11));
    if (!IsValid(Saves) || !Saves->GetJournal().Contains(Requested)) return;
    JournalSelection = Requested;
    // Keep the selected entry's text in view instead of scrolling to its button.
    bReadTextPending = true;
    bRefreshPending = true;
}
