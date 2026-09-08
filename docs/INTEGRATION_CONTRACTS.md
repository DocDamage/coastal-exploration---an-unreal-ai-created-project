# Integration contracts

This document defines **our required behavior**. It is not a claim that AGIS, Hyper Interaction, or any other purchased product already implements these exact APIs.

> M1 extends this foundation contract. Read `M1_ADAPTER_CONTRACT.md` for the actual new methods, and `M1_BLUEPRINT_WIRING.md` for current setup.

## 1. Core ownership boundary

The plugin now owns First Signal facts, native world proxies, the guarded action bridge and campaign save coordination. AGIS still owns item counts, grid positions and container contents. UI focus, actual Hyper wiring, final player presentation and provider persistence remain project integration work.

The real AGIS instance is the only inventory authority. The project-owned adapter translates logical item IDs into real installed AGIS definitions. Do not maintain shadow item counts in the quest component.

## 2. Availability query

`CheckRequirements(requirements)` is read-only and synchronous. Inspect carried inventory only. Return `Available`, `MissingItems`, `NotConfigured`, or `Failed`. Invalid/unknown logical IDs, invalid quantities, and a missing provider must fail explicitly, not count as zero-cost requirements.

The proposed mission requirements are one `item.radio_battery` and one `item.marine_fuse`. They are fixed in `GetRepairRequirements()` for this minimal source starter. Any change must update the JSON and C++ together; the validator checks their IDs and quantities.

For HUD rendering, `GetObjective()` yields a coarse objective even if the adapter is missing. The UI must separately inspect availability and show the integration error in development builds. Do not interpret `FindEquipment` as proof that the provider is healthy.

## 3. Repair transaction

`TryCommitRequirements(transactionId, requirements)` is a synchronous, all-or-nothing operation. The initial ID is `first_signal.radio_repair.v1`. The ledger key is the active campaign ID plus that transaction ID. Reset or replace the entire ledger on a new campaign.

Required algorithm:

1. Validate the active campaign, adapter, item-definition mappings, positive quantities, and canonical requirements fingerprint.
2. Consult the campaign transaction ledger **before checking item availability**. If the same ID and requirements were already committed, return `AlreadyCommitted` without removing anything. An ID reused with different requirements is an error.
3. Enter the game-owned mutation guard; reject conflicting inventory operations during the transaction.
4. Validate every required item and reserve all required quantities. No partial consumption is permitted.
5. Commit removal of all requirements and record the transaction ID/fingerprint with the inventory state. If the provider cannot guarantee all-or-nothing behavior, implement tested rollback/reservation logic before enabling this adapter.
6. Return `Committed`. Failures return `MissingItems`, `NotConfigured`, or `Failed` with every inventory quantity and ledger entry unchanged.
7. The quest component marks the radio repaired only for `Committed` or `AlreadyCommitted`, then broadcasts state change. Queue unified saving after the outer mutation completes.

The plugin guards reentrant repair requests while the adapter is running. After repair, repeated requests short-circuit and never call the adapter again. The guard does not replace AGIS's inventory correctness or the campaign save coordinator.

If the real vendor operation is asynchronous or latent, do not pretend it implements this synchronous signature. Add an explicit pending/completion contract, keep the mutation/save boundary correct, and extend the tests before using it. No blocking sleep or guessed timeout should turn “pending” into “success.”

## 4. Interaction bridge

Hyper selects an actor and presents focus; our bridge decides whether its game action is permitted. Resolve the actor and prompt once per appropriate focus update, not through redundant world searches every tick.

Before a pickup or repair, validate: player is active; UI does not own the input; target is still valid; character-origin reach and line of sight pass; no load or conflicting mutation is in progress. Route radio inspection and repair to the mission component. Route storage opening and transfers to AGIS.

Do not assume camera line of sight proves character reach. Do not give the radio actor its own copied battery/fuse counts. Do not let both the vendor demo prompt and our prompt respond to the same key press.

## 5. Pickup and storage transaction contracts

Each persistent pickup/container has a stable, authored world ID. Do not use transient actor addresses or editor-generated display labels as save identifiers.

For a pickup: confirm the world ID has not already been consumed, attempt the actual inventory insertion, then mark the world ID consumed and remove/hide the actor **only after success**. Perform these changes in one game-owned mutation scope. A rejected insertion leaves the object available in the world.

For storage: either the whole requested transfer succeeds or both source and destination remain unchanged. UI refresh follows the result. No destroy/recreate loop that loses item instance IDs or attachments. The first build does not need nested bags, but it must not corrupt existing inventory data.

## 6. Unified persistence

The plugin's `FFirstSignalSnapshot` is one section of a save, not the save itself. Export it alongside the inventory, transaction ledger, world registry, journal, and player state under a shared campaign ID and generation.

Important example: the battery/fuse have been consumed and the transaction ledger says committed. On a coherent snapshot the radio is also repaired. If an older objective section must be reconciled during a migration, the committed transaction may recover the repair without consuming again. Normal saves must never deliberately split these sections across generations.

Load validation must reject unsupported schema versions, missing adapters, unknown required item definitions, impossible objective facts, mismatched campaign IDs, and structurally invalid inventory data. It must not replace a failed load with a silent new game.

`RestoreSnapshot()` rejects unknown local schema versions and impossible boolean states. It does not validate the rest of the campaign, nor prove the inventory ledger matches. The coordinator must validate those before calling it. Restore triggers `OnStateChanged`; suppress autosaves during the entire load.

Settings can live separately, but gameplay state must not be scattered across unrelated slots without an explicit transaction design. Use a tested two-generation strategy and a visible fallback/recovery message. Never erase the last valid save merely to clear an error.

## 7. Journal and transmission

Use stable journal entry IDs, such as `journal.first_signal.maintenance`, `journal.first_signal.postcard`, and `journal.first_signal.transmission`. Insertion is idempotent. Opening the journal must not grant equipment or mark a repair complete.

`FinishRadioTransmission()` may be called only after the transcript is available and the player finishes or explicitly acknowledges it. Calling it before repair returns `NotRepaired`. Repeating it returns `AlreadyApplied`; no duplicate rewards or journal entries.

## 8. Integration mapping table to complete locally

| Logical requirement | Actual installed symbol/path | Status |
|---|---|---|
| AGIS inventory owner | To inspect | Unverified |
| AGIS item definition for battery | To create/map | Unimplemented |
| AGIS item definition for fuse | To create/map | Unimplemented |
| Atomic consume/reserve capability | To inspect or implement around provider | Unverified |
| Inventory export/import representation | To inspect | Unverified |
| Hyper interaction interface/action event | To inspect | Unverified |
| Input ownership and prompt replacement | Project implementation | Unimplemented |
| Campaign save coordinator | `UCoastalSaveCoordinator` | M1 source implemented; engine tests not run |

An unfilled row is a real dependency. Never replace it with a guessed vendor class name and call integration complete.
