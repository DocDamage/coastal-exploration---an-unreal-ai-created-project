# M1 — Systems Test Room & Campaign Persistence

**Project:** Coastal Exploration / First Signal  
**Section date:** September 5, 2026  
**Delivery:** Expanded original source, test-room editor recipe, tests, and integration handoff  
**Milestone status:** M1 source work advanced; the in-engine M1 completion gate has not passed.

## 1. Purpose and scope

The supplied foundation defines M1 as compatibility checks and a functional test room before assembling the coast. This section implements the game-owned source layer needed for that room. It does not jump ahead to fishing, boats, combat, an offshore level, or a larger world.

The intended local test loop is: start a campaign, inspect the radio, collect both parts, move one into storage and back, repair the radio, acknowledge its transcript, save, quit, and continue with a coherent world. Pickups must not duplicate, the repair must not partly consume equipment, and a failed load must not silently start a new game.

The actor/map presentation is deliberately labelled **DEV PROXY**. It is not finished environment or character art. The original coastal opening, real asset integration, final menus and Windows package are still later work.

## 2. Implemented source in this delivery

| Area | Source implementation | Boundary |
|---|---|---|
| Campaign snapshot | Schema, campaign ID, generation, map ID, player/checkpoint transforms, inventory section, receipt ledger, world records, journal, mission. | Actual AGIS payload serialization must be implemented locally. |
| Save coordinator | Non-overlapping mutation/save/load scopes; deferred/coalesced autosave requests; explicit new/load/save results. | Small synchronous disk operations; not an asynchronous performance solution. |
| Save resilience | Alternating slots, outer CRC32 envelope, full read-back equality, semantic revalidation, generation checks and damaged-slot fallback. | Not a guarantee of crash-safe disk atomicity. Requires real platform fault tests. |
| Restore | Read-only validation, inventory replacement, world/journal/mission restoration, safe-position checks, rollback and fail-stop lock. | Provider restore must actually be atomic; late rollback requires real-engine testing. |
| Persistent world actors | Authored IDs, duplicate rejection, door state, collected pickup state, optional note/discovery state. | Native development proxies, not final vendor actors. |
| Interactions | Character-origin reach/visibility checks, UI blocking, pickup/repair/transfer guards, storage context and transcript tokens. | Hyper focus/prompt and real input mapping are not wired here. |
| Story presentation | Objective text, maintenance/postcard text, radio transcript, North Reach journal lead. | Blueprint widgets and audio presentation are not supplied. |
| Test room | Editor Python script consumes a centimetre-based layout and creates a fresh proxy level. | Script is syntax-checked only; no map was generated in Unreal here. |
| Verification | Original mission tests plus save-envelope, campaign/gate/fingerprint and Python tooling checks. | Standalone tests are not Unreal, AGIS, Windows or gameplay tests. |

## 3. Code ownership

`ACoastalMissionDirector` contains one `UFirstSignalComponent` and one `UCoastalSaveCoordinator`. Place exactly one director. The coordinator caches the test map's `ACoastalWorldObject` actors during `Configure`; it does not search the entire level every frame. Do not spawn or destroy registered actors after configuration. Collected proxies are hidden, not destroyed, so they can be restored.

`UCoastalInteractionBridge` belongs on the actual player character. Hyper supplies its selected actor to `TryInteract`; the bridge decides whether the action is legal and coordinates the mutation. It is not another focus-scanning framework.

`UCoastalInventoryAdapter` is still a game-owned boundary around AGIS, not an inventory implementation. Its default methods return `NotConfigured`. The expanded methods must be overridden using the installed AGIS package's real symbols. The source contains no guessed AGIS or Hyper class names.

`UCoastalCampaignSave` serializes the whole campaign. `Core/CampaignRules.h`, `Core/SaveEnvelope.h` and `Core/TransactionRules.h` are the exact engine-independent helpers used by the runtime source and standalone tests.

## 4. The test room

The editor recipe creates `/Game/Coastal/Maps/L_SystemsTest`, with logical map ID `level.systems_test`. The floor is 3,600 × 2,400 centimetres. It includes a player start, bounding walls, a visibility-test obstruction, a director, and seven labelled objects: radio, storage chest, optional door, battery pickup, fuse pickup, maintenance note and optional postcard.

The two required items are standalone proxies in M1, not the production dock containers. Their stable IDs are `world.test.battery` and `world.test.fuse`. The M2 production map must deliberately map or migrate state; changing a map/manifest is not automatically compatible with an M1 save.

