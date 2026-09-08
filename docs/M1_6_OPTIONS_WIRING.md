# M1.6 — Local Player Options Wiring

## 1. Install the complete source update

Back up/commit the real host project, implemented AGIS adapter, campaign saves and any local preference files. Replace only `Plugins/CoastalFoundation` with the complete folder from this package. Keep all paid content and host configuration. Rebuild Development Editor / Win64 with the engine/toolchain selected by the actual asset-compatibility test.

No `.uproject`, `.umap`, Blueprint input-action assets, compiled plugin or Windows executable is included. Old M1.5 binaries do not contain the new reflected classes. Continue following the existing [startup](M1_3_STARTUP_WIRING.md), [interaction](M1_4_INTERACTION_WIRING.md) and [recovery](M1_5_RECOVERY_WIRING.md) guides. Real AGIS provisional initialization and Hyper target/prompt wiring are still required.

## 2. Text options are already connected to the native UI

Keep the same single controller-owned `CoastalUISessionComponent` and the same bootstrap call:

```text
Bootstrap.StartTestRoom(ActualDirector, ActualAGISAdapter, ValidatedInitialDryCheckpoint)
```

The UI now loads its own local preference object before creating widgets. Do not create an independent second options owner or call its initialize function from a competing Blueprint graph. The Session and Pause screens expose Player options. Text scaling works without any additional actor component.

Only native Coastal text is affected. Do not assume the purchased Hyper prompt, an unrelated vendor inventory screen, or editor UI will scale. Those systems require their real installed style API, which is not supplied here.

## 3. Optional FOV consumer on the real character

Add exactly one `CoastalLookInputComponent` to the possessed Third Person character, beside the existing interaction bridge and optional recovery component. Do not add it to the PlayerController or director. Leave `bUseHostLookEvents=false` for FOV-only integration while retaining the host's existing look input.

Before the bootstrap reaches UI initialization, the actual character must be the controller's view target and have exactly one registered active perspective `UCameraComponent`. Camera-component view discovery must be enabled on that actor. The new consumer captures that camera and applies the saved/default FOV once. The standard fixed third-person camera is the intended host; a custom camera manager, cine-camera lens or multiple active cameras needs deliberate integration, not a bypass of the checks.

The native UI initializes the component automatically. Do not also call InitializeOptions manually. A missing/incompatible binding leaves camera controls disabled while native text options remain available. A warning here is not a substitute for the original startup/inventory gate.

The component does not retune the spring arm, collision, interior occlusion or pitch clamps. Test those in the actual cabin. FOV is a setting, not a finished camera pass.

## 4. Route real mouse and stick inputs only once

To enable sensitivity and inversion, set `bUseHostLookEvents=true` before play **after** replacing the original direct look forwarding with this component's functions. Preserve the real host input action assets, mappings, dead zone and intended orientation. These new function names are project-owned APIs, not guessed Hyper symbols:

```text
Real mapped mouse look delta -> LookComponent.SubmitMouseLook(MappedMouseDelta)
Real mapped normalized stick state -> LookComponent.SubmitStickLook(MappedStickAxis)
```

Separate the two device paths in the host. A single aggregated action that hides its source cannot correctly be treated as both a per-frame mouse delta and a per-second stick rate. Use separate project-owned actions/bindings or the actual host's trustworthy device-specific route. This package does not create those `.uasset` input actions for you.

For mouse, send the actual mapped movement delta once per frame. Do not multiply it by delta seconds. For stick, send the actual normalized axis state after the host's dead zone/orientation, before multiplying by a rate or delta seconds. This consumer applies its own 90-unit/second base rate, selected percentage and capped game delta. No extra dead zone is added.

Do not retain the old AddControllerYawInput/AddControllerPitchInput calls for the same inputs, and do not scale the same settings in another modifier as well. Both devices can contribute once in the same frame, but duplicate callbacks from one device are deliberately suppressed. Existing controller/legacy axis scales remain part of the engine input path; inspect the actual host settings and record them in local evidence. Do not assume the numeric base rate bypasses them.

The component captures the `bUseHostLookEvents` mode at initialization. Changing the property while playing does not reconnect actions. Stop/relaunch after changing ownership. With the property false, SubmitMouseLook/SubmitStickLook deliberately refuse input; do not remove your original look handler in the FOV-only configuration.

## 5. Neutral stick samples after menus and recovery

The host must submit the actual current stick state, including actual zero/neutral samples after gameplay becomes eligible. Merely forwarding nonzero Triggered callbacks can leave the gate waiting for a release it never sees. Use the host's real current-state/readiness path rather than fabricating a true/false or neutral transition.

