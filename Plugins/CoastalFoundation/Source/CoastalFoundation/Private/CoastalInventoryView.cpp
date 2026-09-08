#include "CoastalInventoryView.h"
#include "Core/InventoryViewRules.h"
bool FCoastalContainerView::IsValidFor(FName Expected) const
{
    if (Expected.IsNone() || ContainerId.IsNone() || Items.Num() > 4096) return false;
    std::vector<coastal::InventoryViewRow> Rows;
    for (const auto& Item : Items)
    {
        if (!Item.InstanceId.IsValid() || Item.ItemId.IsNone() || Item.DisplayName.IsEmpty()
            || Item.DisplayName.ToString().Len() > 160 || Item.Description.ToString().Len() > 4096) return false;
        Rows.push_back({TCHAR_TO_UTF8(*Item.InstanceId.ToString()), TCHAR_TO_UTF8(*Item.ItemId.ToString()),
            Item.Quantity, Item.Position.X, Item.Position.Y, Item.Size.X, Item.Size.Y});
    }
    return coastal::ValidInventoryView(TCHAR_TO_UTF8(*Expected.ToString()), TCHAR_TO_UTF8(*ContainerId.ToString()),
        Grid.X, Grid.Y, Revision, Rows);
}
