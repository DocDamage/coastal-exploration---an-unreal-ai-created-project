# M1.4 — World Interaction Routing and Pressed-Once Protection

**Project:** Coastal Exploration / First Signal  
**Section date:** September 5, 2026, America/New_York  
**Plugin version:** `0.1.4-m1-interaction-source`  
**Status:** Original source implemented; standalone checks executed; Unreal/vendor/Windows acceptance not run.

## 1. Why this section

The supplied M1.3 source still required the host to connect Hyper's actual selected actor and pressed-once input to the game bridge. It included no real vendor binding and no proof that the packaged test-room gate had passed. This increment implements reusable game-owned interaction routing and a concrete native input option without pretending the vendor integration is known.

Source inspection also exposed an actual guard gap: the bridge released its mutation scope before broadcasting its action notice and did not remember a completed action's frame. Two ordinary bindings could call `TryInteract` in one frame, toggling a door twice or inspecting and then repairing a radio. A callback from the completed notice could also reenter another bridge action after the save mutation guard had ended. The new shared dispatch guard covers the entire public action, including its notifications. The regression is exercised in a pure helper test, with a native Unreal test supplied but not executed.

This remains test-room work, not M2 coast assembly. No evidence of real engine/vendor/package acceptance was supplied. The exact engine remains a local compatibility choice.

## 2. Source added or changed

| Source | Responsibility |
|---|---|
| `CoastalInteractionRelayComponent.h/.cpp` | Controller-owned focus forwarding, input intent, session/permission invalidation, changed-only prompt publication, teardown. |
| `CoastalInteractionInput.cpp` | Optional runtime Enhanced Input Interact action and ownership-specific cleanup. |
| `CoastalInteractionOffer.h/.cpp` | Read-only prompt projection for the existing Hyper presentation owner. |
| `Core/InteractionRules.h` | Exact input, focus-lease, dispatch, permission and offer-selection helpers shared with runtime and standalone tests. |
| `CoastalInteractionBridge.h/.cpp` | Whole-call same-frame/reentrant guard, permission revision and explicit world pause/recovery/possessed-player checks. |
| `CoastalStartupPreflight.cpp` / bootstrap | Validate and initialize a present relay without breaking intentionally retained M1.3 hosts that have none. |
| `CoastalUISessionComponent.cpp` | Understand the appended suppression/stale-focus result values without treating suppression as success. |
| `tests/interaction_core_tests.cpp` | Eight-suite standalone coverage now includes original interaction routing/offer rules. |
| `Private/Tests/M1InteractionAutomationTests.cpp` | Four additional Unreal automation tests; provided, not run. |

The campaign schema, world/item IDs, opaque provider payload, receipt fingerprints, save namespace and story text remain unchanged. The two new action-result values are appended rather than inserted before existing enum values. This is source-level compatibility, not proof of loading an existing Unreal binary save.

## 3. One owner per responsibility

Hyper remains the only selected-target/focus system and the prompt renderer. The relay does not sphere trace, scan actors, choose a nearby target, create an alternate outline, render a competing HUD prompt or execute a vendor demo action. The existing bridge retains its character-origin distance/visibility validation; these traces are legal-action checks, not focus discovery.

AGIS remains the only inventory. The unchanged default adapter still returns `NotConfigured`. The relay does not grant items, parse vendor bytes, create a ledger, fake a backpack, or replace any inventory transaction. Actual insertion/transfer/repair atomicity still depends on the real adapter and its original acceptance tests.

The existing native UI owns its menus, input mode and pause. The relay takes no movement/look locks and never clears unrelated input contexts. Movement, camera, sprint and jumping remain the host's systems. There is no controller glyph/remapping/settings implementation in this increment.

## 4. Input ownership and intentional presses

### VendorEvents — default

No mapping context or native input component is created. The actual host/vendor wrapper forwards the aggregate Interact action-down state through `SubmitExternalInput(bool)`. This should be the real logical action state, including all keys/buttons bound to that action, not a fabricated true/false pair per callback.

A true sample may execute once. Further true samples while held do nothing. False is a real released sample that arms a later press. New/load/menu/paused/permission transitions require a released sample after gameplay becomes eligible; no action executes on the enabling frame itself. The host must continue forwarding actual state after a menu or session transition so that a missed release does not leave input permanently latched.

### NativeEnhancedInput — opt-in

The relay creates a Boolean action with E and gamepad face-left bindings, and its own context/input component at priority 20. Started attempts a press. Completed acknowledges release only when neither mapped key remains down. Canceled cancels the intention; it is not treated as proof of physical release. Native tick can observe both keys up, and the existing UI's mapping rebuild suppresses held keys across menu transitions.

The component removes only its own context/input resources. It does not call `ClearAllMappings`, reset another owner's input locks, change input mode or create a new pawn. The real host must still be tested with its actual mapping priorities, UI-only mode, controller and EnhancedPlayerInput. API documentation is not evidence of that runtime behavior.

Choose the mode before initialization. The chosen mode is captured for the lifetime of that initialized relay; changing a property while playing does not hot-swap input ownership. Do not leave a direct Hyper/Blueprint `TryInteract` handler active alongside the relay. The same-frame guard is defense in depth, not a conflict resolver for duplicate handlers on different frames.

### Legacy direct bridge calls

`TryInteract`, `TransferWithOpenStorage` and `AcknowledgeTranscript` now share one guard. A second action in the same frame or a synchronous reentrant action is `SuppressedInput` and does not broadcast another notice. The guard is not reset by a campaign transition.

