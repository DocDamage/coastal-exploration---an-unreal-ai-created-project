# Acceptance tests and evidence ledger

**Status key:** `NOT RUN` means the actual in-engine scenario has not been tested. This document is a planned test suite, not a list of completed features. Standalone rule checks are recorded separately in `VALIDATION_REPORT.md`.

## Project and assets

| ID | Test | Required result | Initial status |
|---|---|---|---|
| BUILD-01 | Compile clean host project and source plugin. | Unreal Header Tool and Development Editor build succeed. | NOT RUN |
| BUILD-02 | Run all four supplied tests under Coastal.FirstSignal and Coastal.M1. | All four pass; this does not certify AGIS integration. | NOT RUN |
| BUILD-03 | Package and run Windows systems test map. | Launches without editor; no missing module/asset references. | NOT RUN |
| ASSET-01 | Open each essential vendor demo independently. | Record working version, dependencies, and warnings. | NOT RUN |
| ASSET-02 | Migrate selected cabin/dock assets. | Materials, references, collision, and door behavior are valid. | NOT RUN |
| ASSET-03 | Review temporary-asset register. | Every proxy is labeled; finished-art claims match actual content. | NOT RUN |

## Inventory and repair

| ID | Test | Required result | Initial status |
|---|---|---|---|
| INV-01 | Pick up battery with room available. | Exactly one inventory instance; world pickup removed only after success. | NOT RUN |
| INV-02 | Pick up battery with a full backpack. | Explicit capacity message; pickup remains; no hidden count change. | NOT RUN |
| INV-03 | Repeated input on the same pickup. | No duplicate items or missing world-state marker. | NOT RUN |
| INV-04 | Move a required part to storage and back. | Counts conserved, instance stable, objective readiness refreshes. | NOT RUN |
| INV-05 | Attempt transfer to full storage. | Source remains unchanged; understandable failure. | NOT RUN |
| INV-06 | Try to discard/sell/destroy a critical part. | Action unavailable; required item stays recoverable. | NOT RUN |
| INV-07 | Repair with only one part. | No part consumed; quest unchanged; missing-item prompt. | NOT RUN |
| INV-08 | Repair with both parts. | Exactly one of each consumed; one ledger commit; radio repaired. | NOT RUN |
| INV-09 | Repeat repair interaction 20 times. | No extra removal or rewards; no repeated adapter commit after repaired flag. | NOT RUN |
| INV-10 | Inject failure during two-item removal. | Complete rollback/reservation release; neither item lost. | NOT RUN |
| INV-11 | Replay an already committed transaction ID. | AlreadyCommitted; no consumption even though parts are now absent. | NOT RUN |
| INV-12 | Reuse a transaction ID with different requirements. | Explicit error; no state mutation. | NOT RUN |
| INV-13 | Remove/unwire the real adapter. | NotConfigured; no fake repair or disappearing items. | NOT RUN |

## Mission order and presentation

| ID | Test | Required result | Initial status |
|---|---|---|---|
| QUEST-01 | Follow intended route. | Objective completes and returns to exploration. | NOT RUN |
| QUEST-02 | Collect both parts before inspecting radio. | Valid progression; no missing trigger prerequisite. | NOT RUN |
| QUEST-03 | Skip note, cabin trigger, and overlook. | Required mission still completes. | NOT RUN |
| QUEST-04 | Try listening before repair. | No completion flag. | NOT RUN |
| QUEST-05 | Acknowledge transcript with audio muted/skipped. | Clear transcript and valid completion. | NOT RUN |
| QUEST-06 | Replay transmission. | No duplicate journal entry or reward. | NOT RUN |
| QUEST-07 | Finish and inspect future lead. | Journal lead exists; no button opens an unbuilt destination. | NOT RUN |

## Saving and recovery

