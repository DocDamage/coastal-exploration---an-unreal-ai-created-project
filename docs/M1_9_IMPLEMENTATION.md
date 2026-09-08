# M1.9 — Ambience and Transcript Audio Playback

**Project:** Coastal Exploration / First Signal  
**Section date:** September 6, 2026, America/New_York  
**Plugin:** `0.1.9-m1-audio-playback-source`  
**Delivery:** Complete source increment based on the actual supplied M1.8 archive. No native Unreal build, audio recording, generated map or Windows executable.

## 1. Scope and source basis

M1.8 added routed volume settings but explicitly did not implement playback. The foundation calls for coastal ambience and a radio whose full transcript remains usable without audio. This section implements those two bounded presentation consumers for the existing systems-room source. Selecting this source increment is an implementation decision within that plan, not evidence that M1 passed or that a production M1.9 milestone had already been promised.

The actual host project, installed AGIS/Hyper packages and Unreal toolchain remain unavailable in this delivery runtime. The required next production gate is still real local integration and a Windows test-room package. This increment does not invent vendor APIs or build a substitute inventory. M2 coast assembly has not been attempted or claimed.

The playback scope is deliberately specific: **one local, nonspatial ambience bed and one optional radio recording associated with the visible transcript**. There are no footsteps, ambient zones, interior/exterior crossfades, spatial radio emitter, adaptive music, dialogue framework or newly supplied sound files. Those are not implied by a functioning playback API.

## 2. Delivered source

| Source under the plugin module | Responsibility |
|---|---|
| `Public/Core/AudioPlaybackRules.h` | Exact presentation/session command rules and source-metadata policy shared with standalone tests. |
| `Public/CoastalAudioPlaybackComponent.h` | Optional controller owner and authored sound references. |
| `Private/CoastalAudioPlaybackComponent.cpp` | Checked initialization, captured routing/device lifetime, UI-driven refresh, suspension and teardown. |
| `Private/CoastalAudioPlaybackSources.cpp` | Private native audio-component construction, class overrides, Play/Pause/Resume/Stop and destruction. |
| `Private/CoastalUIAudio.cpp` | Read-only projection from the existing native UI and discovery of one optional playback owner. |
| Existing UI session/stack source | Initialization after options/widgets, immediate cancellation hooks, status and ordered cleanup. |
| Existing audio-options component | Read-only captured-class accessors and a pre-release notification for dependent source cleanup. |
| `Private/Tests/M1AudioPlaybackAutomationTests.cpp` | Four supplied, unrun native automation tests. |

One new standalone suite exercises the shared playback rules. New specification, unresolved dependency register, 34-case native acceptance ledger and the exact existing transcript as a recording script are included. No additional settings row, binary asset or engine-module dependency is introduced.

## 3. Initialization and asset contracts

Add zero or one `CoastalAudioPlaybackComponent` to the same local PlayerController as the native UI and M1.8 audio-options owner. The character and director are not valid owners. `bEnablePlayback` defaults false. `AmbienceLoop` and `RadioTransmission` default null; either slot may be omitted, but at least one valid sound is required to initialize playback.

The existing UI initializes this owner after local preferences, the seeded volume mix and native widgets have initialized. The bootstrap signature is unchanged. Do not add a second host initialization or independent BeginPlay sound player for these sources. Initialization captures references and installs lifecycle hooks; **it does not call Play**.

Readiness requires the registered/ticking local controller component, one standalone player, the active native UI, a ready M1.8 routed volume owner, actual dedicated class references and the same live audio device. Missing/duplicate/disabled setup produces an optional-feature diagnostic, not a poisoned campaign or a fabricated audible result.

Assigned sounds are checked through real `USoundBase` metadata. The ambience must be playable and looping. Radio must be playable, nonlooping and report a finite duration greater than zero and no greater than 900 seconds. Both assigned sources must report PlayWhenSilent support. These are original project admission rules, not universal engine limits or measurements. An invalid assigned slot refuses this component's setup; omitting an unwanted slot is supported.

