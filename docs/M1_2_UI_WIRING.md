> Retained M1.2 section. M1.3 adds an optional preferred [checked startup path](M1_3_STARTUP_WIRING.md) instead of the three manual initialization calls below. The underlying UI and adapter contracts remain.

# M1.2 — Local Build and Native UI Wiring

## 1. Preserve your project and identify the real dependencies

Back up/commit your host project, saves and any implemented project-owned adapter before replacing the plugin. This is a full plugin update; do not merge it by copying only a few new headers. Keep the supplied original code separate from purchased content. No vendor folder/class path is known here.

Use the actual Third Person C++ host and the engine you select after the compatibility check. Rebuild Development Editor / Win64. The plugin now declares EnhancedInput and uses `UMG`, `Slate`, `SlateCore`, `InputCore`, `EnhancedInput` modules. The platform save interface is supplied by the existing `Engine` dependency (`PlatformFeatures.h`), not a separate module. A precompiled old M1 DLL cannot supply the new reflected classes.

Optional read-only preflight (replace every example path with a real local path):

```powershell
py tools/preflight_host.py `
  --project "D:\YourProject\CoastalExploration.uproject" `
  --engine-root "C:\YourSelectedUnrealEngine" `
  --agis-root "D:\YourProject\Content\YourActualAGISFolder" `
  --hyper-root "D:\YourProject\Content\YourActualHyperFolder" `
  --report "D:\YourEvidence\m1_ui_preflight_01.json"
```

The report directory must already exist and the report file must not exist. The tool does not import, patch, compile, write project files, inspect Blueprint APIs, or prove an asset licence. File presence remains only file presence. A successful preflight never marks Unreal/vendor/gameplay/package checks as run.

## 2. Keep the original M1 room and vendor wiring

Follow `M1_BLUEPRINT_WIRING.md` for the single director, registered world objects and actual AGIS/Hyper integration. Generate a new test map only through the original non-overwrite recipe, if you do not already have one. No `.umap` is included.

The original initialization order still matters. After AGIS's provisional session is exportable and the character has valid dry-floor placement:

```text
Director.Saves.Configure(ActualAGISAdapter, ActualCharacter, "level.systems_test")
ActualCharacter.InteractionBridge.Configure(Director.Saves)
```

Check both return values. Bridge configuration is now explicitly one-time, against a configured coordinator. Do not rebind a running bridge to a different campaign director. A new campaign is not a new director; use the coordinator's Start/Load operations.

Hyper's real interface must route a pressed-once Interact event to `TryInteract(SelectedCoastalWorldObject)`. The native UI does not scan for targets or implement a guessed vendor interface. Keep its source actor WorldId, Kind, PickupItem and JournalEntry mappings unchanged.

## 3. Add the native UI owner

Create/use your game-owned PlayerController subclass and add exactly one `CoastalUISessionComponent`. Set that controller class on your real host GameMode. Do not add this component to the character, director, spectator or a second local controller.

After the original configuration above succeeds, call from the local controller:

```text
CoastalUISessionComponent.InitializeUI(
    Director.Saves,
    ActualCharacter.InteractionBridge,
    ValidatedInitialDryCheckpointTransform)
