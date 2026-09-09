#include "CoastalUISessionComponent.h"
#include "CoastalItemPreviewComponent.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalSaveCoordinator.h"
#include "CoastalPanelWidget.h"
#include "CoastalCharacterUI.h"
#include "GameFramework/PlayerController.h"

void UCoastalUISessionComponent::InspectSelectedItem()
{
    if (!bViewsValid || !SelectedView().Items.IsValidIndex(SelectedRow)) return;
    const auto Selected = SelectedView().Items[SelectedRow];
    const int64 Revision = SelectedView().Revision;
    if (!ReadViews()) return;
    const auto* Current = SelectedView().Items.FindByPredicate([&](const auto& Row)
        { return Row.InstanceId == Selected.InstanceId; });
    if (!Current || SelectedView().Revision != Revision || Current->ItemId != Selected.ItemId)
    { UIError = TEXT("Inventory changed. Select the item again."); bRefreshPending = true; return; }
    const auto Item = *Current;
    CloseItemPreview();
    ItemDetail = FText::FromString(Item.DisplayName.ToString() + TEXT("\n\n") + Item.Description.ToString()
        + FString::Printf(TEXT("\n\nQuantity: %d\nSize: %d x %d spaces"), Item.Quantity, Item.Size.X, Item.Size.Y));
    if (const auto* Mesh = ItemPreviewMeshes.Find(Item.ItemId); Mesh && IsValid(Mesh->Get()))
    {
        if (!IsValid(ItemPreview))
        {
            ItemPreview = NewObject<UCoastalItemPreviewComponent>(Controller);
            Controller->AddInstanceComponent(ItemPreview); ItemPreview->RegisterComponent();
            if (!ItemPreview->InitializePreview(Controller))
            { ItemPreview->DestroyComponent(); ItemPreview = nullptr; }
        }
        if (ItemPreview && ItemPreview->OpenPreview(Item.InstanceId, Revision, Mesh->Get()))
        { PreviewInstance = Item.InstanceId; PreviewItem = Item.ItemId;
          PreviewContainer = SelectedView().ContainerId; PreviewRevision = Revision; }
    }
    if (!Push(coastal::PanelKind::ItemDetails)) CloseItemPreview();
}

bool UCoastalUISessionComponent::ValidateItemPreview()
{
    if (!ItemPreview || !ItemPreview->IsPreviewOpen()) return false;
    const auto* Top = Flow.Top();
    bool Valid = IsInitialized() && bInputOwned && Top && Top->ticket.kind == coastal::PanelKind::ItemDetails
        && IsValid(Saves) && Saves->HasActiveCampaign() && !Saves->IsBusy() && !Saves->IsRecoveryRequired()
        && !Saves->IsPlayerReturnActive() && Epoch == Saves->GetSessionEpoch() && !bSessionReset
        && IsValid(Controller) && IsValid(Bridge) && Controller->GetPawn() == Bridge->GetOwner();
    FCoastalContainerView View;
    if (Valid)
    {
        auto* Provider = Saves->GetInventory();
        Valid = IsValid(Provider) && Provider->ReadContainerView(PreviewContainer, View) == ECoastalProviderResult::Ready
            && View.IsValidFor(PreviewContainer) && View.Revision == PreviewRevision
            && View.Items.ContainsByPredicate([this](const auto& Row)
                { return Row.InstanceId == PreviewInstance && Row.ItemId == PreviewItem; });
    }
    if (!Valid) CloseItemPreview();
    return Valid;
}

bool UCoastalUISessionComponent::RotateItemPreview(UCoastalPanelWidget* Sender, float Yaw, float Pitch)
{
    if (!IsValid(Sender) || Panels.IsEmpty() || Panels.Last() != Sender
        || !Flow.IsTop(Sender->Ticket) || !Sender->HasBeenPresented() || !ValidateItemPreview()) return false;
    return ItemPreview->RotatePreview(PreviewInstance, PreviewRevision, Yaw, Pitch);
}

UTextureRenderTarget2D* UCoastalUISessionComponent::ItemPreviewTexture() const
{
    if (Flow.Top() && Flow.Top()->ticket.kind == coastal::PanelKind::CharacterCreator)
        if (auto* Creator = FindCoastalCharacterCreator(IsValid(Bridge) ? Bridge->GetOwner() : nullptr)) return Creator->CharacterPreview();
    return ItemPreview && ItemPreview->IsPreviewOpen() ? ItemPreview->GetPreviewTexture() : nullptr;
}

void UCoastalUISessionComponent::CloseItemPreview()
{
    if (ItemPreview) ItemPreview->ClosePreview();
    PreviewInstance.Invalidate(); PreviewItem = PreviewContainer = NAME_None; PreviewRevision = -1;
}
