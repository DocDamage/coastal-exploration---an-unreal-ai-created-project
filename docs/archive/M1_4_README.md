# Coastal Exploration — M1.4 Interaction Routing & Pressed-Once Protection

**Third-person • Single-player • Unreal • Windows first • First Signal**  
**Plugin:** `0.1.4-m1-interaction-source`  
**Delivery:** Complete updated source package, based on the supplied M1.3 ZIP.

This section implements the game-owned connection between Hyper's selected actor, the player's Interact input, and the existing mission/save bridge. It adds a controller-owned relay, an optional native E / gamepad face-left action, read-only prompt data, stale-focus rejection and a shared same-frame/reentrant action guard. It does not add another focus scanner, inventory, map or travel system.

**No Unreal-compiled build or Windows executable is included.** The real host `.uproject`, AGIS/Hyper packages and Unreal toolchain were not found in the inspected supplied environment. M1's playable gate remains unproven. New rules were tested outside Unreal; native input, reflection, collision and vendor behavior require local verification.

## What the update does

`UCoastalInteractionRelayComponent` goes on the local PlayerController alongside the existing UI and bootstrap. Hyper still decides which actor is focused and renders the prompt. The relay accepts only a current, registered world object and forwards an intentional press to the existing bridge, which rechecks reach, visibility, UI state and campaign state before mutation.

There are exactly two selectable input routes. `VendorEvents` is the default and creates no mapping; the host forwards the real aggregate interaction-down state. `NativeEnhancedInput` installs only an E / face-left action. Pick one route and remove any duplicate direct world-interaction handler. Neither route remaps movement, jumping or menus.

Repeated input cannot toggle a door twice or advance inspection and repair within one frame. Holding Interact does not repeat through the relay. Focus expires without fresh vendor updates, and menu/new/load transitions require fresh focus and a released input before another press. No rejected action is queued for automatic retry.

## Start here

| File | Purpose |
|---|---|
| [M1.4 wiring](../../docs/M1_4_INTERACTION_WIRING.md) | Exact original plugin calls and integration sequence; no guessed vendor API. |
| [M1.4 implementation](../../docs/M1_4_IMPLEMENTATION.md) | Ownership, input/focus lifecycle, fixes and boundaries. |
| [Current validation](M1_4_VALIDATION_REPORT.md) | Actual checks, evidence and unrun engine/vendor gates. |
| [Interaction acceptance](../../data/m1_4_interaction_acceptance.json) | Local test cases; all shipped as not run. |
| [M1.3 startup wiring](../../docs/M1_3_STARTUP_WIRING.md) | Retained provider/room/bootstrap prerequisites. |

## Install and initialize

Back up/commit the host project and your real adapter. Replace only the complete `Plugins/CoastalFoundation` directory, then rebuild the actual Development Editor / Win64 target. Do not put the entire archive into Plugins. Do not overwrite purchased content or host settings.

Add one relay to the local controller and select its input owner. The existing `Bootstrap.StartTestRoom(ActualDirector, ActualAGISAdapter, ValidatedDryCheckpoint)` call now initializes a present relay after bridge configuration and before the native UI. Do not also initialize it manually. A previously integrated M1.3 host without a relay remains supported, but does not gain the relay's held-input and focus-lease guarantees.

Wire actual Hyper selection to `UpdateFocusedTarget` every local focus evaluation/frame, and `OnOfferChanged` to Hyper's real prompt presenter. In `VendorEvents`, additionally forward actual input state to `SubmitExternalInput`; see the wiring guide for the required released sample. A successful bootstrap still is not a successful vendor integration or a saved campaign.

## Standalone checks

```sh
bash tools/run_core_tests.sh
CXX=clang++ bash tools/run_core_tests.sh
CXX=clang++ bash tools/run_sanitizers.sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 tools/verify_package.py
```

The package includes fifteen Unreal automation tests across the retained groups and new `Coastal.M1Interaction` group; none ran here. Complete the real pickup → storage → repair → transcript → save → exit/relaunch → Continue loop in a Windows package before marking M1 complete or beginning M2 coast assembly.