The generator refuses an existing destination map, active Play In Editor, or unsaved editor packages. It checks plugin classes and the engine cube mesh before creating anything. If generation fails after creation, it leaves the new partial map for inspection rather than deleting user work. The script does not change project-wide input or rendering configuration.

## 5. Save protocol and guarantees actually implemented

### 5.1 Save sets and campaign identity

A save set is a user/developer-selected, lowercase logical name, such as `m1_test_01`. It produces `Coastal_m1_test_01_A` and `Coastal_m1_test_01_B` through Unreal's save-slot API. The snapshot contains a separately generated campaign GUID.

`StartNewCampaign` refuses any save set with an existing A or B file. It does not delete saves or silently replace a completed game. For a fresh test use a new name. A future title/menu layer should remember the selected set; this source delivery does not include a save-selection UI or a last-played profile catalogue.

`StartedNew` means a new campaign exists **in memory** and its initial save is queued. Only the later `Saved` result means a write passed read-back verification. Do not show “saved” merely because new-game creation succeeded.

### 5.2 Mutation and autosave ordering

The bridge owns the outer mutation scope. Inventory mutation, native world-state updates and mission/journal updates finish before that scope releases. A save requested during repair is coalesced and handled on a later coordinator tick. It is not captured inside an arbitrary mission notification.

Saving and loading are synchronous and non-overlapping in M1. The coordinator's tick runs after ordinary update work and can tick while paused. Save requests raised by restoration notifications are ignored during IO, preventing a load callback from producing a mixed snapshot. This policy is intentionally small and inspectable; larger payloads need measured asynchronous IO work later without changing the snapshot boundary.

### 5.3 Snapshot validation

Before writing or restoring, the source checks supported schemas/map, campaign identity, generation, transforms, section sizes, duplicate receipts, duplicate world IDs, object kinds, journal duplicates, and mission consistency. Required coherence includes:

- A repaired radio has the matching repair receipt and canonical requirement fingerprint.
- Each collected pickup has the matching pickup receipt; uncollected pickups do not.
- A completed transmission has both its transcript entry and the North Reach lead.
- Reading the maintenance note agrees with its world state and journal entry.
- Radio and storage actors do not maintain a second copy of mission/container state.

The AGIS adapter separately validates item definitions, instance IDs, grids, container contents, quantities, critical-item conservation and the receipt ledger. Outer snapshot validation cannot infer these facts from opaque vendor bytes.

### 5.4 Alternating writes and recovery

The coordinator inspects both slots, chooses the highest complete valid generation, then writes only the opposite slot. The payload is wrapped with a magic value, envelope version, payload length and CRC32. It is reread and compared byte-for-byte, then decoded and semantically validated before the in-memory generation advances.

If one slot is damaged and the other validates, loading returns `RecoveredPrevious` with an explicit message. The damaged slot might have been older or newer; its corrupt metadata is not trusted. Equal valid generations, incompatible schemas/provider data, or two valid slots from different campaigns block automatic resolution. No valid generation means no load, not a new game.

After an ambiguous write failure, a newer valid disk generation may exist even though success was not confirmed in memory. Subsequent saves refuse to overwrite that newer generation; reload and validate it before continuing. This is deliberate protection, not a reason to erase the files.

The CRC detects accidental corruption. It is not encryption, authentication, a hostile-save parser, or an anti-cheat mechanism. The standalone inspection tool checks only the envelope. It cannot certify the inner Unreal object or AGIS campaign.

### 5.5 Restore and rollback

The coordinator captures the previous live campaign first. The provider must replace items, containers and ledger atomically. Native world objects, journal and mission then restore, and the character is placed at a validated saved location or dry checkpoint.

The placement helper checks capsule obstruction, a walkable floor and a `Coastal.UnsafeCheckpoint` exclusion tag. It checks again after restoring door collision. This is an M1 dry-room placement check, not the complete water-recovery system from M3.

A late restore failure triggers replacement of the previous inventory and native state. If rollback itself fails, interactions and saves lock and the user must exit and relaunch. Disk saves are not modified by loading. Movement/menu recovery presentation still needs local integration; no invisible “successful restore” is reported.

## 6. Interaction behavior

Pickups call the real adapter first, then mark and hide the actor only for `Committed` or `AlreadyCommitted`. `NoSpace`, missing configuration and other failures leave the proxy visible. Pickup receipts use `pickup.` plus the stable world ID.

