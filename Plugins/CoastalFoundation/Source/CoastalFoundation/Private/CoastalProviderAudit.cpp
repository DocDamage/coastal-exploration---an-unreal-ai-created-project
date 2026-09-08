#include "CoastalIntegrationLibrary.h"
#include "CoastalInventoryAdapter.h"
#include "CoastalInventoryView.h"
#include "Engine/World.h"

bool UCoastalIntegrationLibrary::AuditProvisionalProvider(UCoastalInventoryAdapter* Provider,
    FCoastalIntegrationReport& Report)
{
    Report = {};
    auto Fail = [&Report](const TCHAR* Code, const TCHAR* Message)
    { Report.Add(FName(Code), TEXT("AGIS adapter"), Message); };
    if (!IsInGameThread() || !IsValid(Provider))
    { Fail(TEXT("provider.missing"), TEXT("Supply the actual local game-thread adapter instance.")); return false; }
    if (Provider->GetClass() == UCoastalInventoryAdapter::StaticClass())
    { Fail(TEXT("provider.not_ready"), TEXT("The base adapter is intentionally unconfigured; initialize the real AGIS subclass.")); return false; }
    if (!Provider->IsRegistered() || !Provider->GetWorld() || !Provider->GetWorld()->IsGameWorld()
        || Provider->HasAnyFlags(RF_ClassDefaultObject))
    { Fail(TEXT("provider.session_instance"), TEXT("Use a registered provider instance in the play world, not an editor instance or class default.")); return false; }
    if (Provider->GetProviderStatus() != ECoastalProviderResult::Ready)
    { Fail(TEXT("provider.not_ready"), TEXT("Initialize the real AGIS provisional session before startup.")); return false; }
    FCoastalInventorySnapshot Before;
    if (Provider->ExportInventory(Before) != ECoastalProviderResult::Ready)
    { Fail(TEXT("provider.export_failed"), TEXT("A fresh provisional session must be exportable before startup.")); return false; }
    if (Before.SchemaVersion != 1 || !Before.CampaignId.IsValid() || Before.ProviderId.IsNone()
        || Before.ProviderVersion.IsEmpty() || Before.Payload.IsEmpty() || Before.Payload.Num() > 12 * 1024 * 1024)
        Fail(TEXT("provider.invalid_snapshot"), TEXT("Provisional snapshot schema/identity/version/payload is invalid."));
    if (!Before.Receipts.IsEmpty())
        Fail(TEXT("provider.not_pristine"), TEXT("Do not restore an old campaign independently; startup expects no committed provisional receipts."));
    if (!Report.Issues.IsEmpty()) return false;
    if (Provider->ValidateInventory(Before) != ECoastalProviderResult::Ready)
    { Fail(TEXT("provider.validation_failed"), TEXT("The adapter rejected its own provisional inventory snapshot.")); return false; }
    FCoastalContainerView Backpack, Storage, Recheck;
    const FName PlayerId(TEXT("container.player")), StorageId(TEXT("world.test.storage"));
    const bool bBackpack = Provider->ReadContainerView(PlayerId, Backpack) == ECoastalProviderResult::Ready
        && Backpack.IsValidFor(PlayerId);
    const bool bStorage = Provider->ReadContainerView(StorageId, Storage) == ECoastalProviderResult::Ready
        && Storage.IsValidFor(StorageId);
    if (!bBackpack) Fail(TEXT("provider.backpack_view"), TEXT("ReadContainerView must return a valid actual backpack view, including when empty."));
    if (!bStorage) Fail(TEXT("provider.storage_view"), TEXT("Map world.test.storage to a real initialized storage container and return its view."));
    if (!bBackpack || !bStorage) return false;
    if (Backpack.Revision != Storage.Revision)
        Fail(TEXT("provider.incoherent_revision"), TEXT("Backpack and storage reads must share one live inventory revision."));
    TSet<FGuid> Instances;
    bool bSharedInstance = false, bPregranted = false;
    for (const auto* View : {&Backpack, &Storage})
        for (const auto& Item : View->Items)
        {
            bSharedInstance |= Instances.Contains(Item.InstanceId);
            Instances.Add(Item.InstanceId);
            if (Item.ItemId == TEXT("item.radio_battery") || Item.ItemId == TEXT("item.marine_fuse"))
                bPregranted = true;
        }
    if (bSharedInstance) Fail(TEXT("provider.shared_instance"), TEXT("One or more item instances appeared in both containers."));
    if (bPregranted) Fail(TEXT("provider.pregranted_part"), TEXT("M1 radio parts must begin as world pickups, not pregranted in either grid."));
    for (const FName Part : {FName(TEXT("item.radio_battery")), FName(TEXT("item.marine_fuse"))})
    {
        FCoastalItemRequirement Requirement; Requirement.ItemId = Part; Requirement.Quantity = 1;
        if (Provider->CheckRequirements({Requirement}) != ECoastalAvailability::MissingItems)
            Fail(TEXT("provider.requirements_mismatch"), TEXT("Each absent radio part must independently report MissingItems, not Available or NotConfigured."));
    }
    // A second bounded read detects obvious read-triggered mutations. It cannot certify opaque vendor internals.
    if (Provider->ReadContainerView(PlayerId, Recheck) != ECoastalProviderResult::Ready
        || !Recheck.IsValidFor(PlayerId) || Recheck.Revision != Backpack.Revision)
        Fail(TEXT("provider.unstable_reads"), TEXT("Read-only queries changed or lost the backpack revision."));
    FCoastalInventorySnapshot After;
    if (Provider->ExportInventory(After) != ECoastalProviderResult::Ready
        || After.CampaignId != Before.CampaignId || After.ProviderId != Before.ProviderId
        || After.ProviderVersion != Before.ProviderVersion || After.SchemaVersion != Before.SchemaVersion
        || !After.Receipts.IsEmpty() || After.Payload.IsEmpty() || After.Payload.Num() > 12 * 1024 * 1024)
        Fail(TEXT("provider.read_side_effect"), TEXT("Read-only validation changed/lost session identity or created a receipt."));
    Report.Finish(); return Report.bPassed;
}
