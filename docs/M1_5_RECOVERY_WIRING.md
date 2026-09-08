# M1.5 — Local Safe-Return Wiring

## Install the full plugin update

Back up/commit the actual host project, current saves and your implemented AGIS adapter. Replace only the package's complete `Plugins/CoastalFoundation` directory. Keep vendor content and host configuration. Rebuild the actual Development Editor / Win64 target against the engine selected by your compatibility test. Old M1.4 binaries cannot provide these new reflected classes.

No `.uproject`, `.umap`, paid assets, compiled DLL or Windows executable is included. Follow the existing [startup guide](M1_3_STARTUP_WIRING.md) and [interaction guide](M1_4_INTERACTION_WIRING.md) for real provider readiness, native UI ownership and actual Hyper focus. Those are still prerequisites.

## Add one optional character component

Add exactly one `CoastalPlayerRecoveryComponent` to the **actual possessed Third Person character**, beside its interaction bridge. Do not put it on the director or PlayerController. The existing bootstrap and native UI remain on the PlayerController, together with an optional interaction relay.

Configure `bEnableFallBoundary`, `FallBoundaryZ` and `bFadeCamera` before initialization. Defaults are a -1500 cm fall plane and camera fade enabled. Keep the boundary generously above the actual world's KillZ; the native minimum margin is 500 cm, not a measured fall-speed guarantee. Standard gravity/upright capsules and the fixed M1 pawn are assumed. No swim, ragdoll, custom gravity or automatic respawn integration is supplied.

A host retaining no recovery component and no safety volumes remains supported, but does not gain this feature. Safety volumes without a recovery owner are a startup error. Their presence alone is not working protection.

## Keep the same bootstrap call

After real AGIS provisional initialization and possession, use the existing method:

```text
Bootstrap.StartTestRoom(ActualDirector, ActualAGISAdapter, ValidatedInitialDryCheckpoint)
```

The bootstrap order is coordinator → bridge → optional interaction relay → optional character recovery → native UI. It passes the validated startup checkpoint to recovery as its initial fallback. Do not also manually initialize recovery when using the bootstrap. A partial binding failure requires relaunch.

For an intentionally manual host, check every return in this order:

```text
Saves.Configure(ActualAGISAdapter, ActualCharacter, "level.systems_test")
Bridge.Configure(Saves)
Relay.InitializeRelay(Bridge)                       // only if a relay is present
Recovery.InitializeRecovery(Saves, Bridge, InitialDryFallback)
UI.InitializeUI(Saves, Bridge, InitialDryFallback)
```

These are original plugin names. None is an AGIS/Hyper vendor symbol. The UI automatically subscribes to a recovery component present when `InitializeUI` runs. Do not add a second return widget or duplicate save/teleport callback.

## Author the safety actors

Place native `CoastalSafetyVolume` actors. Select `DeepWater`, `OutOfBounds` or `DryCheckpoint`. Set the root `Bounds` box extent and position; only upright/yaw rotation and positive scale are supported. Safety queries do not require collision or generated overlap events. Do not convert these into persistent world objects or invent new world IDs for them.

For a dry checkpoint, move its `ReturnPoint` to a real unobstructed capsule-center transform above dry, walkable floor. The volume origin is its default marker position, so a floor-level actor origin is **not** automatically a usable character destination. Check the actual character capsule size, doors, wall clearances and hazard boundaries. Leave at least one valid initial fallback available.

Keep safety actor membership fixed after initialization. Do not spawn/destroy/stream these actors during the M1 session. Keep checkpoint regions distinct; overlapping regions record neither. Place hazards off the required pickup/storage/radio route for baseline tests. They can represent deep water without supplying a water material, physics volume or swimming.

## Optional fresh-room recipe

The existing non-overwriting generator has an opt-in safety variant. In Unreal, after compiling the plugin and enabling the existing editor Python support, with PIE stopped and no unsaved work:

```python
import runpy
room = runpy.run_path(r"D:\YourExtractedPackage\tools\unreal\build_systems_test.py")
room["build"](include_safety=True)
```

It still targets `/Game/Coastal/Maps/L_SystemsTest` and refuses an existing map. It does not modify an existing room, set a GameMode, add the character recovery component, import vendor assets or wire AGIS/Hyper. An error after map creation leaves the partial new map for inspection, as before. The script has not run inside Unreal in this delivery.

The layout lives in `data/m1_5_safety_layout.json`. All positions/extents are centimetres:

| Development volume | Center | Half extent |
|---|---|---|
| Cabin checkpoint | (-1300, 0, 100) | (150, 150, 100) |
| Dock checkpoint | (1000, 0, 100) | (120, 120, 100) |
| Deep-water proxy | (1300, -700, 100) | (120, 160, 100) |
| Boundary proxy | (1480, 0, 100) | (100, 250, 100) |

Thin, non-colliding square markers identify the development regions. They are not final coastline/water artwork. Verify marker heights against the real capsule locally. To retain an existing map, author equivalent native actors in the editor rather than deleting or overwriting it. Use fresh disposable save-set names after changing safety geometry; there is no automatic M1 save migration.

## Keep input and presentation ownership coherent

Recovery's temporary input blocker has priority 90; the existing Coastal menu component is 100. Standard gameplay handlers must be lower priority. Do not add higher-priority Jump/use bindings that bypass recovery, or mutate/teleport the pawn from Blueprint tick while it owns movement. The recovery component does not change input mode, cursor, pause state or mapping contexts; it removes its own input component and balances its own ignore counts.

Test holding Jump and Interact throughout the return, including pressing a second mapped device. A Boolean action held through release should require release before executing. In VendorEvents mode continue forwarding real action-down/released state, and refresh actual Hyper focus after the return. Never synthesize fake release samples.

Do not start another camera fade while recovery owns its fade. An already active external fade is left alone; disable `bFadeCamera` when another deliberately integrated presentation owner supplies recovery fading. No audio timing is required for progress.

The native HUD reports returning, checkpoint recorded, fallback use and return completion. Save queued is not saved. A later write failure must remain visible. A terminal failure clears the owned fade and opens the existing recovery-required UI; do not reset the poison flag or save that live state.

## Local build and acceptance

Run the nineteen supplied Unreal automation tests across `Coastal.FirstSignal`, `Coastal.M1`, `Coastal.M1UI`, `Coastal.M1Startup`, `Coastal.M1Interaction` and `Coastal.M1Recovery`. Four recovery tests are native helper/fail-closed tests; passing them still does not establish real collision/input or AGIS behavior.

Run all 32 entries in [the recovery ledger](../data/m1_5_recovery_acceptance.json), plus the original mission/save, UI, startup and interaction ledgers. Use actual keyboard/mouse and controller-only runs. In particular verify carried/storage item instances before and after return, post-repair return, primary/fallback collision, changed destination after teleport, grounding timeout, held Jump/Interact, external pause, write failure and relaunch/Continue. Fault tests require disposable backed-up saves.

Package and launch the actual Windows test room outside the editor. Record the selected engine/toolchain, installed vendor versions, source checksum, build/runtime logs and real screenshots in a separate local evidence copy. The shipped source-only report remains unchanged historical evidence of this delivery environment.

**The next production gate is still the real M1 packaged campaign loop. M2 coast assembly is not included or marked complete.**
