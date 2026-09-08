# M1.9 — Local Ambience & Radio Playback Wiring

## 1. Install the complete plugin without losing host work

Back up/commit the real host project, project-owned AGIS adapter, vendor wrappers, campaign saves and local preferences. Replace only the complete `Plugins/CoastalFoundation` folder from this package. Keep purchased Content, input mappings, GameMode and project configuration. Rebuild **Development Editor / Win64** using the engine/toolchain chosen by your real asset-compatibility test.

The archive contains no `.uproject`, `.umap`, `.uasset`, recording, compiled plugin or Windows executable. Old M1.8 DLLs cannot provide the new reflected class. This update changes neither campaign nor preference schema: existing schema-3 options remain current. Earlier schema-1/2 preference readers still cannot read schema 3; that inherited downgrade boundary is unchanged.

Retain [startup](M1_3_STARTUP_WIRING.md), [Hyper interaction](M1_4_INTERACTION_WIRING.md), [safe return](M1_5_RECOVERY_WIRING.md), [look/options](M1_6_OPTIONS_WIRING.md), [sprint](M1_7_SPRINT_WIRING.md) and [audio routing](M1_8_AUDIO_WIRING.md). Actual AGIS and Hyper integration is still mandatory; this feature supplies neither package's API.

## 2. Keep the existing three-class volume route

Keep one `CoastalAudioOptionsComponent` on the local PlayerController. Its real Ambience, Effects and Radio sound-class references must pass the M1.8 independent-class requirements. Set `bUseDedicatedSoundClasses=true` only after the actual classes and sound routing are ready.

Playback uses the captured Ambience and Radio classes from that initialized owner, not guessed asset paths or mutable property values. Effects remains available for actual host effects but this increment does not implement footsteps, pickup or door sound triggers.

Do not create a second master volume multiplier on the sources. The native playback components use volume/pitch multipliers of 1 and let the existing mix control volume. Do not clear all sound mixes or replace the host's global base mix to make this feature work.

## 3. Assign actual owned sounds

Create or choose appropriate project-owned sound assets/wrappers using sound content you actually own. Keep provenance and licensing separate from original source. No sound is included or downloaded by this archive.

| Playback property | Required authored source |
|---|---|
| `AmbienceLoop` | Playable looping source reporting PlayWhenSilent support. This is one local 2D bed, such as a carefully mixed shoreline/wind loop. |
| `RadioTransmission` | Playable nonlooping source reporting PlayWhenSilent support, with a finite reported duration in `(0, 900]` seconds. Match the original transcript. |

Either property may remain empty. One valid assigned source is sufficient. Both empty means there is no playback feature to initialize. An assigned source that fails its metadata checks refuses this component's configuration; remove the unwanted assignment rather than bypassing the check.

Use the [exact radio recording script](../data/m1_9_radio_recording_script.txt) for an optional local recording. It is the existing story text, not a new lead or rewritten mission. The player must still have that full text even with no recording.

Configure virtualization deliberately in the actual sound assets. PlayWhenSilent is required by this bounded consumer so mute is intended to preserve the playback timeline. For composite cues, the metadata query does not prove every branch behaves that way. Inspect the underlying source(s), concurrency, streaming and cooked output. A simple single-source test is preferable to an uninspected graph with its own loops, class changes or triggers.

The plugin does not alter these asset settings. Its 900-second admission cap is not a timer that stops a misleading looping graph. Validate the actual clip once before treating it as a valid transmission source.

## 4. Add one playback owner to the local controller

Add exactly one `CoastalAudioPlaybackComponent` to the same local PlayerController as `CoastalUISessionComponent`, `CoastalSessionBootstrapComponent` and the audio-options owner. Do not put it on the character or mission director.

Set `AmbienceLoop`, `RadioTransmission` and `bEnablePlayback=true` before play, after the actual source/routing setup. Leave the default false when the host is intentionally testing without sound. The opt-in flag is a setup declaration, not proof of audible output.

Remove/bypass any competing host BeginPlay ambience, radio interaction Play call, transcript playback listener or audio-finished mission callback for these same sources. Keep unrelated audio systems that intentionally own other sounds. The native controller component is now the only player for this one bed and transcript recording.

Do not manually spawn another audio component, bind another listener to acknowledge the token, poll IsPlaying to restart the clip, or call Play from every UI refresh. The new source owns these lifetimes already.

## 5. Keep the same single startup call

After actual AGIS provisional initialization, possession and valid dry-floor placement:

```text
Bootstrap.StartTestRoom(ActualDirector, ActualAGISAdapter, ValidatedInitialDryCheckpoint)
```

