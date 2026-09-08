> Retained M1.2 section. M1.3 adds an optional preferred [checked startup path](M1_3_STARTUP_WIRING.md) instead of the three manual initialization calls below. The underlying UI and adapter contracts remain.

# M1.2 — Native Interface and Session/Input Ownership

**Project:** Coastal Exploration / First Signal  
**Section date:** September 5, 2026, America/New_York  
**Version:** `0.1.2-m1-ui-source`  
**Status:** Source implemented and standalone checks executed; Unreal/vendor/Windows gates unrun.

## 1. Why this is the next piece

The supplied M1 handoff explicitly left development Start/Save/Continue controls, journal and transcript widgets, storage presentation, controller focus, and UI input ownership unfinished. M1's vendor-integrated packaged test room has not been proven. This section implements the game-owned UI layer instead of falsely skipping the M1 gate to coast art, fishing, or an offshore mission.

The existing mission, campaign snapshot schema, save envelope and world-object IDs remain unchanged. There is no automatic migration to a production map. AGIS remains the sole inventory authority and Hyper remains the focus/prompt owner.

## 2. Included implementation

| Source | Responsibility |
|---|---|
| `CoastalUISessionComponent` | Controller-owned lifetime, delegate subscriptions, deferred requests, current-session validation, menu commands and error presentation. |
| `CoastalUIStack.cpp` / `Core/UIFlowRules.h` | Bounded modal stack, unique tickets, top-only actions, epoch checks, frame debouncing, focus restoration, sticky recovery. |
| `CoastalUIInput.cpp` | Three runtime menu actions, local context, UI-only ownership, balanced movement/look locks, modal pause and release suppression. |
| `CoastalPanelWidget` / `CoastalPanelInput.cpp` | Native UMG widget construction, keyboard/gamepad button navigation, scroll commands and paint-before-action gate. |
| `CoastalHUDWidget` | Read-only objective, action/save/error feedback and controls. |
| `CoastalUIPresentation.cpp` | Development session/pause/journal/transcript/inventory/storage/inspection/confirmation/recovery copy and action availability. |
| `CoastalUICommands.cpp` | Screen-specific command whitelist and original game-system calls. |
| `CoastalUISaveCatalogue.cpp` | Save-name discovery through platform save APIs; exact selected-set Continue; fresh-name generation. |
| `CoastalInventoryView` / `Core/InventoryViewRules.h` | Strict validation of ephemeral AGIS display data; canonical save-slot-name parsing. |
| `CoastalUIInventory.cpp` | Live view reads, selected-instance preservation, stale-selection checks and transfer intention retries. |
| `tools/preflight_host.py` | Read-only local host/tool/vendor-file presence checks with optional exclusive-new-file JSON report. |

The widget trees are constructed in C++; these screens do not require missing `.uasset` widgets. The source still must pass Unreal Header Tool, UnrealBuildTool, cook and runtime tests locally. Source construction is not evidence of successful rendering.

## 3. Menu and session behavior

### Session screen

The initial selected namespace is `m1_test_01`. A keyboard/mouse user can edit it; a gamepad user can cycle discovered save-set names or generate a new GUID-based name without text entry. The catalogue derives namespaces from canonical `Coastal_<set>_A` and `_B` names and deduplicates the pair. It is alphabetical, not a last-played or timestamp catalogue.

Names are discovery only. Their presence does not assert valid payloads, campaign completion, compatible vendor data or safe player transforms. Continue still calls `LoadCampaign`, which validates and selects generations. Unavailable name enumeration is distinguished from a successfully enumerated empty list. A typed exact name remains possible where enumeration is unsupported.

New uses the selected name and refuses existing disk saves through the original coordinator. A live session replacement requires confirmation because unsaved in-memory progress may be lost. No save deletion, rename, cloud account, multiple-user profile or automatic save migration was added.

### Pause and exit

All modal screens request a single-player pause when the world was not already paused. The coordinator's retained tick-while-paused behavior can write its queued small M1 save. If the game mode refuses pause, movement/look are still locked; this must be checked in the host and is not a substitute for a working pause configuration.

Save and Exit calls the original synchronous `SaveNow` and exits only on `Saved`. An explicit separate Exit Without Saving option explains potential progress loss. Recovery exposes only exit without writing the potentially inconsistent live campaign.

### Journal and transcript

Journal text comes from `CoastalStoryLibrary` and `Saves.GetJournal()`. Objective readiness reads the actual adapter rather than cached item counts. Story content is unchanged; the North Reach lead still is not a travel destination.

The transcript callback is queued until the interaction's mutation scope has ended. A native paint pass marks the panel as presented. A later explicit non-repeating Continue must also have the top panel's current ticket and epoch. Only then is the bridge acknowledgement called. This establishes presentation opportunity, not proof a human read every word. Audio remains optional. Back cancels the token without advancing the mission.

## 4. Focus and input rules

There is one M1 UI owner on one local PlayerController. The component adds only a private runtime context containing Pause, Inventory and Journal actions, at priority 100. It does not remap movement, interact, jump or sprint, and it does not call `ClearAllMappings`.

When the first modal opens, the component takes one movement lock and one look lock, remembers the previous cursor state, enters UI-only mode and requests pause if necessary. Nested panels do not add more controller locks. Every panel gets its own reserved `coastal.ui.*` bridge blocker. Closing the last panel releases only this owner's lock counts and restores game-only mode. A mapping rebuild suppresses keys held across the transition until release. Other owners' mapping contexts and independent ignore-input lock counts are not reset.

