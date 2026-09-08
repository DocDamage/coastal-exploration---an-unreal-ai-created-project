# M1.4 — Local Interaction and Hyper Wiring

## 1. Install without losing host work

Back up/commit the actual host project, saves and implemented AGIS adapter. Replace only this package's complete `Plugins/CoastalFoundation` folder. Keep purchased content and host configuration. Rebuild the actual Development Editor / Win64 target; the new reflected relay/offer types cannot be supplied by an old M1.3 DLL.

Keep the standard Third Person character/GameMode, the single native director, the seven registered M1 world objects, the real AGIS provisional-session setup and all original persistence contracts. The exact installed AGIS/Hyper symbols are not included here and must not be guessed. No `.uproject`, `.umap`, `.uasset` widgets or paid assets are included.

Read the retained [startup wiring](M1_3_STARTUP_WIRING.md) for actual provider readiness, ownership and checkpoint requirements. This guide adds the interaction route; it does not replace those prerequisites.

## 2. Add one relay and choose one input owner

On the same local PlayerController that owns `CoastalUISessionComponent` and `CoastalSessionBootstrapComponent`, add exactly one `CoastalInteractionRelayComponent`. Keep exactly one `CoastalInteractionBridge` on the actual possessed character.

Choose its InputOwner before play:

| Mode | Keep | Remove or bypass |
|---|---|---|
| `VendorEvents` — default | Actual host/vendor interaction mapping and actual aggregate action-state read. | Its direct world-use calls; instead submit the state to the relay. |
| `NativeEnhancedInput` | Real Hyper focus detection and prompt presentation. The relay creates E / face-left. | Competing host/vendor Interact action handlers/mappings. Do not submit external input to this relay. |

Do not remove Hyper target selection or its project-owned actor interface. Do not remove movement, look, jump, sprint or Coastal menu mappings. Never clear every mapping context to resolve a conflict. The native route does not make a vendor demo's unrelated inventory actions safe; keep all item mutations through the original game bridge/adapter.

A fully integrated legacy M1.3 host may omit the relay, but then must retain its own tested pressed-once input. The updated bridge's same-frame guard alone does not stop a direct call repeated across many frames while held.

## 3. Use the existing single startup call

After actual provider readiness and possession, call exactly the same original plugin method:

```text
Bootstrap.StartTestRoom(ActualDirector, ActualAGISAdapter, ValidatedDryCheckpoint)
```

The bootstrap now initializes a present relay automatically between bridge and native UI initialization. Do not separately call `InitializeRelay` in this path. Bind the relay's presentation delegate in the host readiness flow so that subsequent changed offers reach the real prompt presenter.

Zero relays remains supported for existing M1.3 integrations. Two relays, an already initialized relay or an invalid input-owner value fail preflight. A failed initialization after binding has started requires exit/relaunch, not another partial setup attempt.

For a host intentionally retaining manual setup rather than bootstrap, the original plugin calls are:

```text
Saves.Configure(ActualAGISAdapter, ActualCharacter, "level.systems_test")
Bridge.Configure(Saves)
Relay.InitializeRelay(Bridge)
UI.InitializeUI(Saves, Bridge, ValidatedDryCheckpoint)
```

Check every return. Use one startup path only. Failure after some of these manual calls have bound means stop the session, fix the host outside play and relaunch. Do not proceed with half the owners initialized.

## 4. Forward actual Hyper focus every frame

In your project-owned integration wrapper, obtain Hyper's actual current selected actor using its installed API. Cast/resolve it to the configured `CoastalWorldObject` subclass without changing its WorldId, Kind, PickupItem or JournalEntry. Forward it to:

```text
Relay.UpdateFocusedTarget(ActualSelectedCoastalWorldObjectOrNull)
```

These are names of this package's original methods, not Hyper API names. The wrapper must use the actual installed package to obtain the selection. There is no provided vendor class path or automatic binding.

Submit a fresh selection every local focus evaluation/frame, before submitting an external press. A callback only when focus changes is not enough: the lease expires after the sample frame plus two more frames. Do not keep replaying a cached actor while the vendor is no longer evaluating focus; submit its current selection or null. Stop presenting prompts when the returned offer is hidden.

When Hyper reports a specific previous actor lost focus, this optional original method is safe against callback order:

