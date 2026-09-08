# M1 — Local Blueprint and test-room wiring

> M1.2 update: the native UI in `M1_2_UI_WIRING.md` replaces the manual development-widget work in sections 3, 5 and 6 below. Keep the host/vendor/world initialization guidance. Do not run both widget implementations at once. The original text is retained to explain the underlying contracts.

This guide uses only original plugin symbols plus roles to resolve in your real AGIS/Hyper packages. It is not a claim that Blueprint assets are included.

## 1. Host and map

Create/open `CoastalExploration` as a standard Third Person C++ Unreal project. Do not use a combat variant. Build the untouched template, install/compile the supplied plugin, then run the four engine automation tests.

Enable Python Editor Script Plugin and Editor Scripting Utilities. Save or discard current editor changes yourself. Use Unreal's Python execution interface to run the included `tools/unreal/build_systems_test.py` file. A typical Python console invocation is:

```text
py "D:/YourExtractedPackage/tools/unreal/build_systems_test.py"
```

That is an example path to replace, not a known path on your PC. The script creates a new `/Game/Coastal/Maps/L_SystemsTest`. It refuses to overwrite one. If an earlier generation exists, inspect it and choose a different destination in `data/test_room.json` rather than deleting it blindly.

In World Settings, select the real Third Person GameMode from your host template. Ensure the expected third-person character spawns at the included PlayerStart. Keep the generated map in the Windows cook/package map list during M1 tests.

## 2. Components and one-time initialization

The map already contains native `CoastalMissionDirector` with FirstSignal and CampaignSaves components. Do not place a second generic mission director from the older M0 instructions.

Add your implemented `BP_CoastalAGISAdapter` and a `CoastalInteractionBridge` component to the player or use an inventory owner with the same world and a direct reference. The bridge itself belongs on the actual character.

After AGIS finishes synchronous local initialization, get the sole director once and cache it. Call:

```text
Director.Saves.Configure(ActualAGISAdapter, ActualPlayerCharacter, "level.systems_test")
Player.InteractionBridge.Configure(Director.Saves)
```

Both results must be checked. Do not begin a campaign when configuration fails. The default adapter intentionally cannot pass this step. Bind `OnSaveNotice` to a visible development status label and retain its detail string in your test log.

The character must be on a valid dry floor for initial configuration/capture. Do not configure from a flying spectator pawn or while actors are still being streamed/spawned. All seven persistent objects must already exist. The M1 map is intentionally not streamed.

## 3. Start, save and continue controls

Create a minimal development panel with a save-set field defaulting to `m1_test_01`, Start New, Save and Continue. Only expose these M1 controls; no travel, fishing or combat buttons.

Start New calls `StartNewCampaign(SaveSet, ValidatedPlayerStartTransform)`. Use a unit-scale character transform at the dry spawn, not an unscaled mesh transform. An existing save set is rejected. A new test can use `m1_test_02`; do not manually delete saves as the normal new-game flow.

Show `StartedNew` as “campaign started; initial save pending.” Show success only after `OnSaveNotice` reports `Saved`. Save calls `RequestSave`, or `SaveNow` when not inside another mutation. Continue calls `LoadCampaign` with the same explicit save-set name. A failed Continue must remain an error; do not chain it to Start New.

On successful load/new session close stale storage/transcript widgets, call bridge `CloseStorage`/`CancelTranscript`, and release their UI blocker tokens. Refresh objective and journal. On `RecoveryRequired` show the error and provide exit/relaunch guidance; saves/interactions stay blocked.

This development panel is local implementation work. A polished title/save-selection flow is not included in source.

## 4. Hyper interaction mapping

Inspect Hyper's installed interaction interface/event. Implement it on a game-owned wrapper/subclass of `CoastalWorldObject`; route the selected actor into `TryInteract`. Do not bind both the vendor demo action and your action to mutate the same object.