The native UI initializes preferences and the seeded volume mix, builds its widgets, then initializes the optional playback component. Do **not** add a separate InitializePlayback call. Initialization itself plays nothing. Choosing New/Continue and reaching eligible gameplay starts the bed.

This ordering also applies to a host intentionally retaining the older manual startup path: the existing `UI.InitializeUI(...)` now handles playback. Do not execute both manual and bootstrap paths.

Keep both UI and playback components registered and ticking. The new component ticks after the UI and during pause, so it can observe actual top-panel presentation. Before deliberately disabling/removing it, use its supported release/lifetime flow. No non-running component can promise to stop a future stale source.

`IsPlaybackReady()` reports source/owner/routing prerequisites, not a speaker acknowledgement or content approval. Read `LastDetail` in the native HUD/menu or Output Log. Source properties are captured for the initialized lifetime; editing them during play does not hot-swap a recording. Stop/relaunch after changing configuration.

## 6. Expected player behavior

The initial Session menu is silent. Start a campaign and enter gameplay: one ambience bed begins. Open Pause, then settings or journal: the same bed pauses, stays paused through nested menus, and resumes after the final Back. Safe return similarly suspends the bed without changing inventory/progress. Session changes stop old voices rather than carrying their offsets into another campaign.

Interact with the repaired radio. The original transcript must appear before the voice receives its one Play request. Because the transcript modal pauses the world, the radio source is intentionally configured as a UI sound. Its native subtitles are suppressed; the complete Coastal text remains the one presentation owner.

Let the recording finish: the transcript must stay open and the objective must not advance until the player explicitly continues. Press Continue early: the normal acknowledgement may complete the mission, then closes/stops the voice. Press Back: stop audio without granting completion. Reinteract deliberately: a fresh transcript ticket permits one replay from the beginning.

The journal does not autoplay a recording. Scrolling or rereading a panel does not replay it. A canceled unpainted panel cannot start sound later. Holding Confirm/Back still follows the retained one-command-per-frame and release protections; playback installs no new input mapping or focus owner.

## 7. Mute, failures and teardown

Test Master=0 before relaunch to verify there is no audible startup burst. Check Ambience and Radio independently, then mixed Master/category values, using real output. This code submits source commands after the seeded mix; it cannot confirm an audio-thread/speaker result synchronously.

Complete First Signal with Radio=0, Master=0, the radio reference absent and the entire playback component absent. Full text and Continue must remain available in all four cases. Audio must never determine item consumption, journal insertion, save success or `messageHeard`.

A source allocation/registration failure reports a presentation diagnostic and does not retry indefinitely. An engine Play command can also be rejected or become inaudible without a synchronous failure return; that is not represented as verified sound. This implementation never polls IsPlaying to manufacture a retry loop. A new deliberate radio interaction can make a new attempt; the same transcript cannot.

Normal UI shutdown stops/destroys sources before releasing their volume mix. A pre-release hook also stops them when the routing owner is explicitly released first. Test recursive/repeated release and leave an unrelated mix playing to verify ownership boundaries. Other host sources using these classes are not automatically stopped by this hook.

A changed device, invalidated source or lost native UI/routing lifetime stops this feature and refuses automatic rebind. Correct the host outside play, then relaunch. The component does not reset a campaign, replace preferences or clear a poisoned save coordinator to recover audio.

## 8. Execute the actual engine and Windows gates

Run all **34 supplied native automation tests** under the retained Coastal groups plus four `Coastal.M1AudioPlayback` tests. The new tests cover unbound failure, shared presentation rules, source metadata rules and unbound release reentry. They do not play actual sound or access your profile files. They are unrun in this delivery environment.

Execute all **34 cases** in [the playback acceptance ledger](../data/m1_9_playback_acceptance.json), alongside retained campaign/UI/startup/interaction/recovery/options/sprint/audio cases. These are two separate counts: 34 native automation tests across the full plugin and 34 new manual/integration acceptance scenarios. Neither is the standalone C++ assertion total.

Use actual keyboard/mouse and controller-only routes at 1920×1080 and 1280×720, 100% and 150% native text. Verify paint-before-play, early Continue, cancellation, muted transcript completion, no spontaneous replay, safe return, device/owner loss, title silence and cleanup. Perform save/IO failure cases only with disposable backed-up files.

Package the real test room for Windows and launch outside the editor. Complete New → pickup → storage round-trip → repair → transcript → safe return → verified save → exit/relaunch → Continue, with actual AGIS, Hyper, sounds and preferences. Record exact engine/toolchain/vendor versions, source checksum, real sound/class paths, build/runtime logs, audio observations and actual screenshots in a separate evidence copy.

**The shipped report remains source-environment evidence. Only the actual packaged campaign loop passes M1. M2's coast, final character/environment/audio art, footsteps and complete presentation remain later work.**
