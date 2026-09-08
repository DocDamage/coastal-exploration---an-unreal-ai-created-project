# M1.7 — Local Walk/Sprint Wiring

## 1. Preserve the actual host and preferences

Back up/commit the real host project, your implemented AGIS adapter, campaign saves and both local preference files. Replace only the complete `Plugins/CoastalFoundation` folder from this package. Retain purchased content and host input/project configuration. Rebuild Development Editor / Win64 with the engine/toolchain selected by the real asset compatibility test.

No `.uproject`, `.umap`, `.uasset` input mappings, compiled DLL or Windows executable is supplied. Old M1.6 DLLs cannot supply the new reflected component. The preference reader supports original M1.6 schema 1, but an explicit new save writes schema 2, which the old plugin cannot read. Keep the backed-up pair for deliberate rollback.

Existing prerequisites still apply: actual AGIS initialization and atomic contracts, one director/bridge, actual Hyper target/input route, one native UI/bootstrap owner, and optional look/recovery integration. This feature does not wire those packages automatically.

## 2. Put the speed owner on the character

Add exactly one `CoastalSprintComponent` to the **actual possessed Third Person character**, beside its interaction bridge and optional look/recovery components. Do not put it on the PlayerController or director. It is optional: with no component the retained host movement continues and the Sprint mode setting remains disabled.

The defaults are `WalkSpeed=330`, `SprintSpeed=520` in centimetres/second, and `bUseHostSprintEvents=false`. Leave the opt-in false until actual state forwarding and sole speed ownership are in place. There is no automatic new mapping context.

Remove/bypass other Sprint handlers that write `CharacterMovement.MaxWalkSpeed`, including vendor/demo examples and Blueprint tick resets. Retain move direction, analog magnitude, jump, camera and existing menu handling. Do not change every mapping context or release somebody else's input locks to solve a conflict. The component owns just this speed property, not the character framework.

Use the standard non-crouched walking character. Tuning must be finite and pass the documented bounds, with actual `MinAnalogWalkSpeed` between zero and WalkSpeed. The component does not change that analog threshold or silently repair invalid values. Custom/root-motion locomotion requires deliberate testing; a speed cap does not prove actual movement speed.

## 3. Keep the same single startup call

After real provider readiness, possession and valid dry spawn, retain:

```text
Bootstrap.StartTestRoom(ActualDirector, ActualAGISAdapter, ValidatedInitialDryCheckpoint)
```

The existing native UI options owner initializes a present Sprint component automatically while initializing local preferences. Do **not** add a second manual Sprint initialization call. The bootstrap itself is unchanged. Camera/look and sprint consumers are independent; Sprint mode does not require the optional camera component.

A duplicate/unready/unopted-in consumer produces an options warning instead of a fake working mode. `IsSprintReady()` means native prerequisites and speed ownership are valid; it does not prove the host actually sends current input. Read `LastDetail` and the native UI/log for failures.

## 4. Forward one real aggregate logical state per frame

The original project API is:

```text
SprintComponent.SubmitSprintInput(ActualSprintActionIsDown)
```

The host must supply the **current actual logical held state** of its Sprint input. Combine all mapped devices into one state. The foundation defaults are Left Shift and left-stick click. Those are design mappings, not included assets or claims about installed vendor symbols.

| Host input situation | Value to forward |
|---|---|
| At least one Sprint binding is physically/logically down | `true` once for that frame. |
| All Sprint bindings are actually released | `false` once for that frame. |
| Menu closes but the button is still held | The actual `true`, not a synthetic release. |
| Key was released during a menu and is now actually up | The actual `false` on an eligible post-menu frame. |
| Input context is canceled but physical release is unknown | Do not invent a release; obtain the real current state through the host's input route. |

Do not forward `true` followed by `false` for every Triggered callback, nor assume a Canceled event means physical release. A nonzero-only action callback is insufficient because rearming needs released samples. A forever-cached Started value is also insufficient. Keep one aggregate sampler after actual input evaluation, not multiple per-device calls competing within a frame.

`SubmitSprintInput` returns whether a state sample was accepted. It is **not** a promise that sprint started, that the player moved, or that a transaction committed. Inspect `IsSprintRequested` only as a transient speed request. Do not use it as a mission fact or saved state.

The component follows controller input tick and precedes movement. A host sampler in Blueprint actor tick or another subsystem may need an explicit compatible order. Avoid circular prerequisites. Verify the real frame order and responsiveness locally rather than treating a successful Blueprint connection as acceptance.

## 5. Release/repress and movement behavior

After startup/menu/load/recovery, the enabling frame cannot start Sprint. Send actual release on a later eligible frame, then a later deliberate press. Holding through a menu or safe return never resumes sprint automatically.

Hold selects sprint only while the accepted input remains held. Toggle preserves the selection on release and toggles off on the next press. Neither mode supplies movement direction. Stopping movement input stops the character using its ordinary physics even with Toggle selected.