The generator initially places native actors without the vendor interface. Replace those instances with your project-owned Hyper-enabled subclass and preserve every WorldId, Kind, PickupItem and JournalEntry value. Alternatively, add `world_object_class` to `data/test_room.json` using the actual generated class path of that subclass, then generate a fresh map destination. The source makes no assumption about a local Blueprint path. Recheck IDs before coordinator configuration.

Use the existing project Interact action's **Started/pressed-once** event. Do not call `TryInteract` continuously every frame while the button is held. Hyper remains responsible for focus selection and prompt display. The bridge's own range and line-of-sight validation must remain enabled.

E and gamepad face-left are the foundation's proposed interaction defaults. Respect separate UI/exploration ownership. Attach a unique blocker token when each inventory, journal, pause or transcript widget opens, and release that exact token when it closes. Actual mapping-context switching and focus restoration remain widget/controller work.

`OnActionNotice` drives visible feedback for success, AlreadyApplied, Busy, BlockedByUI, TooFar, Occluded, invalid target, missing configuration, missing items, NoSpace and failure. Never display success for NotConfigured.

## 5. Storage widget

Bind `OnStorageRequested(StorageWorldId)`. Display the real AGIS player/backpack and mapped storage. The callback itself only opens the widget; it does not execute a transfer while the outer interaction is still running.

When the player chooses a transfer, create a new operation GUID, copy the actual item-instance GUID, set a positive quantity and the source/destination logical container IDs, then call `TransferWithOpenStorage`. Retrying the same pending intention reuses its original operation GUID. For a new intended transfer generate another one.

After the returned result, refresh inventory views and the objective using `FirstSignal.GetObjective(ActualAdapter)`. Storing a required part must change readiness back to FindEquipment. `NoSpace` must leave the source item where it was and show an understandable message.

Close invalidates the storage context and releases only this widget's UI token. Gamepad support requires slot selection, inspect, transfer and Back without a mouse. Full focus-stack behavior remains in the UI implementation.

## 6. Radio transcript and journal

Bind `OnTranscriptRequested(Transcript, AcknowledgementToken)`. Create a readable widget, set its text from the supplied transcript and hold the token. No audio duration is required.

Only the player's explicit Continue action after text is available calls `AcknowledgeTranscript(Token)`. A Cancel action calls `CancelTranscript`; cancellation does not complete the mission. Do not call acknowledgement from the original broadcast callback—its mutation scope is still active, and it will return Busy.

Journal widgets read `Saves.GetJournal()` and `CoastalStoryLibrary.JournalText(EntryId)`. The objective uses `CoastalStoryLibrary.ObjectiveText(FirstSignal.GetObjective(Adapter))`. Note and postcard world interactions add their entries idempotently. In M1 show a brief “entry added” notice or open the journal; reading is not an item grant.

For the maintenance note, use the bridge action. Calling the old component's `ReadMaintenanceNote` directly without the journal/world update creates an inconsistent campaign and saving correctly rejects it. Cabin/dock discovery facts can still use their original component methods, with save requests after the triggering action completes.

## 7. Manual sequence before packaging

Start a new set and confirm a verified save. Inspect radio; attempt repair with no parts; collect battery; save/relaunch/continue; transfer battery into storage; collect fuse; verify objective is not ready until battery returns to backpack; repair; save before transcript; relaunch; acknowledge transcript; save; relaunch and confirm completion plus journal.

Repeat with equipment collected before inspection and with note/postcard skipped. Test a full backpack and full storage. Disconnect provider and verify that configuration/Continue fail visibly rather than creating a mock inventory.

For a damaged-slot test, close the game and back up the entire disposable test save set. Use `inspect_save_envelope.py --corrupt-copy` to create a separate corrupted fixture. Only replace the chosen slot in that disposable copy; never corrupt your only save. A valid outer envelope alone does not prove the inner game state is valid.

Run the full acceptance ledger, package the room for Windows, and launch outside the editor. Record real logs and screenshots only. Do not use generated screenshots or a passing helper-test count as gameplay evidence.
