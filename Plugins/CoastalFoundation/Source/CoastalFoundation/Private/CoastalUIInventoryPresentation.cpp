#include "CoastalUISessionComponent.h"
#include "CoastalSaveCoordinator.h"

namespace
{
FString CapacityText(const FCoastalContainerView& View)
{
    int32 Occupied = 0;
    for (const auto& Item : View.Items) Occupied += Item.Size.X * Item.Size.Y;
    return FString::Printf(TEXT("%d stacks | %d / %d spaces occupied"), View.Items.Num(), Occupied, View.Grid.X * View.Grid.Y);
}
}
void UCoastalUISessionComponent::PresentInventory(bool Storage, FText& Title, FText& Body,
    TArray<FCoastalUIChoice>& Choices)
{
    const auto Add = [&Choices](const TCHAR* Id, const FString& Label, bool Enabled = true, UTexture2D* Icon = nullptr)
    { Choices.Add({FName(Id), FText::FromString(Label), Enabled, Icon}); };
    const bool Ready = IntegrationReady() && !Saves->IsBusy();
    Title = FText::FromString(Storage ? TEXT("CABIN STORAGE") : TEXT("BACKPACK"));
    FString Text = Storage ? TEXT("Select an item, then choose where to move it.\n")
        : TEXT("Equipment and discoveries carried with you.\n");
    const bool Selected = bViewsValid && SelectedView().Items.IsValidIndex(SelectedRow) && !bTransferPending;
    if (!bViewsValid) Text += TEXT("\nInventory is unavailable. Refresh to try again. Transfers are disabled.");
    else
    {
        Text += TEXT("\nBackpack: ") + CapacityText(BackpackView);
        if (Storage) Text += TEXT("\nStorage: ") + CapacityText(StorageView);
        Text += bStorageSide ? TEXT("\n\nBrowsing storage") : TEXT("\n\nBrowsing backpack");
        const auto& View = SelectedView();
        if (View.Items.IsEmpty()) Text += TEXT("\nThere are no items here.");
        if (View.Items.IsValidIndex(SelectedRow))
        {
            const auto& Item = View.Items[SelectedRow];
            Text += FString::Printf(TEXT("\n\nSelected: %s\nQuantity: %d | Size: %d x %d spaces\n%s"),
                *Item.DisplayName.ToString(), Item.Quantity, Item.Size.X, Item.Size.Y, *Item.Description.ToString());
        }
        Text += TEXT("\n\nFree spaces may be separated; an item still needs room for its shape.");
    }
    Add(TEXT("inspect"), TEXT("Read item details"), Selected);
    if (Storage && !bTransferPending)
        Add(TEXT("transfer"), bStorageSide ? TEXT("Move one to backpack") : TEXT("Store one at the cabin"), Selected && Ready);
    if (Storage) Add(TEXT("switch_side"), bStorageSide ? TEXT("Browse backpack") : TEXT("Browse storage"), bViewsValid && !bTransferPending);
    if (bTransferPending)
    {
        Text += TEXT("\n\nThe last transfer is unresolved. Retry checks that same transfer; stop retrying does not undo an item already moved.");
        Add(TEXT("retry_transfer"), TEXT("Retry unresolved transfer"), Ready);
        Add(TEXT("cancel_transfer"), TEXT("Stop retrying and refresh items"));
    }
    Add(TEXT("previous_item"), TEXT("Previous item"), Selected);
    Add(TEXT("next_item"), TEXT("Next item"), Selected);
    // Keep a bounded set of directly selectable rows, even for a larger provider.
    // Previous/Next move through the complete validated view and update the page.
    if (bViewsValid && !SelectedView().Items.IsEmpty())
    {
        const auto& Items = SelectedView().Items;
        const int32 First = FMath::Max(0, SelectedRow) / 6 * 6;
        const int32 Last = FMath::Min(First + 6, Items.Num());
        Text += FString::Printf(TEXT("\n\nItem list: %d-%d of %d"), First + 1, Last, Items.Num());
        for (int32 I = First; I < Last; ++I)
        {
            const auto& Item = Items[I];
            const FString Id = TEXT("select_item.") + Item.InstanceId.ToString(EGuidFormats::Digits);
            const TObjectPtr<UTexture2D>* Icon = ItemIcons.Find(Item.ItemId);
            Add(*Id, FString::Printf(TEXT("%s%s  x%d"), I == SelectedRow ? TEXT("Selected: ") : TEXT(""),
                *Item.DisplayName.ToString(), Item.Quantity), !bTransferPending, Icon ? Icon->Get() : nullptr);
        }
    }
    Add(TEXT("refresh_inventory"), TEXT("Refresh items"), Ready);
    Add(TEXT("back"), TEXT("Close"));
    Body = FText::FromString(Text);
}
void UCoastalUISessionComponent::SelectInventoryItem(FName Command)
{
    if (bTransferPending) return;
    FGuid Requested;
    if (!FGuid::ParseExact(Command.ToString().RightChop(12), EGuidFormats::Digits, Requested)) return;
    // Resolve the identity again after a fresh read; a row number is not an item.
    if (!ReadViews()) { bRefreshPending = true; return; }
    const int32 Found = SelectedView().Items.IndexOfByPredicate([&Requested](const auto& Item) { return Item.InstanceId == Requested; });
    if (Found == INDEX_NONE) UIError = TEXT("That item is no longer here. Review the refreshed inventory.");
    else
    {
        if (SelectedRow != Found) OnMenuFeedback.Broadcast(TEXT("select"));
        SelectedRow = Found; bReadTextPending = true;
    }
    bRefreshPending = true;
}
