# M1.10 — Local Display Settings Wiring and Acceptance

## 1. Preserve the real host

Back up/commit the host project, actual AGIS adapter and vendor wrappers, campaign saves, both Coastal player-preference slots and the engine's actual user-settings file. Merge the complete `Plugins/CoastalFoundation` source update; preserve any locally implemented adapter changes rather than blindly overwriting them. Keep purchased Content, GameMode, input actions, project configuration and selected engine/toolchain.

This archive contains no host `.uproject`, map, `.uasset`, DLL or executable. Rebuild Development Editor / Win64 and your actual game target. Old M1.9 binaries do not contain the new reflected internal display owner or UI declarations. No engine version is selected by a documentation page.

Campaign and player-preference schemas do not change. Existing schema-3 preferences remain current; inherited schema-1/2 forward reads still work. Engine video settings are a separate persistence path. Do not remove campaign files to fix a display configuration.

## 2. Declare one display owner

Use the existing **single `CoastalUISessionComponent` on the local PlayerController**. After removing/bypassing competing graphics menus and display-persistence handlers, set its new `bEnableDisplaySettings` property true before startup. Default false leaves display controls unavailable while the original game continues.

Do not add `CoastalDisplaySettings` as an actor component: it is an internal UObject the UI creates. Do not initialize a second object manually. Keep the same original host call:

```text
Bootstrap.StartTestRoom(ActualDirector, ActualAGISAdapter, ValidatedInitialDryCheckpoint)
```

Existing AGIS/Hyper, recovery, look/sprint, audio options and playback wiring remains required. This section installs neither vendor API nor another input context.

Use a Windows non-editor standalone game with exactly one local player and the standard `UGameUserSettings` class. The source refuses PIE/editor viewports, custom settings subclasses and a present XR system. Use the actual game executable or a correctly built native game target for display switching; editor automation tests alone cannot test the feature. Relaunch after changing ownership configuration. Keep UI ticking throughout its initialized lifetime.

## 3. Audit competing writers first

Inspect existing demo menus, GameUserSettings graphs, console shortcuts, Alt-Enter behavior, resolution CVars, auto-save callbacks and launch flags. Keep one intentional resolution/window-mode owner. The source can notice a distinct change to the two captured engine settings fields, but not all same-value writers, changes that happen and revert between polls, arbitrary config writes or another process.

The trial directly requests a runtime mode without staging engine preference fields. An unrelated host callback that saves current runtime state can defeat that intended separation. Remove or deliberately integrate it and record the actual behavior; do not call another owner's `ApplySettings` every tick or clear user settings as a workaround.

No monitor picker is supplied. Engine-reported convenient/windowed and fullscreen lists, and the desktop borderless size, are candidates. Record the actual monitor/DPI/driver/engine setup. Mixed-DPI and secondary monitors are explicit local tests, not advertised certified support. Prior window position, monitor choice, refresh rate and HDR are not part of the rollback snapshot.

## 4. Operate the native menu

Session or Pause → **Display settings**. Next available window mode cycles the categories actually admitted. Previous/Next resolution cycles sizes in the chosen category. Borderless uses the engine-reported desktop size. Refresh rereads the catalogue and resets the draft to the actual viewport. Back discards the draft.

Choose a different candidate, then **Test draft for 15 seconds**. Revert is default focus. The dialog displays the requested and previous mode, a real-time countdown and its three decisions. A post-change presentation reset scrolls the body into view; controller navigation can then bring the actions into view. Inspect this at 1280×720/150% text as well as 1080p. The source has not been rendered here.

**Keep for this session only** retains the runtime mode but does not change engine saved preference fields. **Keep and request engine settings save** additionally calls the standard engine writer. That writer returns no verified disk-success result, so the UI correctly reports a save request and requires relaunch testing. These operations are not campaign saves and do not touch Coastal's ten-field player-preference codec.

Fresh Escape/B or Revert can cancel on a later frame even before the dialog body paints; an opening-frame/held command cannot confirm. Keep requires the current top ticket/epoch, foreground window, exact requested viewport mode/size, post-match body presentation and a later command before the deadline.

## 5. Timeout and recovery expectations

The deadline uses `FPlatformTime::Seconds()`. Pause does not freeze it. The first running observation at/after 15 seconds requests the actual previous mode. A stalled game thread cannot execute code during the stall; on resume it rejects late Keep. There is no external OS watchdog.

Revert submits one deferred request. The UI reports it as pending until a later-frame observation sees the original size and window mode. Further trials are blocked while restoration is pending. Failure to observe it within five seconds locks this display owner until relaunch. No guessed fallback resolution or retry loop is used.

Observed focus loss, session replacement, recovery/return, a missing dialog or lost fixed owner cancels. A source/device/viewport replacement is never silently adopted. If the old viewport no longer exists, the diagnostic must say restoration was not verified rather than issuing commands against the replacement.

An ordinary UI teardown requests best-effort restoration for an active trial before releasing its references. Once ticking stops it cannot verify a later result. Test normal exit and repeated cleanup; do not disable the UI tick in the middle of an active trial and claim timeout safety still exists.

## 6. Engine settings persistence tests

Find and back up the actual engine user-settings file used by your packaged host; this source does not assume a machine-specific path. Use disposable profiles for read-only/disk-failure/termination tests. Run one game/editor writer per profile.

Test without Keep and verify no source-initiated save occurred. Test session-only Keep then relaunch and inspect the actual previously saved mode. Test explicit Keep plus save and verify the actual new mode after relaunch. Do not infer a successful disk write solely from the returned Keep result or `SaveSettings` completing.

The standard writer serializes existing engine settings, not a transaction limited to two custom fields. Capture before/after files and inspect unintended changes. Existing non-display fields may be serialized, and resolution setters can update derived engine bookkeeping. Graphics-quality application, benchmarking, VSync, frame caps, render scale and HDR are not controls delivered here.

Keep source fallback instructions available outside the game during risky tests. Restore a backed-up engine settings file only while all game/editor instances using it are closed. Never delete unrelated campaign or local control/audio preferences as a display repair step.

## 7. Execute the retained real milestone gate

Build with the real selected engine/toolchain. Run all **38 supplied Unreal automation tests**, including four `Coastal.M1Display` tests. Those four exercise unbound rejection/shared rules and do not perform a real display switch. Execute all **38 new display acceptance scenarios** in [the ledger](../data/m1_10_display_acceptance.json); its count is separate from the native automation inventory.

Run all retained mission/save/UI/startup/interaction/recovery/options/sprint/audio/playback ledgers. Test controller-only and keyboard/mouse; capture real layouts, current viewport observations, mode/driver behavior, file faults and relaunch results. Keep true engine/toolchain/vendor versions, source hash and actual evidence in a separate local copy; shipped cases remain `not_run`.

Package Windows and launch outside the editor. Complete New → pickup → storage round-trip → repair → transcript → safe return → verified campaign save → exit/relaunch → Continue. Verify local controls/audio settings and explicit display persistence independently. Only that coherent actual-host result passes M1. M2 coast assembly and final M3/M4 work remain as described in [What remains](WHAT_REMAINS.md).