Normal walking is the only sprint-eligible mode. An observed jump/fall, crouch or other movement mode cancels the request; after landing, release/repress is required. The implementation does not zero airborne velocity. `MaxWalkSpeed` also affects lateral falling speed in Unreal, so record actual jump/deceleration behavior and camera comfort with your host tuning.

Input samples expire after the sample frame plus two frames. Keep sampling actual state even while Toggle is on and Sprint is released. A lost heartbeat returns to walk when the running consumer observes expiry. Long frames stretch that lease in wall-clock time. A pulse wholly between samples may be missed; it is never queued for later action.

Keep the component tick enabled throughout its initialized lifetime. Before deliberately disabling it, release/remove the owner through its supported lifetime; disabled code cannot enforce heartbeat expiry. Keep safety recovery and menu owners at their existing priorities. Sprint does not acquire movement/look locks or replace their pause/input modes.

## 6. Use the existing options screen

Session/Pause -> Player options -> Previous/Next setting -> Sprint input.

**Switch Hold / Toggle sprint** changes the draft. **Back** discards unapplied edits. **Defaults** resets only available settings in the draft. **Apply for this session only** changes mode without a file write. **Apply and save local options** updates live values only after the existing exact verification succeeds.

Mode changes invalidate active Sprint. Native menu closure then requires fresh release/repress as usual. No active key state or toggle latch is saved. Starting/continuing another campaign preserves the locally selected mode but starts without an active sprint request.

When the consumer is unavailable, its field is disabled. A consumer lost after editing blocks application of that changed mode rather than silently changing a control that no longer works. Text and compatible look/camera controls retain their existing functionality.

## 7. Preference upgrade and rollback

Both slot names remain unchanged:

```text
CoastalLocalOptions_v1_A
CoastalLocalOptions_v1_B
```

Original schema-1 data loads the five saved values and defaults Sprint to Hold. Loading alone writes nothing. Explicit Save writes 64 encoded bytes (schema 2) to the inactive slot with generation+1, verifies them and leaves the older valid file untouched by that operation. A subsequent normal save may alternate again; this is a previous-generation policy, not permanent archival of schema 1.

Keep only one game/editor writer per profile. Before platform fault tests, close the game and back up both disposable preference files. Never use valuable player saves as corruption fixtures. The included golden legacy bytes were generated by the old standalone codec; copying them into actual platform slots is a separate local test, not evidence of native IO here.

Do not downgrade the plugin against upgraded options: M1.6 treats schema 2 as unsupported. For a deliberate rollback, restore the backed-up old preference pair while all game/editor instances are closed. Do not delete unrelated campaign saves to fix a settings issue.

Equal generations, future schemas and read failures block unsafe selection/writes. Corrupted newer schema-2 data can recover older schema-1 values, including Hold, with a notice. A write failure can still leave valid new bytes on disk; further writes block until relaunch resolves the actual pair. Session-only apply remains available. CRC/read-back is not an OS process lock or crash-atomicity guarantee.

## 8. Conflicting writers and teardown

A distinct external `MaxWalkSpeed` edit disables Sprint and preserves the external value. The native HUD/options and Output Log report the conflict. Fix the duplicate writer outside play, then relaunch; there is no silent hot-rebind. Teardown restores the original speed only while it still owns its last value and removes its tick prerequisites.

Do not destroy or replace the fixed character during a normal M1 campaign. Deliberate fault testing should verify that Sprint does not follow an unrelated pawn. The existing UI/coordinator/recovery lifecycle has its own fail-closed requirements; this optional feature does not weaken them.

## 9. Build and execute the real gate

Run all 26 supplied Unreal tests: the retained `Coastal.FirstSignal`, `Coastal.M1`, `Coastal.M1UI`, `Coastal.M1Startup`, `Coastal.M1Interaction`, `Coastal.M1Recovery`, `Coastal.M1Options`, plus four `Coastal.M1Sprint` tests. They are unrun in this delivery environment. Helper-style native tests do not replace actual locomotion/controller/IO acceptance.

Execute all 35 cases in [the Sprint ledger](../data/m1_7_sprint_acceptance.json), plus retained campaign/UI/startup/interaction/recovery/options scenarios. Test both devices, aggregate presses, holds through every menu, falls/returns, legacy upgrade faults, foreign writers and actual 720p/1080p scrolling at 100–150% text. Retained M1.6 format assertions describe the old payload; use the M1.7 format for new writes.

Package the actual systems room for Windows, launch outside the editor and complete New -> collect -> storage round-trip -> repair -> transcript -> safe return -> verified save -> exit/relaunch -> Continue with preferences restored separately. Record actual engine/toolchain/vendor versions, source hash, build/runtime logs, screenshots and case outcomes in a separate local evidence copy.

**Only that tested Windows package passes M1. M2 coast assembly, final art/audio, graphics/remapping and complete accessibility remain later work.**