Radio interaction first inspects an uninspected radio, then attempts repair when unrepaired, then opens the transcript after repair. Repeated repair never consumes again after the mission's repaired flag. The original order-independent equipment rules are preserved.

Storage opening requires a bound UI callback. Transfers require an open, in-range storage actor, a valid item-instance GUID, a positive quantity, a unique operation GUID, and endpoints limited to `container.player` and that storage object's stable ID. Auto-placement inside the destination grid is provider work. Arbitrary remote-container transfer is not exposed by the bridge.

A transcript callback receives the actual text plus an acknowledgement token. The widget acknowledges only after the text is displayed and the player explicitly continues. Cancelling grants no completion. Tokens from a previous successfully loaded/new session are rejected. Replaying the message does not duplicate journal entries.

Widget-specific UI blockers prevent world interactions under menus. This is input-permission infrastructure, not a completed menu focus stack. Gamepad navigation, one-layer Back behavior and Enhanced Input context ownership remain local wiring/test tasks.

## 7. Local execution order

1. Back up the host project and any existing adapter implementation. Compile the untouched standard Third Person C++ template against the candidate engine. Verify the actual cabin, AGIS and Hyper packages separately and record their versions.
2. Install this complete plugin folder. Build Development Editor / Win64 using Unreal Header Tool and UnrealBuildTool. Do not call a standalone C++ pass an Unreal build.
3. Run `Coastal.FirstSignal` and `Coastal.M1` editor automation groups. There are four supplied engine tests; none ran in this delivery environment.
4. Enable editor Python and Editor Scripting Utilities, save work, and execute the test-room script. Select the host template's real Third Person GameMode in the generated map.
5. Implement the adapter against the actual AGIS API. It must provide an exportable provisional session before coordinator configuration, without granting quest items to the backpack. See the dedicated adapter contract.
6. Add/configure the interaction bridge on the player. Configure the director once with the real adapter, actual character and `level.systems_test`. Bind Hyper, action feedback, storage and transcript callbacks.
7. Build the minimal M1 widgets and debug Start/Save/Continue actions described in the wiring guide. Use a fresh save-set name and confirm the initial verified-save notification.
8. Execute full-backpack, repeated-pickup, partial-repair failure, storage round-trip, post-repair relaunch and damaged-slot tests. Fix failed scenarios before introducing production coastal art.
9. Package `L_SystemsTest` for Windows and launch it outside the editor. Record engine, toolchain, vendor versions, source revision and evidence paths.

## 8. Validation boundary

The delivery contains the retained 407 original mission assertions, 227 campaign/transaction assertions and 583 save-envelope assertions: **1,217 C++ assertions**, executed under both GCC and Clang. Ten Python test cases cover the envelope-inspection/corrupt-copy utility. These numbers count assertions and Python test cases, not thousands of gameplay scenarios.

The local evidence report records compiler output, structural checks and any sanitizer run. Engine automation source is provided but unexecuted. Unreal is not installed in this execution environment; the purchased packages and host project are not mounted here. Therefore Unreal compilation, editor map generation, actual AGIS/Hyper integration, end-to-end save/load, controller UX and Windows packaging remain **NOT RUN**.

## 9. M1 acceptance gate and next section

M1 passes only when the real test room can perform pickup → storage transfer → repair → transcript → save/relaunch/continue without loss or duplication, missing providers fail visibly, fault tests preserve recoverable saves, and the Windows package launches outside the editor.

After that gate, M2 is **Cabin Cove, Shoreline Trail, Old Dock and Rail Overlook**, using verified owned art and the existing objective systems. It is not an offshore platform mission or a fishing expansion.

## References and source boundaries

The original design basis is the supplied `COASTAL_EXPLORATION_FOUNDATION.md`; the code and design decisions in this M1 section are original implementation work. Official documentation was checked for the engine serialization/editor entry points, not to certify this plugin or the user's vendor installation.

- [Epic: Saving and Loading Your Game](https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine?lang=en-US)
- [Epic: UGameplayStatics API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameplayStatics?lang=en-US)
- [Epic: SaveGameToMemory](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameplayStatics/SaveGameToMemory?lang=en-US)
- [Epic: EditorActorSubsystem, Python 5.7](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EditorActorSubsystem?application_version=5.7)
- [Epic: LevelEditorSubsystem, Python 5.7](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/LevelEditorSubsystem?application_version=5.7)
- [Epic: EditorLoadingAndSavingUtils, Python 5.7](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EditorLoadingAndSavingUtils?application_version=5.7)
