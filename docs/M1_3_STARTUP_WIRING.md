# M1.3 — Local Build & Checked Startup Wiring

## 1. Preserve the existing project

Back up/commit the host project, current saves and the project-owned AGIS adapter. This ZIP contains the complete updated `Plugins/CoastalFoundation` folder; replace that folder rather than copying a handful of headers. Do not place the entire archive inside Plugins, remove purchased content or overwrite the host project configuration.

Rebuild the actual Development Editor / Win64 target against the locally selected engine. The module dependencies are retained from M1.2; there is no new vendor dependency or pinned EngineVersion. Old compiled DLLs do not contain the new reflected startup/diagnostic classes. No local build has been performed in this delivery environment.

Record real paths and versions in [the integration worksheet](../data/m1_3_integration_worksheet.json). Its null entries are deliberate. The supplied [host preflight](../tools/preflight_host.py) still provides read-only path/tool checks, not a compiler or vendor API audit.

## 2. Keep one owner for each system

Use the M1 test map and actual Third Person GameMode/character. Keep one native CoastalMissionDirector. Keep one real AGIS adapter on its existing well-defined inventory owner. On the possessed character, keep exactly one CoastalInteractionBridge.

On the local PlayerController, add exactly one CoastalUISessionComponent and one CoastalSessionBootstrapComponent. Do not add either to the pawn or director. The bootstrap does not add missing components automatically.

Use either the new startup path or the old M1.2 manual calls, not both. For the new path remove/bypass the old startup graph's calls to `Saves.Configure`, `Bridge.Configure` and `InitializeUI`; the bootstrap performs those in order after preflight. Do not remove Hyper's actual focus/interaction wiring or the real AGIS initialization itself.

## 3. Initialize the real provider first

Use the installed AGIS API, not guessed symbol names. Its fresh provisional session must be exportable and validate before Coastal startup. The provider must initialize both the actual player grid and the container mapped to `world.test.storage`, even when storage is empty.

Do not independently load an old provider save during bootstrap. Continue must restore inventory/ledger/world/mission together through the existing campaign coordinator. Required radio parts must be represented by the two world pickups, not pregranted to either provisional grid. The provisional receipt ledger is empty.

All original [adapter methods](M1_ADAPTER_CONTRACT.md) and the [M1.2 read view](M1_2_UI_WIRING.md) remain necessary. Passing the new read-only provider audit is not permission to leave collection, transfer, consumption or restoration unimplemented.

## 4. Make the one startup call

After the local controller possesses the actual character, its EnhancedPlayerInput exists, the provider's real initialization is complete, and the character has a valid dry-floor spawn, call:

```text
Result = CoastalSessionBootstrapComponent.StartTestRoom(
    ActualCoastalMissionDirector,
    ActualCoastalAGISAdapterInstance,
    ValidatedInitialDryCheckpointTransform)
```

Use a unit-scale world transform for the character capsule, not a mesh-relative transform or an arbitrary zero pin. Startup validates both this checkpoint and the current player location without teleporting them. An early BeginPlay call can legitimately return Blocked because possession, input or vendor readiness is not complete. Use the host's actual readiness flow; do not add an endless per-tick retry.

`Started` opens the native M1.2 campaign-selection screen. It does not automatically create a campaign. `AlreadyStarted` is an identical repeated request and must not trigger another UI initialization. `Blocked` means inspect LastReport, fix the reported setup and retry explicitly. `Busy` is a rejected concurrent/reentrant attempt. `Rejected` means a changed binding request or a stopped component. `RestartRequired` means initialization partly executed and the safe route is exit/relaunch.

`RunPreflight(Director, Provider, Checkpoint, Report)` is an optional read-only check on an unbound bootstrap. The report is Blueprint-readable: bPassed plus issue Code, Subject and Detail. It cannot be used to audit an already-playing campaign. OnStartupNotice publishes the completed initial result; use the direct return for every call, including repeats.

