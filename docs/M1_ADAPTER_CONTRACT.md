# M1 — Real AGIS adapter contract

All names below are **our plugin's API**, not advertised or inferred vendor symbols. Inspect the installed package before implementing `BP_CoastalAGISAdapter`. Use AGIS as the only item/container authority; no shadow counts or success-returning mock is acceptable.

## Provider lifecycle

`GetProviderStatus` returns `Ready` only after the AGIS owner, item definitions, inventory grids, persistence codec and atomic mutation strategy are actually usable. The inherited default returns `NotConfigured`.

Initialize an empty, exportable provisional session with a valid GUID before calling the coordinator's `Configure`. The provisional session has an empty backpack, empty cabin storage, no receipts, and no granted battery/fuse. The required M1 copies exist as authored world pickups. This initialization is local adapter work, not performed by the default component.

`BuildNewInventory(CampaignId, OutSnapshot)` is read-only: build a fresh payload and empty ledger for the supplied GUID. It must not reset the live inventory as a side effect. `RestoreInventory` is the only point where the coordinator commits that initial snapshot. The M2 container-based item sources require a deliberate content/persistence update; do not seed the same required items both in containers and loose pickups.

## Method behavior

| Method | Required behavior |
|---|---|
| `CheckRequirements` | Read-only carried-backpack query. No remote storage. Reject unknown IDs/invalid quantities. |
| `TryCommitRequirements` | Campaign-scoped, synchronous all-or-none consumption. Check the receipt first, then availability. Same ID/different fingerprint fails. |
| `TryCollectWorldItem` | Insert one authored source's item into the backpack and record its pickup receipt atomically. Failure leaves both inventory and ledger unchanged. Never hide/destroy the actor here. |
| `TryTransfer` | Atomically move the requested quantity between mapped containers, preserving legitimate instance identity and grid consistency. Deduplicate OperationId. Same ID/different request fails. Full destination returns `NoSpace`. |
| `ExportInventory` | Read-only consistent export of actual item instances, grid positions, containers and receipt ledger under one campaign ID. |
| `ValidateInventory` | Read-only validation of arbitrary candidate campaign data, including a different campaign GUID from the live one. No mutations or lazy grants. |
| `RestoreInventory` | Atomic replacement of all inventory/container data and the ledger. On failure everything live is unchanged. Used for both load and rollback. |
| `BuildNewInventory` | Read-only initial snapshot construction, as above. |

Every operation is synchronous in this source version. A latent vendor API requires an explicit pending/completion redesign and new tests. Do not block with sleeps, mutate later after returning success, or call a two-step removal “atomic” without rollback/reservation evidence.

## Snapshot representation

`FCoastalInventorySnapshot` contains schema version 1, campaign GUID, provider ID, exact provider version, a nonempty payload and committed receipts. Provider ID/version are records of your actual implementation; their values are not supplied by this package.

The payload must contain enough actual AGIS data to reconstruct item definition references, stable item-instance GUIDs, quantities, rotations/positions, owner grids and all container contents. Do not serialize transient UObject addresses. Validate missing item definitions, invalid grids, overlaps, impossible stacks, duplicate instance IDs and unknown container mappings before replacing anything.

The explicit `Receipts` array is the adapter's authoritative transaction ledger. Export and restore it with `Payload` in the same snapshot. Do not create a competing independently saved ledger. If the vendor has its own transaction history, define and test how this adapter ledger is derived/reconciled without contradictory authority.

Validate critical-item conservation against collected-source and repair receipts. In the M1 map each battery/fuse has one guaranteed source. Before collection it is absent from inventory; after collection it has exactly one recoverable instance in player/cabin storage unless the repair receipt records its successful consumption. A structurally plausible payload with duplicated or vanished critical equipment is invalid.

## Canonical repair/pickup receipts

Use `UCoastalContractLibrary::MakeRequirementsFingerprint`. It validates IDs/quantities, sorts by logical item ID and produces an unambiguous versioned string. For the original radio repair:

```text
TransactionId: first_signal.radio_repair.v1
Fingerprint: requirements.v1|16:item.marine_fuse:1|18:item.radio_battery:1|
```

For the battery proxy:

```text
TransactionId: pickup.world.test.battery
Fingerprint: requirements.v1|18:item.radio_battery:1|
```

The fuse uses `pickup.world.test.fuse` and `requirements.v1|16:item.marine_fuse:1|`.

Scope every receipt to its snapshot's campaign GUID. A retry looks up the ledger **before** checking current item availability. `AlreadyCommitted` requires the same ID and fingerprint. Unknown IDs, zero quantities, partial insertion/removal or a different fingerprint must not create a receipt.

Transfer receipts should use `transfer.` followed by the operation GUID's lowercase digit form. Canonicalize item-instance GUID, source ID, destination ID and quantity together; document that format in the local implementation. Retrying an operation must reuse its original OperationId. A new intended transfer gets a new GUID. M1's native bridge requests destination auto-placement, not a specific grid coordinate.

## Inventory UI restrictions

Critical items cannot be discarded, destroyed, sold or consumed by another action. Disable these paths in the real AGIS UI, not only in the mission component. Map `container.player` to the actual backpack and `world.test.storage` to the cabin storage. Configure the starting test grids at 6 × 4 and 8 × 6 if the installed system supports them; these are project design settings, not vendor defaults.

All M1 transfers must pass through the guarded bridge. A vendor drag/drop callback that mutates directly bypasses the snapshot boundary and is not approved. Provide select/transfer/back actions usable from a gamepad; drag/drop alone does not meet the gate.

## Required local evidence

Test real insertion rejection, repeated insertion, full storage, partial-removal failure, commit retry after consumption, ID/fingerprint collision, post-repair save/relaunch, one part stored during relaunch, provider import failure and rollback failure. Record actual quantities, world visibility, ledger and quest before/after each test. Passing standalone helper tests does not establish any of these vendor behaviors.

Complete `data/m1_dependency_register.json` with actual classes, events, item definitions, persistence codec and version evidence. Null entries are deliberate unresolved dependencies.