This is not a universal compositor for arbitrary third-party menus. No unrelated system may change input mode, pause ownership or directly create competing M1 widgets while this shell owns the UI. Host/vendor demo menu handlers must be removed or routed to this owner. Production settings/remapping/accessibility and a general focus manager for unrelated menus remain M3 work.

Each panel ticket identifies its session epoch, screen kind and unique ID. Only the top panel can command. An input frame may claim at most one command, preventing one Back event from closing a child and its exposed parent. Repeated key events for Back/Confirm are consumed, not executed. Parent focus index is retained when a child is opened. Closing inspection leaves the underlying item selection intact.

Recovery is sticky for the component lifetime. A stale session UI cannot unpoison it. If recovery-widget creation fails, the source retains input blocking and emits an explicit exit/relaunch diagnostic rather than reporting successful recovery.

## 5. Inventory presentation is not an inventory system

`ReadContainerView(ContainerId, View)` is a new read-only `BlueprintNativeEvent` on the original adapter. The base implementation returns `NotConfigured` and clears the output. No data is inferred from opaque vendor save bytes; no test backpack is granted.

A view provides a container ID, grid dimensions, revision, and item rows with actual instance GUID, definition ID, display text, quantity, effective position and occupied size. The native wrapper rejects missing/duplicate GUIDs, invalid definitions, empty names, invalid quantities, malformed dimensions, out-of-grid or overlapping cells and excessive text/array sizes. Opening storage additionally requires coherent revisions and no shared instance GUID across the two containers.

The UI is intentionally a list view that reports grid positions. It does not claim drag-and-drop grid rearrangement, equipment slots, icons, sorting, item rotation or automatic vendor integration. Item inspection and one-item transfer are usable through buttons instead of requiring a mouse drag. More polished grid UI remains separate work.

Before a new transfer intention, both visible views are reread. The selected instance, definition, quantity, position, footprint and revision must still match. A changed view requires another explicit selection review. The actual bridge still validates storage range/visibility, campaign epoch, endpoints and request data before asking AGIS to mutate.

A new transfer intention gets one operation GUID. Busy/Failed retain that same request for an explicit Retry. No timer or UI refresh retries it automatically. Confirmed/terminal results release the intention. Cancel stops retries but does not reverse a commit that a provider may already have recorded; the UI explains this distinction. Actual atomicity/idempotence remains the real adapter's responsibility.

## 6. Save and lifecycle integration

Callbacks from save/new/load can occur while the coordinator's IO guard is active. The UI only records notices/flags in those callbacks. On a later component tick after the guard releases, a successful new/load session closes stale screens, invalidates storage/transcript/intention state, updates the epoch and restores gameplay input.

A successful new-game notification still means initial save pending, not a disk write. Only the existing `Saved` notice reports a verified generation. A failed load stays on an error path. A recovery result enters the non-dismissible recovery screen; no new campaign is substituted.

On component teardown, the shell removes its delegates, widgets, blockers, input component and mapping context, releases only its own pause/input ownership, and retains no inventory data as authority. Pawn replacement is not silently rebound; it requires a fresh correctly wired session UI or relaunch. This is an intentional boundary for the fixed M1 test-room character.

## 7. What was checked here

The same new core headers used by runtime source are exercised in standalone C++ tests. Those cover ticket/session rejection, one-command-per-frame, parent focus, bounded stack, recovery persistence, paint readiness, selection wrapping, inventory-view validation and save-name parsing. Existing mission/campaign/envelope tests were retained and rerun.

Python tests cover local preflight failure reporting and report non-overwrite safeguards in temporary filesystem fixtures, alongside the original envelope-inspector tests. These fixtures do not constitute a real host project or engine installation. Structural checks inspect source/data/doc consistency, dependencies and fail-closed defaults; they do not compile reflection or run the UI.

Actual counts, compiler results and sanitizer results are in [the validation report](../VALIDATION_REPORT.md). Unreal/editor tests are provided but unexecuted. No rendered screenshot or game capture has been fabricated.

## 8. Remaining local blockers

The first local task is still to compile the host/plugin and implement the real AGIS methods, now including the view projection, and connect the real Hyper interface. Follow [M1.2 UI wiring](M1_2_UI_WIRING.md). Then execute the full pickup → storage → repair → transcript → verified save → relaunch → Continue loop with the native UI, including controller-only and fault cases.

Only after the Windows test-room gate passes should M2 assemble Cabin Cove, Shoreline Trail, Old Dock and Rail Overlook from verified owned assets. Production menus/settings, final character/environment art, footsteps, water recovery and performance validation have not been silently counted as done.

## References

Original design and missing-work basis: supplied M1 implementation, wiring and validation, retained in this package. New architecture/code above is original project work. Official documentation was consulted to check entry-point signatures and behavior, not to certify the plugin:

- [Epic UUserWidget API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/UUserWidget) — native widget and paint/input hooks.
- [Epic OnPreviewKeyDown](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/UUserWidget/OnPreviewKeyDown?lang=en-US) — handled preview input prevents child processing.
- [Epic Enhanced Input](https://dev.epicgames.com/documentation/unreal-engine/enhanced-input-in-unreal-engine) — runtime mapping contexts.
- [Epic FModifyContextOptions](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/EnhancedInput/FModifyContextOptions) — held-key suppression on rebuild.
- [Epic AController API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AController) — stacked movement/look input locks.
- [Epic ISaveGameSystem API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/ISaveGameSystem) — platform-dependent save-name enumeration.

The exact engine remains a local compatibility choice. Documentation availability for a version does not establish that the user's assets or this source build on it.
