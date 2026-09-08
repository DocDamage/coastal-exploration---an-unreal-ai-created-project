# M1.3 — Checked Startup & Test-Room Integration Diagnostics

**Project:** Coastal Exploration / First Signal  
**Section date:** September 5, 2026, America/New_York  
**Plugin:** `0.1.3-m1-startup-source`  
**Status:** Source increment and standalone verification; no Unreal/vendor/Windows acceptance claim.

## 1. Why this section

The supplied M1.2 handoff implemented the native development interface but still required manual initialization in a precise order, a ready real AGIS provider and local Unreal validation. No evidence supplied with it showed that M1's playable acceptance gate had passed. This increment addresses the game-owned startup work without inventing vendor APIs or jumping to production coast assembly.

The new default integration path is one explicit controller-owned entry point. It checks enough of the host to report concrete failures before taking permanent bindings. The previous three-call path remains available for an already integrated host; they must not both own the same session.

This is not M2, a new coastline map, a replacement inventory or an engine compatibility certification. The next production world remains Cabin Cove → Shoreline Trail → Old Dock, with optional Rail Overlook, after the actual M1 gate.

## 2. New source

| Source | Responsibility |
|---|---|
| `CoastalSessionBootstrapComponent` | Explicit startup, repeat/reentry rejection, original save/bridge/UI call ordering and terminal partial-binding failures. |
| `CoastalStartupPreflight.cpp` | Same-world ownership, single-player controller/pawn, component counts, Enhanced Input, pristine room/provider and dry-spawn checks. |
| `CoastalIntegrationTypes` | Blueprint-readable issue code, subject and detail; report pass state. |
| `CoastalIntegrationLibrary` | Read-only M1 actor-manifest inspection, usable from editor or initial play world. |
| `CoastalProviderAudit.cpp` | Read-only queries against the real provisional adapter, never a synthetic inventory. |
| `CoastalPlacementLibrary` | One dry-destination check shared by startup and campaign restore. |
| `Core/BootstrapRules.h` | Engine-independent startup transition rules used by the native component. |
| `Core/TestRoomRules.h` | Exact seven-object M1 authoring contract, used by the native audit and standalone tests. |
| `tools/unreal/audit_systems_test.py` | Inspect the loaded editor world without creating/loading/saving a map or calling the provider. |
| `tools/integration_report.py` | Bounded manifest-only report validation and exclusive-new-file output. |

No campaign schema, repair receipt fingerprint, item ID, journal text, save namespace policy or world-object ID was migrated. The native placement body was moved into a shared library; finite positive capsule-dimension checks were added. No alternative destination policy or water system was introduced.

## 3. Startup lifecycle

`StartTestRoom` is called explicitly after possession and real provider readiness, not automatically from an arbitrary BeginPlay or a polling timer. Its logical map ID is deliberately fixed to `level.systems_test`. This component does not discover/select a production world.

| State | Meaning | Allowed next step |
|---|---|---|
| Idle | No initialization attempted. | Explicit StartTestRoom or read-only RunPreflight. |
| Checking | Read-only prerequisites are being checked. | Finish current attempt; recursive startup returns Busy. |
| Blocked | Preconditions failed before any binding. | Fix setup, explicitly retry. No automatic retries. |
| Binding | Save, bridge and UI initialization is underway. | Finish once; no reentrant initialization. |
| Ready | Native session menu attached. | User chooses New/Continue; identical startup call returns AlreadyStarted. |
| RestartRequired | Binding partially failed, or a repeated request discovered lost bound lifetime. | Exit/relaunch; never silently rebind. |
| Stopped | Component teardown occurred. | No further startup. |

Different director/provider/checkpoint arguments after Ready return Rejected. An identical repeated request rechecks key bound references instead of installing duplicate UI, input mappings or delegates. This is a check on an explicit repeated call, not a new continuous runtime watchdog. The existing UI retains its own session/pawn checks.

On partial binding failure, the coordinator's existing operation gate is poisoned, preventing further mutations and writes. The bootstrap takes its own movement/look locks and balances only those on EndPlay. It never resets another input owner's locks. It does not attempt to undo arbitrary vendor initialization, recreate a pawn or delete any save. The source emits a structured report, Output Log entry and development on-screen diagnostic; there is no new polished production startup-error screen. A failed UI allocation may therefore require closing the development executable or stopping PIE directly.

Notification recursion is rejected while OnStartupNotice is being broadcast. Busy, AlreadyStarted, Rejected and an already-terminal result are returned directly without repeated notifications or log spam.

## 4. What preflight actually checks

The bootstrap must be registered on the only local PlayerController in a standalone game/PIE world. The controller must possess the real ACharacter. Exactly one bootstrap and one uninitialized UI component belong to the controller, and exactly one unbound interaction bridge belongs to the character. The director must have its one native mission and save component, be in the same world and not already configured/poisoned.

EnhancedPlayerInput and the Enhanced Input local-player subsystem must exist before menu initialization. The native UI still creates only its own menu input component/context. The bootstrap does not edit host mappings, clear contexts, create a replacement GameMode or configure Hyper.

