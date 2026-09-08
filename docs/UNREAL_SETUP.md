# Local Unreal setup and source installation

> This file retains the original M0 setup guidance. For M1, use `M1_BLUEPRINT_WIRING.md`: the native `CoastalMissionDirector` and editor recipe replace the generic director/manual proxy-room steps below. Saving source now exists, but the adapter and UI must still be implemented locally.

## What this procedure produces

A local Unreal Third Person project that can compile the supplied objective plugin. It does **not** automatically create the map, download owned assets, connect AGIS, or implement saving. Those steps follow in the integration plan.

## Prerequisites and engine selection

Use a disposable compatibility project first. Test UE 5.7 as an initial candidate because the checked Hyper Interaction listing names that version; do not treat that single listing as certification of the complete asset combination. Select another common supported version if the actual installed packages require it. Record the exact engine build and package revisions before production work.

Install the C++ toolchain supported by that selected engine, including the Windows SDK and game-development C++ workload. Epic's current compatibility table lists Visual Studio 2022 17.8+ for UE 5.7, recommending 17.14. Recheck the table for the engine actually selected; a different engine can require a different toolchain. [U1]

## Create the host project

Create a Games → Third Person project, select the standard/None variant, choose C++, and name the project `CoastalExploration`. Target desktop. Choose conservative initial rendering settings rather than importing a high-cost environment preset. The standard template supplies the development character and movement foundation. [U2]

Build and run the untouched template first. Record that baseline. When integrating into an existing project, back up or commit it before adding the plugin; do not replace its entire configuration with unrelated demo settings.

## Install the source plugin

Close Unreal Editor. Copy only the package's `Plugins/CoastalFoundation` folder into:

```text
<YourProject>/Plugins/CoastalFoundation/
```

The descriptor should end up at:

```text
<YourProject>/Plugins/CoastalFoundation/CoastalFoundation.uplugin
```

Do not put it inside `Content`, and do not copy the entire starter ZIP into `Plugins`. Unreal project plugins are discovered from the project's Plugins directory. [U3]

Regenerate project files as needed, open the generated solution, and build the project's **Development Editor / Win64** target using the selected engine's toolchain. Open the project and enable Coastal Foundation in the Plugins panel if it is not enabled. Restart if prompted.

The descriptor intentionally omits `EngineVersion`: this avoids claiming a tested version. It does not make the code universally compatible. The first local build must exercise Unreal Header Tool and the engine compiler. Record and fix any version-specific errors before integrating vendor content.

## Run the included engine tests

The plugin includes two engine automation tests under `Coastal.FirstSignal`:

- `DefaultAdapterFailsClosed`
- `SnapshotValidation`

Use the Automation/Test Automation UI available in the selected editor build, enable the relevant editor testing support, and run these tests. Epic documents its automation framework and editor entry point. [U4]

These tests were not executed in the source-delivery environment. Passing them locally still does not validate the actual AGIS adapter.

## Create the test map

Create a small test map named `L_SystemsTest`. Use explicit development proxies for now. Include the template player start, a door, two item pickups, a storage chest, a radio, and a generic Actor Blueprint named `BP_FirstSignalDirector`.

Add one `FirstSignalComponent` to `BP_FirstSignalDirector`. Place exactly one director in the map. Store a direct, validated reference to it in the game-owned interaction bridge; do not search the entire world on every frame.

Create a Blueprint subclass of `CoastalInventoryAdapter` named `BP_CoastalAGISAdapter` and add it to the player or another well-defined local inventory owner. The base component is deliberately nonfunctional and returns `NotConfigured`. Its presence is not proof of an inventory integration.

Override `CheckRequirements` and `TryCommitRequirements` only after inspecting AGIS's actual installed API. Map `item.radio_battery` and `item.marine_fuse` to real AGIS item definitions. Never replace `NotConfigured` with unconditional success just to make the quest advance.

Use the contract in `INTEGRATION_CONTRACTS.md` for transaction behavior. Complete real pickup, transfer, repair, and persistence before building the coast.

## Connect mission events

| Actual in-game event | Source component call |
|---|---|
| Player enters the cabin discovery volume | `MarkCabinVisited()` |
| Validated player interaction inspects the radio | `InspectRadio()` |
| Player enters the old dock discovery volume | `DiscoverDock()` |
| Player reads the optional maintenance note | `ReadMaintenanceNote()` |
| Validated repair interaction | `RequestRadioRepair(actual adapter)` |
| Transcript is presented and the player finishes/acknowledges it | `FinishRadioTransmission()` |
| HUD/journal refresh | `GetObjective(actual adapter)` and `ExportSnapshot()` |
| Save capture | Include `ExportSnapshot()` in the full campaign save object. |
| Campaign load | Restore inventory/world coherently, then apply `RestoreSnapshot()`. |

These are names of the original plugin's methods, not guessed Hyper or AGIS methods.

`OnStateChanged` is a presentation/update notification. A save coordinator must not capture half of an inventory transaction from inside an arbitrary event callback. Queue a save after the guarded operation returns.

## Import the selected environments

Open each vendor project separately, confirm the demo works, and identify the exact selected assets and dependencies. Use Unreal's migration workflow into the destination project's Content folder. [U5]

Keep vendor folder structures intact during the first compatibility pass. Create game-owned Blueprints and data under `/Game/Coastal/`. Do not move purchased content into an imagined shared folder before checking references. Review project settings requested by a pack individually; do not overwrite the host game's input, game mode, or rendering configuration wholesale.

Inspect scale, collisions, material dependencies, openable doors, and the third-person camera at player height. Track unresolved visual asset paths in `data/asset_register.json`; null paths mean unresolved, not permission to invent a path.

## JSON is specification data

The provided `data/*.json` files are human- and tool-readable design records. The plugin does not load them. There is no automated Unreal DataTable or asset importer in this starter. Create the corresponding Unreal item definitions, input assets, and content references deliberately; then keep IDs synchronized with the provided validation tool.

## Source-control boundary

Keep the generated project and original source in a suitable working repository. The starter does not grant rights to redistribute purchased source assets. Record actual acquisition/license terms and approved collaborator access before adding vendor files to a repository. Do not assume a private link alone resolves licensing. Do not put credentials or cookies into manifests.

## Official references

- [U1 — Visual Studio setup/version compatibility](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine)
- [U2 — Third Person template](https://dev.epicgames.com/documentation/en-us/unreal-engine/third-person-template-in-unreal-engine)
- [U3 — Plugins](https://dev.epicgames.com/documentation/en-us/unreal-engine/plugins-in-unreal-engine)
- [U4 — Automation framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine)
- [U5 — Migrating assets](https://dev.epicgames.com/documentation/en-us/unreal-engine/migrating-assets-in-unreal-engine)
