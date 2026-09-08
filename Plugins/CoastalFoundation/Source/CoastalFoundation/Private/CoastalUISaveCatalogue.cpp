#include "CoastalUISessionComponent.h"
#include "CoastalSaveCoordinator.h"
#include "Core/InventoryViewRules.h"
#include "Core/TransactionRules.h"
#include "PlatformFeatures.h"
#include "SaveGameSystem.h"

void UCoastalUISessionComponent::RefreshSaveSets()
{
    SaveSets.Reset(); TArray<FString> Names;
    auto* System = IPlatformFeaturesModule::Get().GetSaveGameSystem();
    if (!System || !System->GetSaveGameNames(Names, 0))
    { CatalogueNotice = TEXT("Save-name enumeration is unavailable. Enter an exact save-set name; this is not a 'no saves' result."); return; }
    for (const FString& Name : Names)
    {
        std::string Set;
        if (coastal::SaveSetFromSlot(TCHAR_TO_UTF8(*Name), Set)) SaveSets.AddUnique(FName(UTF8_TO_TCHAR(Set.c_str())));
    }
    SaveSets.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
    CatalogueNotice = FString::Printf(TEXT("%d save-set names found. Names are not proof of valid saves; Continue validates both slots."), SaveSets.Num());
}
void UCoastalUISessionComponent::SetSaveSetText(const FText& Text)
{
    const FString Value = Text.ToString();
    if (Value.Len() > 64 || Value.Equals(TEXT("none"), ESearchCase::IgnoreCase)
        || !coastal::ValidLogicalId(TCHAR_TO_UTF8(*Value)))
    { SelectedSaveSet = NAME_None; UIError = TEXT("Save-set name must be 1-64 lowercase letters/digits/dots/underscores; no paths or repeated dots."); return; }
    SelectedSaveSet = FName(*Value); UIError.Empty();
}
void UCoastalUISessionComponent::CompleteSessionCommand()
{
    if (!IntegrationReady() || Saves->IsBusy() || PendingSaveSet.IsNone())
    { UIError = TEXT("Campaign action blocked: configure the real adapter/bridge and provide a valid save-set name."); return; }
    if (PendingSessionCommand == TEXT("start")) Saves->StartNewCampaign(PendingSaveSet, InitialCheckpoint);
    else if (PendingSessionCommand == TEXT("continue")) Saves->LoadCampaign(PendingSaveSet);
    // Failed Continue is deliberately NOT followed by StartNewCampaign.
    PendingSessionCommand = NAME_None; PendingSaveSet = NAME_None;
}
