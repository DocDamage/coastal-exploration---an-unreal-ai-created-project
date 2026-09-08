> Historical M1.2 README. The current section is M1.3; see [current README](../../README.md).

# Coastal Exploration — M1.2 Native Test-Room Interface

**Third-person. Single-player. Unreal. Windows first. First Signal.**

This is a **complete updated source package**, not a patch-only ZIP. It contains the retained M1 mission/persistence systems plus native UMG menus, menu input, controller navigation, an AGIS read-only inventory-view contract, and regression tests. The plugin version is `0.1.2-m1-ui-source`.

**No Unreal build or playable Windows game is included.** The host `.uproject`, purchased assets, real AGIS adapter implementation and Hyper interface bindings are not available here. M1's acceptance gate remains **NOT RUN**, not passed. Native widget source now exists; no WidgetBlueprint binaries are required for these development screens.

## What is implemented in source

The session screen starts a new named campaign, selects existing save-set names, continues through the original validated loader, generates a fresh name, and confirms replacement of a live session. Pause includes save, journal, inventory, campaign selection and guarded exit. Save and Exit only exits after a `Saved` result; a failed Continue never starts a new campaign.

The journal uses existing story text and mission state. The radio transcript requires a rendered panel and a later explicit Continue; Cancel does not complete it. Inventory/storage use a validated read-only projection from AGIS, with item selection, inspection, one-item transfer, explicit same-ID retry and no mock contents. This is a development list view, not a replacement inventory or finished grid presentation.

One native UI component owns a stack of menus. Top-screen/epoch tickets reject stale callbacks, a per-frame command guard prevents double Back, and parent selection/focus survives child screens. The component balances its own input locks, uses UI-only input while modal, adds/removes only its own Enhanced Input context, and does not clear another system's mappings.

## Begin with these files

| File | Purpose |
|---|---|
| [M1.2 implementation handoff](../M1_2_IMPLEMENTATION.md) | Changes, architecture, boundaries, and remaining work. |
| [Local UI integration](../M1_2_UI_WIRING.md) | Exact original plugin calls, AGIS view contract, local build/test order. |
| [Current validation](M1_2_VALIDATION_REPORT.md) | Executed checks and explicit unrun gates. |
| [UI acceptance ledger](../../data/m1_2_ui_acceptance.json) | In-engine scenarios to execute; all initially `not_run`. |
| [Original M1 wiring](../M1_BLUEPRINT_WIRING.md) | Host, vendor adapters, world actors, save/repair integration. |

## Install and connect locally

Back up your project and custom adapters. Replace **only** the old `Plugins/CoastalFoundation` folder with this package's complete folder. Do not copy the whole archive into `Plugins`. Keep paid assets and your own project content separate. Rebuild the actual host's Development Editor / Win64 target before launching the editor.

After the real adapter is ready and the original director/bridge configuration succeeds, add one `CoastalUISessionComponent` to your local PlayerController and call `InitializeUI(Director.Saves, Character.InteractionBridge, ValidatedInitialCheckpoint)`. It constructs its own widgets and menu input. Keep Hyper's actual focus/Interact wiring; no second focus scanner was added.

The new `ReadContainerView` event must be implemented on the real AGIS adapter for inventory/storage presentation. All other existing adapter requirements still apply. Default methods remain `NotConfigured`.

## Source checks

```sh
bash tools/run_core_tests.sh
CXX=clang++ bash tools/run_core_tests.sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 tools/verify_package.py
```

Windows commands are retained in `tools/run_core_tests.ps1`; that runner was not executed here. The optional `tools/preflight_host.py` checks explicit local paths without modifying the project. It does not build Unreal, inspect vendor APIs, or certify a playable game.

See [validation](M1_2_VALIDATION_REPORT.md) for actual current results. Earlier evidence is retained under `evidence/archive/m1/`; it is not mislabeled as a new engine test. No repository was modified.