Both the supplied initial checkpoint and the current character position use the shared M1 capsule/floor check. It rejects invalid transforms, unsuitable capsule dimensions, blocking overlaps, missing/unwalkable floor and the existing `Coastal.UnsafeCheckpoint` floor-actor tag. It is still the earlier dry-room policy, not flood detection, route validation or a completed water-recovery feature.

### Exact room manifest

The seven persistent IDs remain `world.test.radio`, `world.test.storage`, `world.test.door`, `world.test.battery`, `world.test.fuse`, `world.test.note` and `world.test.postcard`. The native audit checks kinds, single-item battery/fuse grants, the postcard journal ID, missing/duplicate/unknown IDs and initial inactive state.

The optional door, note and postcard are required members of this specific authored test room so their tests can run. This does **not** make reading or using them a prerequisite for First Signal. Ordinary meshes, lights and other nonpersistent scenery are not part of the manifest and are not rejected merely for existing. Adding another persistent object requires a deliberate manifest revision; this strict M1 check is not a general-purpose world validator.

No transform, label, actor or item is auto-repaired. An audit identifies the problem; it does not silently rewrite the user's map.

## 5. Real-provider read diagnostics

The provider audit requires a registered real adapter instance in a play world. The base adapter stays unconfigured. The provisional session must export a valid schema-1 identity/version/nonempty bounded payload, with no pre-restored receipt ledger, and pass the adapter's own validation.

`container.player` and `world.test.storage` must both yield valid actual views, including empty grids. Their revisions must agree and no instance GUID can appear in both containers. Neither required radio part may be pregranted to either visible grid in the M1 provisional session. Independent availability checks for each part must return MissingItems. Repeated reads check revision stability and session/receipt metadata for obvious read-triggered changes.

These checks cannot inspect opaque AGIS bytes, discover every hidden container, prove that an unchanged revision is truthful, or certify that the provider's validation is correct. They do not establish insert/consume/transfer atomicity, restore rollback or critical-item conservation. The actual vendor implementation and fault cases still need the original M1 acceptance tests.

The audit never calls `TryCollectWorldItem`, `TryTransfer`, `TryCommitRequirements`, `RestoreInventory` or `BuildNewInventory`. The bootstrap never starts/loads a campaign or saves on the user's behalf. Provider initialization remains the host's responsibility; no vendor class or readiness-event name has been guessed.

## 6. Editor inspection and reports

The new editor script inspects the currently loaded editor world through the compiled native manifest function. It refuses PIE, but allows a dirty editor world and records its dirty packages. A pass is explicitly a statement about that loaded world, not proof that its on-disk map is equivalent.

It never loads or saves a level, writes actor properties, edits project settings, invokes AGIS or marks runtime tests passed. Optional JSON output requires an existing parent directory and a new `.json` filename. Existing reports are not overwritten. The report writer refuses unsupported runtime-pass claims and naive timestamps.

The script is supplied and syntax-checked. It has not run inside Unreal here. Its Python report-format tests use ordinary temporary files, not a mocked engine passed off as integration evidence.

## 7. Retained systems and small correction

M1 campaign snapshots, alternating verified save slots, mission facts, atomic/idempotent adapter contracts and all M1.2 menus remain. The inventory remains a development list, not a final grid. Hyper target selection and the real pressed-once action still feed `InteractionBridge.TryInteract`.

One existing failure path was tightened: unsuccessful menu-input installation now removes any partially created input resources and clears temporary references. Its native behavior still needs the local engine/controller tests; source inspection is not a rendered UI or engine compile test.

## 8. Verification and next execution

The standalone suites now include 94 startup-state assertions and 193 manifest assertions in addition to the retained 1,947 checks. The 128 manifest-subset cases are generated authoring assertions, not 128 player playthroughs. Twelve new report-tool tests join nineteen retained Python tests. Actual compiler, sanitizer and structural results are in [current validation](../VALIDATION_REPORT.md).

Four new Unreal automation tests are provided under `Coastal.M1Startup`; together with the retained groups there are eleven. None has been executed here. They include native fail-closed entry points and shared rules, not real AGIS or a fully rendered controller session.

Follow [the startup wiring guide](M1_3_STARTUP_WIRING.md), fill the actual dependency worksheet, compile the host, wire the real adapter/Hyper interface and execute the 26 new startup scenarios plus the retained UI/campaign checks. The milestone gate remains a Windows executable completing the coherent campaign loop, including relaunch/Continue and failure recovery. Further source-only checklists are not a substitute for that gate.

## References and boundaries

Design and sequence come from the supplied foundation/M1/M1.2 documents retained in this package. New code and stricter M1 startup policy are original project work. Official documentation was checked for engine entry points, not to certify a local build or change the selected engine:

- [Epic: AActor::GetComponents](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/AActor/GetComponents).
- [Epic: UEnhancedPlayerInput](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/EnhancedInput/UEnhancedPlayerInput).
- [Epic: UActorComponent::BeginPlay](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UActorComponent/BeginPlay?lang=en-US).
- [Epic: UnrealEditorSubsystem Python 5.7](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/UnrealEditorSubsystem?application_version=5.7).

The engine remains the user's locally verified compatibility choice. Documentation pages may display a newer version; that is not evidence this source or the owned assets compiled on it.
