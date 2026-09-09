#include "CoastalUISessionComponent.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalInventoryAdapter.h"

const FCoastalContainerView& UCoastalUISessionComponent::SelectedView() const
{ return bStorageSide ? StorageView : BackpackView; }
bool UCoastalUISessionComponent::ReadViews()
{
    const FGuid Preferred = bViewsValid && SelectedView().Items.IsValidIndex(SelectedRow)
        ? SelectedView().Items[SelectedRow].InstanceId : FGuid();
    bViewsValid = false;
    if (!IntegrationReady() || Saves->IsBusy()) { UIError = TEXT("Inventory is unavailable or busy."); return false; }
    auto* Provider = Saves->GetInventory();
    FCoastalContainerView Player, Storage;
    const FName PlayerId(TEXT("container.player"));
    if (Provider->ReadContainerView(PlayerId, Player) != ECoastalProviderResult::Ready || !Player.IsValidFor(PlayerId))
    {
        BackpackView = {}; StorageView = {}; SelectedRow = -1;
        UIError = TEXT("DEVELOPMENT ERROR: AGIS ReadContainerView must return a validated backpack projection. No mock contents.");
        return false;
    }
    if (!StorageId.IsNone())
    {
        if (Bridge->ValidateOpenStorage() != ECoastalActionResult::Applied
            || Provider->ReadContainerView(StorageId, Storage) != ECoastalProviderResult::Ready
            || !Storage.IsValidFor(StorageId) || Storage.Revision != Player.Revision)
        { UIError = TEXT("Storage view unavailable, invalid, or from a different revision. Transfers disabled."); return false; }
        TSet<FGuid> PlayerInstances;
        for (const auto& Row : Player.Items) PlayerInstances.Add(Row.InstanceId);
        for (const auto& Row : Storage.Items)
            if (PlayerInstances.Contains(Row.InstanceId))
            { UIError = TEXT("The same item instance was reported in both containers. Transfers disabled."); return false; }
    }
    BackpackView = MoveTemp(Player); StorageView = MoveTemp(Storage); bViewsValid = true; UIError.Empty();
    const int32 MatchingRow = SelectedView().Items.IndexOfByPredicate([Preferred](const auto& Row)
        { return Preferred.IsValid() && Row.InstanceId == Preferred; });
    SelectedRow = MatchingRow >= 0 ? MatchingRow : coastal::CycleSelection(SelectedRow, 0, SelectedView().Items.Num());
    return true;
}
void UCoastalUISessionComponent::MoveSelection(int32 Direction)
{
    if (!bViewsValid || bTransferPending) return;
    const int32 Previous = SelectedRow;
    SelectedRow = coastal::CycleSelection(SelectedRow, Direction, SelectedView().Items.Num());
    if (Previous != SelectedRow) OnMenuFeedback.Broadcast(TEXT("select"));
    bReadTextPending = true;
    bRefreshPending = true;
}
void UCoastalUISessionComponent::TransferSelected(bool bRetry)
{
    if (StorageId.IsNone() || !IntegrationReady()) return;
    if (bRetry)
    {
        if (!bTransferPending) return;
        // Reuse the immutable operation GUID. A changed view does NOT create a second intention.
    }
    else
    {
        if (bTransferPending || !bViewsValid || !SelectedView().Items.IsValidIndex(SelectedRow)) return;
        const auto Selected = SelectedView().Items[SelectedRow];
        const auto Revision = SelectedView().Revision;
        if (!ReadViews()) { bRefreshPending = true; return; }
        const auto* Current = SelectedView().Items.FindByPredicate([&Selected](const auto& Row)
            { return Row.InstanceId == Selected.InstanceId; });
        if (SelectedView().Revision != Revision || !Current || Current->ItemId != Selected.ItemId
            || Current->Quantity != Selected.Quantity || Current->Position != Selected.Position || Current->Size != Selected.Size)
        { UIError = TEXT("Inventory changed. Review the refreshed selection before transferring."); bRefreshPending = true; return; }
        PendingTransfer.OperationId = FGuid::NewGuid(); PendingTransfer.ItemInstanceId = Selected.InstanceId;
        PendingTransfer.SourceContainer = bStorageSide ? StorageId : FName(TEXT("container.player"));
        PendingTransfer.DestinationContainer = bStorageSide ? FName(TEXT("container.player")) : StorageId;
        PendingTransfer.Quantity = 1; bTransferPending = true;
    }
    const auto Result = Bridge->TransferWithOpenStorage(PendingTransfer);
    // Busy/Failed are retained for an explicit retry of the SAME intention. Never auto-repeat.
    if (Result != ECoastalActionResult::Busy && Result != ECoastalActionResult::Failed)
    { bTransferPending = false; PendingTransfer = {}; }
    ReadViews(); bRefreshPending = true;
}