Diagnostics also appear in the Output Log and development on-screen messages. They are not a newly implemented shipping error menu. On a UI-construction failure stop PIE or close the development executable; do not try to clear the coordinator's poison flag.

## 5. Keep the Hyper bridge

The installed Hyper interface supplies the selected CoastalWorldObject subclass and its actual pressed-once Interact event. That event still calls `InteractionBridge.TryInteract(SelectedActor)`. Preserve WorldId, Kind, PickupItem and JournalEntry when replacing native proxies with project-owned interface-enabled subclasses.

There is no second focus scanner or guessed Hyper input action. Remove competing vendor-demo menu handlers so only the Coastal UI owns its menu input/context. Required target distance and visibility checks remain in the original bridge.

## 6. Read-only editor room audit

After rebuilding the plugin, open the existing test map yourself and stop PIE. Enable the same editor Python support used by the original generator. In the Unreal Python console, run this with the actual extracted package path:

```python
import runpy
coastal_audit = runpy.run_path(r"D:\YourExtractedPackage\tools\unreal\audit_systems_test.py")
report = coastal_audit["audit"]()
```

To also create a new report, use an existing evidence directory and a filename that does not exist:

```python
report = coastal_audit["audit"](r"D:\YourEvidence\m1_3_room_01.json")
```

The script neither loads another map nor saves the current one. Dirty packages are recorded; `disk_map_verified` remains false. The report checks only currently loaded persistent actors. GameMode assignments, Hyper interfaces, collision quality, actual player route, storage operations and package behavior remain separate runtime checks. No report was generated from Unreal here.

The original map generator is retained and still refuses to overwrite an existing map. The new audit is for inspection of a map you already have; it does not generate an additional room.

## 7. Typical diagnostics

| Code | Correct local response |
|---|---|
| `startup.local_owner` / `startup.pawn` | Use standalone single-player and wait for the real local controller/possessed character. |
| `startup.ui_owner` / `startup.bridge_owner` | Remove duplicate ownership or mixed old startup calls. |
| `startup.enhanced_input` | Check the actual host input classes and local-player subsystem before initializing menus. |
| `room.missing_object` / `room.item_mismatch` | Restore the authored ID/kind/item mapping in the test map; do not bypass the check. |
| `provider.not_ready` / `provider.export_failed` | Finish the real provisional AGIS setup and export implementation. |
| `provider.storage_view` | Initialize/map the real storage grid and implement its actual read projection. |
| `provider.pregranted_part` / `provider.not_pristine` | Remove independent pre-restoration/pregranting from the provisional setup; use coordinated New/Continue. |
| `startup.checkpoint` / `startup.current_position` | Fix the actual capsule/spawn/floor setup; this code does not auto-move the player. |
| `startup.configure_save` / `startup.initialize_ui` | Inspect the logged native error, fix it outside the running session and relaunch. |

## 8. Execute the local acceptance gate

Run all eleven supplied editor automation tests: the existing `Coastal.FirstSignal`, `Coastal.M1`, `Coastal.M1UI` groups and the four new `Coastal.M1Startup` tests. Do not count a standalone C++ run as any of these tests.

Execute [all 26 startup scenarios](../data/m1_3_startup_acceptance.json), [the retained 31 UI scenarios](../data/m1_2_ui_acceptance.json) and [the original campaign acceptance](ACCEPTANCE_TESTS.md). Test controller-only and keyboard/mouse routes, including storage, transcript cancellation/acknowledgement, save failures and damaged-slot recovery with backed-up disposable saves.

Package the actual test room for Windows and launch outside the editor. Complete new game → pickup → storage round-trip → repair → transcript → verified save → exit/relaunch → Continue. Record exact engine/toolchain/vendor versions, source revision/hash, build/runtime logs, actual screenshots and outcomes in a separate evidence copy. Do not edit the shipped source-only report to imply its environment ran those tests.

Only this exercised, coherent Windows test room passes M1. The next production section is M2's coastal route and owned-art assembly; this source increment does not certify that prerequisite or include that map.