```

The checkpoint is the same real, unit-scale dry spawn transform intended for StartNewCampaign, not a mesh-relative transform or an arbitrary zero-vector pin. The coordinator validates it again when a new campaign starts.

`InitializeUI == true` means the UI owner/input/widgets attached, not that the purchased integration passed QA. The source can deliberately attach an unavailable-provider diagnostic screen; do not treat that as a functioning inventory. Normal integration should initialize AGIS before opening the paused session UI.

The component creates its HUD and panels, subscribes to the bridge's storage/transcript/action delegates, subscribes to save notices, and registers its three menu actions. Remove competing demo/native/Blueprint M1 menu handlers and duplicate storage/transcript widgets. Do not also acknowledge a radio token from another listener. A vendor-provided slot UI may be integrated later, but not as a second independent menu authority.

## 4. Implement the added AGIS read contract

Override `ReadContainerView(ContainerId, View)` on the actual adapter. These are original project DTOs, not assumed AGIS types. Translate the real installed API into them without mutating inventory.

| Field | Required meaning |
|---|---|
| `ContainerId` | Exactly the requested logical ID, either `container.player` or the mapped open storage actor's stable ID. |
| `Grid` | Actual columns/rows; positive and no greater than 64 per axis. |
| `Revision` | Shared, nonnegative revision for one coherent live inventory state. It changes on every pickup, consume, transfer, restore or other inventory mutation. Both views read synchronously from that state have the same revision. |
| `Items[].InstanceId` | Real valid persistent item-instance GUID. Never a new GUID generated per read. |
| `Items[].ItemId` | Original logical item definition mapped to the installed AGIS definition. |
| `DisplayName` / `Description` | Readable plain text; name required, at most 160 characters; description at most 4096. |
| `Quantity` | Actual positive quantity in that instance. |
| `Position` | Zero-based occupied origin within that container. |
| `Size` | Actual occupied footprint after rotation. |

Rows must be deterministic where feasible; the UI preserves selected instance identity across successful refreshes. Rows cannot overlap or appear in both containers. An empty valid container is `Ready` with an empty row array and real grid/revision; an unconfigured adapter is `NotConfigured`, not a fabricated empty container. Clear output on failure so stale rows are not presented as fresh data.

This event does not serialize the provider, grant an item, decrement a count, move an instance or own a second ledger. All original M1 atomic mutation/export/restore/validation methods remain mandatory. Revision is ephemeral UI invalidation metadata, not another durable campaign generation. Session epochs invalidate screens across loads/new games.

A real adapter must pass tests where a changed/reordered grid is reread, a transfer response is retried with the same operation ID, and both containers remain coherent after a save/load. Source projection validation cannot establish real vendor atomicity or critical-item conservation.

## 5. Development controls

| Context | Keyboard/mouse | Gamepad |
|---|---|---|
| Pause/session | Escape | Menu/Start |
| Inventory from world | Tab | Face top / Y |
| Journal from world | J | View/Select |
| Focus commands in a panel | Up/Down or Tab/Shift+Tab | D-pad Up/Down |
| Activate focused command | Enter/Space or click | Face bottom / A |
| Close top panel/cancel | Escape | Face right / B, or Menu |
| Scroll panel body | Wheel or Page Up/Down | Left/right shoulder |
| Select another inventory item | Previous/Next item commands | Same commands through focus navigation |
| Switch storage side | Switch backpack/storage command | Same command |

Tab navigates inside a menu; it does not simultaneously close inventory and reopen it. Keyboard editing of the optional save-name field uses normal text input. Gamepad testing should use fresh-name generation and discovered-save cycling, so no keyboard is required for the normal session loop.

The shell is keyboard/D-pad operable; final remapping, controller glyph switching, stick navigation, production settings, text scaling and a polished grid presentation have not been completed. Verify the actual host uses EnhancedPlayerInput/EnhancedInputComponent and permits single-player pause. Remove conflicting host menu mappings or route them to this component; do not call `ClearAllMappings`.

## 6. Save and recovery expectations

Start New closes into the world after its successful in-memory session transition; the HUD still shows the initial-save-pending notice until a verified save is reported. Continue closes menus only on a successful loaded/recovered session. Save failures remain visible and keep the current campaign; Continue failure never chains to Start New.

Open pause, save, read journal, return to the same focused command, then close the last panel. Movement/look must resume without releasing unrelated locks. Test holding the open/close/confirm key across transitions. Confirm should not jump, interact or acknowledge newly opened text.

Test a disposable damaged-slot pair only after backups. The catalogue may display the name of a damaged campaign because names are not certification. Load must still validate and recover/refuse through the original coordinator.

On `RecoveryRequired`, the UI is non-dismissible and offers no Save. Restart from an actual validated disk generation. Native UI code does not repair vendor corruption or reset a poisoned coordinator.

## 7. Test and package order

Run editor automation groups `Coastal.FirstSignal`, `Coastal.M1` and `Coastal.M1UI`. There are seven supplied automation tests total. None was run in this delivery environment. Three new tests cover provider-view failure, reflected view validation and the shared UI flow rules; they do not simulate a controller or render UMG.

Run every scenario in `data/m1_2_ui_acceptance.json`, then the original M1 save/repair acceptance ledger. Use controller-only and keyboard/mouse routes. Verify native widget construction, scrolling at 1080p and a smaller viewport, visibility of failure messages, and teardown/re-entry without duplicate input contexts or leaked locks.

Package the test map for Windows and launch outside the editor. Record exact engine/toolchain and vendor versions, source hash, startup/build logs, screenshots from the actual game, and acceptance outcomes. Only then call M1 complete and start M2 coastal art/route assembly.