```text
Relay.ClearFocusedTarget(PreviouslyFocusedActor)
```

Loss of A does not clear newer B. A null current selection should be forwarded through `UpdateFocusedTarget(nullptr)`, which unconditionally clears a current permitted selection. The relay's weak actor reference also prevents a destroyed object remaining actionable.

Opening/closing menus and loading/starting a campaign invalidate previous focus. Forward the current real selection again after the transition; the relay intentionally does not guess which actor should regain focus.

## 5. Submit actual input in VendorEvents mode

After forwarding the real selected actor, submit the real aggregate logical action state:

```text
Result = Relay.SubmitExternalInput(ActualInteractActionIsDown)
```

Use the actual held state, not `true` followed immediately by `false` for every vendor Triggered callback. That fake release defeats any pressed-once design. A held true can be sampled repeatedly but only the first eligible press acts. A released false arms a later press. Include release/current-state samples after menus and session transitions, even when no press happened, so the relay can distinguish a held key from a new press.

The normal frame sequence is: current vendor selection → real action state → presentation update. On re-entry to gameplay, a released sample and a later frame are required before a press can act. A press arriving with no fresh target returns `StaleFocus`, is consumed and is not automatically retried. Release and press again after focus is fresh.

NativeEnhancedInput does not use this function; external input is rejected in that mode. It creates its own Started/Completed/Canceled bindings. Test both keyboard and gamepad, including holding one mapped key while releasing the other. The logical interaction must remain held until both are up.

## 6. Use the real Hyper prompt presenter

Bind `Relay.OnOfferChanged` to your existing prompt display logic. Use the offer fields as follows:

| Field | Host behavior |
|---|---|
| `bVisible == false` | Hide the prompt; clear old action text instead of displaying a stale target. |
| `ActionText` | Display the current original game action: Open, Take, Inspect, Repair, Listen, Replay, Read or Review. |
| `bCanInteract` | Enable/disable the prompt action. Never treat this as a reservation or skip commit-time validation. |
| `Detail` | Show a useful disabled reason, such as moving closer or carrying both radio parts. |
| `WorldId` | Match the prompt to the authored persistent target, not a replacement inventory item ID. |

`GetCurrentOffer()` supplies the current read-only value when attaching the presenter. Do not automatically execute an interaction, acknowledge a transcript, add a journal entry or request a save from a prompt callback. The callback is presentation only. No new prompt widget is included, so this must reach the actual installed Hyper presentation API.

Offer reads are advisory. A pickup may have an enabled Take offer and still return NoSpace from real AGIS. That failure must leave the actor and item retrievable. Repair availability reads carried inventory only, so a part moved to storage disables repair until returned.

`SuppressedInput` is not success and generally needs no toast. Stale focus is recorded in relay `LastDetail`; the actual prompt hides. Bridge action notices continue to feed the existing native UI. There is no new polished error widget or glyph system.

## 7. Build and execute the actual tests

Run all fifteen supplied Unreal automation tests using groups `Coastal.FirstSignal`, `Coastal.M1`, `Coastal.M1UI`, `Coastal.M1Startup` and `Coastal.M1Interaction`. None was executed in the delivery environment. Passing the four new native unit-style tests still does not validate Hyper's real focus callbacks, a rendered prompt, or actual AGIS atomicity.

Use [the M1.4 interaction acceptance ledger](../data/m1_4_interaction_acceptance.json), then retained [startup](../data/m1_3_startup_acceptance.json), [UI](../data/m1_2_ui_acceptance.json) and [campaign](ACCEPTANCE_TESTS.md) acceptance. Test both input modes separately, not together. Use disposable save sets and back up real saves before fault cases.

Especially verify repeated door and radio inputs, UI open/close while holding Interact, load/new transitions, focus heartbeat expiry, callback reentry, pawn destruction/replacement, read-only offer polling, storage while the UI pauses the world, full backpack pickup, repair failures and relaunch/Continue. Test with the real host at variable frame rates and with controller-only menus.

Package the actual systems-test map for Windows and launch outside the editor. Record the engine/toolchain/vendor versions, source hash, logs, actual screenshots and scenario outcomes in a separate evidence copy. Complete pickup → storage round-trip → repair → transcript → verified save → exit/relaunch → Continue without loss or duplication. Only that exercised package passes M1; the supplied source does not claim M2 coast construction.
