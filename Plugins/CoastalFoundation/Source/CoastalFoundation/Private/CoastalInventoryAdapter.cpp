#include "CoastalInventoryAdapter.h"

UCoastalInventoryAdapter::UCoastalInventoryAdapter()
{
    PrimaryComponentTick.bCanEverTick = false;
}
ECoastalAvailability UCoastalInventoryAdapter::CheckRequirements_Implementation(
    const TArray<FCoastalItemRequirement>& Requirements) const
{
    return ECoastalAvailability::NotConfigured;
}
ECoastalInventoryCommit UCoastalInventoryAdapter::TryCommitRequirements_Implementation(
    FName TransactionId, const TArray<FCoastalItemRequirement>& Requirements)
{
    return ECoastalInventoryCommit::NotConfigured;
}

ECoastalProviderResult UCoastalInventoryAdapter::GetProviderStatus_Implementation() const
{ return ECoastalProviderResult::NotConfigured; }
ECoastalInventoryCommit UCoastalInventoryAdapter::TryCollectWorldItem_Implementation(
    FName WorldId, FCoastalItemRequirement Item)
{ return ECoastalInventoryCommit::NotConfigured; }
ECoastalInventoryCommit UCoastalInventoryAdapter::TryTransfer_Implementation(const FCoastalTransferRequest& Request)
{ return ECoastalInventoryCommit::NotConfigured; }
ECoastalProviderResult UCoastalInventoryAdapter::ExportInventory_Implementation(FCoastalInventorySnapshot& Snapshot) const
{ Snapshot = {}; return ECoastalProviderResult::NotConfigured; }
ECoastalProviderResult UCoastalInventoryAdapter::ValidateInventory_Implementation(const FCoastalInventorySnapshot& Snapshot) const
{ return ECoastalProviderResult::NotConfigured; }
ECoastalProviderResult UCoastalInventoryAdapter::RestoreInventory_Implementation(const FCoastalInventorySnapshot& Snapshot)
{ return ECoastalProviderResult::NotConfigured; }
ECoastalProviderResult UCoastalInventoryAdapter::BuildNewInventory_Implementation(
    FGuid CampaignId, FCoastalInventorySnapshot& Snapshot) const
{ Snapshot = {}; return ECoastalProviderResult::NotConfigured; }

ECoastalProviderResult UCoastalInventoryAdapter::ReadContainerView_Implementation(
    FName ContainerId, FCoastalContainerView& View) const
{ View = {}; return ECoastalProviderResult::NotConfigured; }