A canceled action because a menu changed context is not proof the physical stick is neutral. Do not send a synthetic `(0, 0)` on menu close. If the stick is still held, keep forwarding that actual state; it is suppressed until the player releases. If it was released during the modal, forward its real current neutral state after returning to gameplay, then a later deliberate movement can act.

Menus, session loads/new games, recovery blockers and independent controller look locks stop look. Permission revision catches a menu round trip even when it occurs between component ticks. Mouse cannot act on an enabling frame; stick requires a later neutral sample and then a later non-neutral sample. No rejected look sample is queued for replay.

Retain M1.4's actual Interact state and focus forwarding independently. This look consumer does not repair an unrelated interaction mapping, consume gameplay actions, install a mapping context or clear any controller locks.

## 6. Player-facing settings operations

The options panel's Previous/Next setting buttons choose a field; Decrease/Increase or Toggle changes the draft. Text size previews within that panel. Other fields change the live game only on Apply. Back discards unapplied edits and restores the parent focus.

**Apply for this session only** updates the live preferences without writing a file. It remains available when preference IO is blocked. New/Continue retains those values in the current UI lifetime. Closing and relaunching reloads saved preferences/defaults.

**Apply and save local options** rereads the observed slots, writes the inactive one and verifies exact bytes/fields. Only success updates live values. This is a preference save, not a campaign save. A failed write retains the previous live values and disables subsequent preference writes until relaunch; the draft can still be explicitly applied for the session. Do not display a campaign-saved message in response to this button.

**Defaults** changes available settings in the draft only. It is not a file deletion, corrupt-profile reset or campaign reset. Camera/look fields remain disabled if their consumer is unavailable. A camera disappearing after edits are made blocks applying those changed fields; close/reopen options to review the actual available controls.

## 7. Files, recovery and test isolation

Preferences use these two fixed platform save-slot names, user index 0:

```text
CoastalLocalOptions_v1_A
CoastalLocalOptions_v1_B
```

Use the actual platform save directory to inspect them; do not assume a machine path in this package is your installed game's path. They are separate from all campaign slots and do not appear in the campaign-name catalogue. New/Continue never overwrites preference records.

Run only one editor/game instance against a profile during testing. The source detects observed changes before writing, but does not hold a cross-process lock. A raw read may allocate its entire file before the bounded decoder checks it. CRC/read-back is accidental-corruption detection, not a security or crash-atomicity promise.

For fault tests, close the game, back up both disposable preference files, then create a controlled damaged/unsupported pair. The shipped save-envelope tool only understands the envelope and **does not certify the new options payload**. Do not label its envelope-only result as a valid preference schema. Restore backups outside play as needed. Existing files without a safe selection are not automatically erased; close the game and resolve them explicitly before relaunch. Never remove unrelated campaign files to fix preferences.

A platform write may reach disk even when success was not confirmed. The source prevents another write in that session. After relaunch, the loader may correctly select the new generation. Test this actual behavior locally rather than assuming the in-memory failure means no bytes changed.

## 8. Compile and run the native gates

Run all 22 supplied Unreal tests: retained `Coastal.FirstSignal`, `Coastal.M1`, `Coastal.M1UI`, `Coastal.M1Startup`, `Coastal.M1Interaction`, `Coastal.M1Recovery`, and three new `Coastal.M1Options` tests. The new tests cover unbound consumers, codec and modal/presentation helpers. They deliberately do not write the developer's real preferences and do not replace controller/camera/IO acceptance.

Run every row in the [34-case options ledger](../data/m1_6_options_acceptance.json), plus retained UI, startup, interaction, recovery and campaign ledgers. In particular, inspect every native screen at 100% and 150% text in 1920×1080 and 1280×720, using mouse and controller-only routes. Check command visibility, focus after reflow, long transcripts, save-name editing, terminal recovery, and the absence of world input under menus. The source has not been rendered here.

Package and launch the actual Windows test room outside the editor. Complete New → collect → storage round-trip → repair → transcript → safe return → verified campaign save → exit/relaunch → Continue with the selected preferences restored separately. Record engine/toolchain/vendor versions, source checksum, actual build/runtime logs, screenshots and case outcomes in a separate evidence copy.

M1 remains unpassed until that real integration succeeds. The next production world remains M2's Cabin Cove, Shoreline Trail, Old Dock and optional Rail Overlook—not another claimed result of a source-only test.