The 900-second bound is a metadata guard, **not a mission countdown or runtime watchdog**. A misleading graph can still behave differently from its metadata. Epic describes `IsPlayWhenSilent()` as true when any sound within the source has that setting; this cannot certify every branch of a complex cue. Prefer a simple, inspected project-owned source for the initial test, then validate actual graph, concurrency, virtualization and cooked behavior. No asset properties are rewritten by this plugin. [S2]

## 4. Ambience behavior

The title/session screen is silent. The ambience receives one Play request when a valid active campaign first enters eligible gameplay. A campaign selected while a modal remains open does not start it underneath the initial screen.

Opening any Coastal modal pauses the existing bed. Nested menus do not repeatedly pause, create extra components or call Play again. Closing the final modal resumes the same source when gameplay is allowed. External world pause and the existing safe-return reservation also suppress the bed. The component does not unpause the world or modify input ownership.

A new/load session epoch stops and destroys the previous bed. The new session may start one bed of its own after gameplay becomes eligible. Successful Continue does not deserialize a saved audio offset; no playback position is persistent. Failed Continue is not New, and the existing retained campaign/menu behavior remains authoritative.

The bed is a local 2D presentation layer. Spatialization and distance attenuation are disabled on the private component; it is not attached to a shoreline location or the player's mesh. This does not supply interior acoustics, environmental route transitions or a complete mix. It is intentionally a small source consumer for testing actual owned ambience.

## 5. Radio behavior and mission independence

The UI exposes a native, read-only playback context containing the coordinator's epoch, session eligibility, current top transcript ticket and whether its body has had the retained presentation opportunity. It exposes no acknowledgement callback to playback.

A radio recording starts only when the current transcript is topmost, visible, in the viewport, associated with a valid current token, and presented on an earlier frame. Merely receiving the bridge's transcript request or creating a widget does not start a clip. This remains a display-opportunity test, not proof the player read every word.

The source allows **one start attempt per monotonically increasing UI ticket**. Repeated refresh, body scrolling, a natural finish, muting, native voice rejection or a failed allocation does not retry it. A closed, canceled, replaced or interrupted ticket is retired, including a panel canceled before it was painted. A stale lower ticket or older session projection cannot resurrect it.

Back stops and destroys the voice without acknowledging the text. Explicit Continue uses the original bridge command and then closes/stops the voice, even if the clip has not finished. A later deliberate interaction with the repaired radio opens a fresh ticket and can replay from the beginning. Reading the stored transcript in the journal does not autoplay voice.

The radio component is marked as a UI sound because the existing transcript menu deliberately pauses the world. Native sound subtitles are suppressed for this source to avoid a competing text layer; the Coastal full transcript remains visible. Epic documents the UI-pause flag and notes that `OnAudioFinished` can occur after either natural completion or Stop. **This implementation binds no completion delegate at all.** [S1]

There is no call from playback into mission, inventory, journal mutation, save, load, repair or acknowledgement. No duration, audio position or mute flag enters quest state. A silent, missing, rejected or completed sound cannot grant `messageHeard`. Actual muted, canceled and early-Continue behavior must still be checked in the real host, especially after removing any older duplicate Blueprint playback/completion graph.

## 6. Native voices and retained volume routing

Private transient `UAudioComponent` objects have auto-activation and auto-destruction disabled, disallow simultaneous instances on one component, and stop with their owner. Creation sets the real sound, captured class override and captured device ID before registration and playback. Registration failure destroys the partial component. A stopped/replaced voice is destroyed, rather than reused for a different transcript's asynchronous lifetime.

Ambience uses the captured Ambience class; radio uses the captured Radio class from the initialized M1.8 owner. Editable class properties changed later are not silently hot-swapped. Component volume and pitch multipliers are both 1. Master/category gains remain exclusively in the retained mix, avoiding a second Master multiplication. The source does not create a new mix, change Windows volume or clear other modifiers. The relevant native construction/attenuation APIs are documented by Epic. [S1, S3]