Direct bridge callers still do not have physical key state. Therefore a legacy caller that invokes `TryInteract` every frame while held does not gain across-frame press/release behavior merely by updating the plugin. Use the relay or retain an independently verified pressed-once host input path. This limitation is explicit.

## 5. Fresh-target contract

The host forwards Hyper's actual selected `ACoastalWorldObject` or project-owned subclass every local focus evaluation/frame using `UpdateFocusedTarget`. A focus-change-only callback is insufficient because the relay deliberately expires old samples. No target is automatically rediscovered.

The focus lease records runtime actor identity, campaign epoch, interaction-permission revision and sample frame. It is valid on the sample frame and the next two engine frames. This is a small ordering tolerance, not a time-based aim-assistance feature. At low frame rates its wall-clock length increases; real low-frame-rate tests remain required. Missing updates hide the prompt and reject a press rather than act indefinitely on an old actor.

Opening or closing any bridge UI blocker increments an ephemeral permission revision. This detects a menu that opens and closes between two relay ticks. New/load changes the existing session epoch. Either invalidates the previous target and requires a fresh released input. Pausing, unavailable providers, recovery and pawn replacement also disable relay interaction.

An invalid or unregistered supplied actor clears the earlier lease. A delayed focus-loss callback can use `ClearFocusedTarget(ExpectedActor)`; loss of actor A cannot erase newly selected actor B. Null focus explicitly clears the lease. Destroyed actors are held weakly and cannot be made valid by an old raw ID.

A press without valid focus is consumed and returns `StaleFocus`. It is never queued or retried when focus later arrives. The player must release and deliberately press again. Range, visibility, ownership and actual game state are checked again by the bridge when execution occurs; a prior prompt is not a permission token.

## 6. Read-only prompt data

`PreviewInteraction(Target)` returns a small `FCoastalInteractionOffer`: WorldId, ActionText, Detail, bVisible and bCanInteract. It reads the existing actor and mission state. `OnOfferChanged` publishes changed display data; the real Hyper prompt handler should display, disable or hide its prompt accordingly. The relay also exposes `GetCurrentOffer` for initial presentation. No new widget asset or native prompt is supplied.

The action changes from Open/Close door, Take item, Open storage, Read/Review note or postcard, and Inspect radio → Repair radio → Listen → Replay. Collected pickups and absent/stale targets produce a hidden offer. Too-far/occluded targets can produce visible disabled offers. Repair reads actual carried requirements; storing either required part prevents a ready-to-repair offer. Storage and transcript offers require their actual callbacks to be bound.

Prompt generation never inspects the radio, consumes repair parts, acknowledges the transcript, grants journal entries or requests a save. An enabled pickup offer does not guarantee free grid capacity: only the actual provider transaction decides that. The source does not infer capacity from a display list. Query correctness still assumes the real adapter honors its read-only contract.

Suppressed input is intentionally silent rather than spamming feedback every held frame. Missing/expired focus records a diagnostic in relay `LastDetail`; the actual prompt disappears. Accepted bridge actions still use the existing UI action notices. The default Take text is generic; this is not an item-icon or finished localization pass.

## 7. Startup and failure behavior

The M1.3 `StartTestRoom` signature remains unchanged. The bootstrap accepts zero or one relay on its local controller. Zero preserves existing integrations; it does not certify that their input has the new safeguards. A present relay must be registered, uninitialized and have a supported input owner.

With a relay, initialization is save coordinator → bridge → relay → native UI, after the existing read-only preflight and actual provider readiness. Failure after permanent binding has started remains terminal: poison further campaign writes, expose diagnostics, and require relaunch. It does not create or load a campaign automatically.

A repeated successful bootstrap request also checks that an originally present relay still exists and is initialized. This is an explicit repeat check, not a new global lifetime watchdog. Destroying an initialized relay removes its own inputs and publishes a hidden prompt. The fixed M1 pawn lifecycle is retained; silent pawn rebinding was not added.

## 8. Verification boundary

The new standalone suite exercises the same helper source used by runtime: one-action-per-frame and reentry, press/release after menus and loads, focus expiry and old loss callbacks, every combination of the seven world-permission flags, radio/action-label selection, and small combined input regressions. These are helper checks, not actual door movement, AGIS operations or a controller playthrough.

Retained mission/campaign/envelope/UI/startup/manifest tests are rerun. Four new native automation tests cover fail-closed relay ownership and offers, real bridge same-frame rejection, and the shared session/focus rules, bringing the supplied engine test inventory to fifteen. They were not run here. No mock engine or invented adapter is used as evidence of an Unreal integration.

See [current validation](../VALIDATION_REPORT.md) and [local wiring](M1_4_INTERACTION_WIRING.md). M1 is still gated on the actual full coherent Windows test-room loop. M2's coastal route, purchased art, final character, fishing, boats and combat are not included.

## References and source boundaries

The supplied foundation and M1.3 handoff establish the design and missing integration boundary. New relay, input contract and guard rules are original project implementation. Official references checked for engine entry points, not to certify the selected local engine or vendor package:

- [Epic Enhanced Input](https://dev.epicgames.com/documentation/unreal-engine/enhanced-input-in-unreal-engine) — Started/Completed/Canceled states and mapping contexts.
- [Epic UEnhancedInputComponent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/EnhancedInput/UEnhancedInputComponent) — component stack processing and action bindings.
- [Epic FModifyContextOptions](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/EnhancedInput/FModifyContextOptions) — pressed-key suppression during mapping rebuilds.
- [Epic APlayerController](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/APlayerController) — input-component stack and key-state queries.

Documentation pages displayed UE 5.8 when checked. That does not change the local compatibility decision or prove this package builds on that version.