| ID | Test | Required result | Initial status |
|---|---|---|---|
| SAVE-01 | Save before acquiring either part; relaunch. | Both world pickups remain. | NOT RUN |
| SAVE-02 | Save carrying one part; relaunch. | One in inventory, only the other remains in world. | NOT RUN |
| SAVE-03 | Save with a required part in cabin storage. | Stored part restored; correct carried-inventory objective. | NOT RUN |
| SAVE-04 | Save after repair but before transmission. | Parts consumed, radio repaired, message still available. | NOT RUN |
| SAVE-05 | Save after full completion; relaunch. | Complete objective, persistent journal, no respawned quest loot. | NOT RUN |
| SAVE-06 | Request save during a repair callback. | Coordinator waits for consistent mutation boundary. | NOT RUN |
| SAVE-07 | Simulate write failure/denied path. | Visible error and previous valid generation preserved. | NOT RUN |
| SAVE-08 | Corrupt newest slot. | Explicit recovery to previous validated generation. | NOT RUN |
| SAVE-09 | Load unsupported schema/campaign mismatch. | Reject without partial restore or silent new game. | NOT RUN |
| SAVE-10 | Start a new game after completion. | New campaign ID and fresh inventory/ledger/world/quest together. | NOT RUN |
| SAVE-11 | Attempt load with missing inventory provider. | Continue blocked clearly; last valid save not overwritten. | NOT RUN |
| SAVE-12 | Reload while player transform is obstructed. | Safe validated fallback checkpoint, no loss of inventory. | NOT RUN |

## Input, traversal, and performance

| ID | Test | Required result | Initial status |
|---|---|---|---|
| UI-01 | Complete opening using only gamepad. | All navigation, transfer, journal, and settings operations accessible. | NOT RUN |
| UI-02 | Interact while inventory/pause menu is open. | No world interaction underneath UI. | NOT RUN |
| UI-03 | Close nested menus with Escape/B. | Only top layer closes; focus restored. | NOT RUN |
| UI-04 | Change input device mid-menu. | Correct prompts and usable focus. | NOT RUN |
| MOVE-01 | Traverse cabin doorway/corners. | Camera does not remain trapped in walls; player can see interactions. | NOT RUN |
| MOVE-02 | Aim at an item through a wall or outside character reach. | Interaction denied even if camera can see it. | NOT RUN |
| MOVE-03 | Fall into deep water/out of bounds. | Safe shore recovery, understandable message, no item loss or loop. | NOT RUN |
| PERF-01 | Repeat full route on baseline PC at 1080p. | Record measured frame times, memory, and streaming hitches. | NOT RUN |
| PERF-02 | Repeat after importing additional dressing. | Compare against baseline; investigate regressions before approval. | NOT RUN |
| RELEASE-01 | Launch packaged game from a clean working folder. | No editor or developer-content path dependency. | NOT RUN |

## Evidence format

For every executed scenario record: source revision, engine/toolchain versions, asset versions, hardware, steps, expected/actual result, date, and evidence path. Screenshots or logs must come from the actual game/editor, not generated mockups. A failed test remains failed until re-executed after its fix.

The release gate requires the relevant scenarios above to pass. Do not replace them with a blanket “all tests pass” statement based only on the standalone C++ rules.

## Additional M1 failure gates

| ID | Test | Required result | Status |
|---|---|---|---|
| M1-01 | Duplicate an actor without changing its stable ID. | Configure refuses duplicate manifest. | NOT RUN |
| M1-02 | New game in an existing save-set namespace. | Refused; both files retained. | NOT RUN |
| M1-03 | Two valid slots with equal generation or different campaign GUIDs. | Ambiguous pair refused. | NOT RUN |
| M1-04 | One future-schema slot beside an older valid slot. | No silent downgrade or overwrite. | NOT RUN |
| M1-05 | Inject late placement failure then provider rollback failure. | Explicit RecoveryRequired; interactions/saves blocked. | NOT RUN |
| M1-06 | Acknowledge a transcript token from before a successful load. | Stale acknowledgement rejected. | NOT RUN |
| M1-07 | Transfer after leaving storage reach or closing context. | Rejected without item changes. | NOT RUN |
| M1-08 | Call old note method directly without journal/world update. | Inconsistent save rejected. | NOT RUN |
| M1-09 | Save API reports failure but a newer valid disk generation exists. | Next write blocked until validated reload. | NOT RUN |
| M1-10 | Run generator with unsaved changes or an existing destination map. | Refused without overwriting user content. | NOT RUN |