Volume edits do not spawn, stop or restart these sources. Assigned PlayWhenSilent metadata supports the intended mute/unmute continuity, but actual silent voice retention and resource cost remain host tests. A native Play request has no synchronous audible-output acknowledgement. An allocation failure can be reported directly; a voice dropped by concurrency or output hardware is not falsely reported as a confirmed sound or repeatedly retried by polling IsPlaying.

Sound command ordering is implemented, not measured audio-thread timing. Seeding the mix before Play avoids intentionally starting with default full volume, but actual muted startup must be captured locally. This package makes no no-click, zero-latency or calibrated-loudness guarantee.

## 7. Cancellation, routing release and teardown

The playback tick runs after the UI in PostUpdateWork and can tick while paused. UI Push/Close hooks refresh presentation immediately; clearing panels, successful session-change notices and safe-return notices suspend playback immediately. Save callbacks only stop presentation; they do not create a new widget, consume a token or reenter campaign mutation.

On normal UI teardown, source voices are stopped/destroyed **before** the volume owner is released. The volume owner now also broadcasts a guarded pre-release notification. A live playback subscriber stops its voices before that owner pops its mix, including explicit routing unregistration. Recursive/repeated Release calls cannot rebroadcast or prematurely remove the mix during that notification.

This ordering protects these owned sources. Other host sounds routed to the same classes are still the host's responsibility when removing their mix. It is not a global audio shutdown or guarantee about every independent sound.

Loss of the captured source, UI, routing or audio-device binding ends this feature and reports a diagnostic. Cleanup retains the original device reference through source destruction, removes only its own delegate/tick prerequisite, and does not automatically rebind. A fixed-pawn/session failure also prevents further playback through the UI projection. Existing campaign failure handling remains separate.

Keep both owners' ticks running while initialized; deliberately disabling all code execution cannot enforce a future cleanup. Release the owner through its supported lifetime before disabling it. A source graph, camera/audio framework, multiple PIE instances or independent sound player requires explicit integration rather than assuming this component arbitrates it.

## 8. Compatibility and verification

No campaign serialization, item/world IDs, mission/story text, receipt rules, inventory adapter, recovery, sprint, look, preference codec or options values were changed. Preferences still read schemas 1/2/3 and explicitly write schema 3, using the existing pair of slots. There is no new upgrade or schema-4 file. Source equality is not proof an old Unreal binary save loads with a new engine build.

See [current validation](../VALIDATION_REPORT.md) for exact compiler/sanitizer/tool output and [local wiring](M1_9_PLAYBACK_WIRING.md) for the actual host procedure. Standalone tests exercise the same command rules used by runtime, not audio devices, UHT, widgets or purchased APIs. Four new native tests bring the supplied Unreal inventory to 34; all remain unrun here. The [34-case ledger](../data/m1_9_playback_acceptance.json) likewise starts entirely `not_run`.

Only the real Windows test-room loop passes M1: New → pickup → storage round-trip → repair → transcript → safe return → verified save → exit/relaunch → Continue, including failure cases. M2 coast assembly and final audio/art are still later work.

## References and boundaries

The supplied foundation and M1.8 archive establish the design and previous implementation. All new command policies, limits and integration choices here are original project work. Official pages were checked September 6, 2026 for API meaning, not engine/vendor compatibility certification.

- [S1 — Epic UAudioComponent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UAudioComponent): component flags, routing override, device identity and lifecycle entry points.
- [S2 — Epic USoundBase](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/USoundBase): playable/looping/duration/PlayWhenSilent metadata and its limits.
- [S3 — Epic FSoundAttenuationSettings](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FSoundAttenuationSettings): distance attenuation and spatialization fields.

Current pages may display UE 5.8. The project still uses the engine selected by its actual asset-compatibility test; no engine version was pinned or certified here.
