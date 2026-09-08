# M1.8 — Routed Audio Options and Preference Upgrade

**Project:** Coastal Exploration / First Signal  
**Section date:** September 6, 2026, America/New_York  
**Plugin:** `0.1.8-m1-audio-options-source`  
**Delivery:** Complete source increment based on the supplied M1.7 ZIP. No Unreal-compiled plugin, audio assets, generated map or Windows executable.

## 1. Scope and source basis

The supplied foundation requires volume controls and a readable radio transcript independent of audio. M1.7 implements walk/sprint and leaves final audio and other presentation work unfinished. This increment brings the volume-control portion of that planned presentation work into the existing systems-room source. It is an implementation choice within the foundation, not a previously promised M1.8 production milestone and not evidence that M1 passed.

The actual supplied M1.7 source archive is the baseline. Its manifest was verified before modification. No real host `.uproject`, installed AGIS/Hyper package or Unreal installation is supplied in this environment. AGIS stays the sole inventory authority; Hyper stays focus/prompt owner; the existing UI stays menu and preference owner. This source neither invents vendor APIs nor substitutes a new inventory, input framework or synthetic playable world.

**This section controls routed audio. It does not create ambience, footsteps, radio voice playback, audio assets or a soundtrack.** Real owned sounds and their routing must be connected locally. No playback button is presented for nonexistent audio. The next production-world section remains M2's Cabin Cove, Shoreline Trail, Old Dock and optional Rail Overlook after the actual M1 packaged gate.

## 2. Delivered behavior

| New field | Default | Range and step | Consumer |
|---|---:|---|---|
| Master volume | 100% | 0–100%, step 5 | Multiplies all three explicitly routed game classes. Not Windows/system volume. |
| Ambience volume | 100% | 0–100%, step 5 | Actual dedicated ambience sound class. |
| Effects volume | 100% | 0–100%, step 5 | Actual dedicated effects sound class. |
| Radio volume | 100% | 0–100%, step 5 | Actual dedicated radio sound class. Never changes transcript or mission facts. |

The original six fields retain their order and meaning: mouse, stick, FOV, text, invert-Y and sprint. Four fields are appended; existing indices do not move. Options now has ten rows, inside the retained fully scrolling native panel. There is no music slider without an implemented music category, no device-selection UI, no EQ, compression, normalization, dynamic ducking or surround configuration.

All four volume fields are unavailable unless one properly configured audio component is ready. A missing component does not disable text or other available settings or prevent the campaign from starting. An opt-in flag or valid class references cannot prove that actual sounds use those classes; readiness reports command prerequisites only.

## 3. Source ownership

| Source | Responsibility |
|---|---|
| `Public/Core/AudioOptionsRules.h` | Exact scalar math, independent-class metadata checks, audio-field comparisons/copying, and one-mix command lifecycle used by runtime and standalone tests. |
| `Public/CoastalAudioOptionsComponent.h` and `Private/CoastalAudioOptionsComponent.cpp` | Optional local-controller owner; checks real class/device bindings, creates one private transient `USoundMix`, submits native overrides and cleans up its own mix. |
| `Public/Core/PlayerOptionsRules.h` | Four appended bounded integer fields; equality, validation and adjustments. |
| `Public/Core/OptionsPersistence.h` | Read schemas 1/2/3; exact current schema-3 encoding. |
| `Public/Core/OptionsSession.h` | Retained verified-write protocol plus read-only source-schema/upgrade metadata. |
| `Private/CoastalLocalOptions.cpp` | Explicit legacy/default-volume and upgrade notices through the existing native preference wrapper. |
| `Private/CoastalUIOptions.cpp` and UI session source | Optional owner initialization, availability, draft/default/apply/save, runtime diagnostics and teardown. |
| `tests/audio_options_tests.cpp` / `tests/audio_upgrade_tests.cpp` | New standalone rules and real-old-encoder-fixture coverage. |
| `Private/Tests/M1AudioOptionsAutomationTests.cpp` | Four native tests supplied but not executed. |

No new engine module dependency is added. `FAudioDevice`, `FAudioDeviceHandle`, `USoundClass` and `USoundMix` are consumed from the existing Engine dependency. The source uses documented native device operations rather than a guessed third-party mixer. [S1–S4] The exact engine remains the local compatibility decision; API documentation is not compilation evidence.

## 4. One optional controller component

Add zero or one `CoastalAudioOptionsComponent` to the same local PlayerController as the existing UI. It does not belong on the pawn or mission director. The native UI discovers and initializes it during its existing `InitializeOptions` path. The bootstrap signature remains unchanged. Do not initialize it a second time manually or create another preference owner.

`bUseDedicatedSoundClasses` defaults false. Enable it only after binding and routing three actual project-owned classes. Initialization checks registration, ticking, a standalone game/PIE world with exactly one local PlayerController, initialized local preferences, allowed audio playback, a valid world audio device and the absence of a duplicate component on that controller. An editor preview object or headless/no-audio run is not silently assigned a substitute device.

The component captures its three class references and an `FAudioDeviceHandle` at initialization. The strong handle is documented to keep the referenced device alive while held. [S2] Class references and the opt-in mode are not hot-swapped by editing component properties during play. Relaunch after changing ownership/configuration. A normal New/Continue does not reconstruct the component or repush its mix; local preferences remain independent of campaigns.

## 5. Dedicated classes and mixing policy

Each routed class must be a distinct, independent class: no parent, child classes or passive SoundMix modifiers. Its authored volume must be finite in [0, 1] and pitch finite in (0, 4]. These conservative bounds and isolated-class requirements are original project restrictions, not the full engine's limits. The component reads those properties but never rewrites them.

This deliberately small topology avoids applying Master once to a parent and again to a child. Do not assign the engine's general-purpose Master class or connect these classes into a vendor's hierarchy to bypass the requirement. Unreal's class inheritance and passive modifiers have their own behavior; those are outside this isolated routing contract. [S3]

For each category, the requested multiplier is:

```text
category multiplier = master percentage × category percentage / 10000
```

Master 50 and Ambience 20 yields 0.10; Master 50 and Effects 100 yields 0.50. There is no separate parent override. Percentages describe a **linear multiplier**, not decibels, measured sound pressure, perceived loudness or a guarantee of half-as-loud perception. Authored source levels, attenuation, effects and any other deliberately integrated mixing still affect the result.

The component allocates one transient `USoundMix`, disables its EQ, sets negative duration, zero initial delay and zero startup/teardown fade, and fills the three `FSoundClassAdjuster` entries with the actual loaded values **before** pushing it. Pitch multiplier is 1 and child application is false. This avoids deliberately fading in from a full-volume default when a saved profile requests mute. [S4] Start actual host sounds after preference/mix initialization; this component cannot undo sound already played by an earlier BeginPlay callback.

On an explicit successful preference Apply, changed effective gains are submitted with a 0.1-second interpolation. That timing is original tuning, not an audio latency measurement. The component uses the documented class-override API and never repushes the mix merely to update a value. [S1, S5] Unchanged values, or channel changes masked by Master=0, do not cause redundant effective-gain commands. Masked channel preferences are still retained for later unmute.

The native push/override/pop entry points return void. `InitializeOptions` or `ApplyAudioOptions` returning true means the expected commands were submitted, not that the audio thread acknowledged them, a speaker produced sound, or a specific cue was routed correctly. There is no fabricated output read-back. The component checks binding loss while ticking but does not keep resending volume commands every frame.

## 6. Drafts, Apply, failures and mute

The existing settings workflow is retained. Editing is a draft; Back discards unapplied changes. Audio does not preview while adjusting a row. Defaults changes only the fields whose consumers are available. Apply for this session changes live preferences without disk IO. Apply and save changes live values only after the existing exact preference read-back succeeds. A failed write therefore does not apply the proposed new audio multipliers.

The UI rechecks availability before accepting a draft whose audio fields changed. Loss of the binding blocks that changed draft instead of saving an option that is known to have no consumer. A rare failure after preference application is separately reported as an audio command/binding failure: preferences may already be saved; the message does not falsely claim the file operation failed or was rolled back. Audio-thread execution has no synchronous acknowledgement and is outside the preference-file transaction.

Zero requests an actual zero scalar after the configured interpolation, not an arbitrary small nonzero gain. Master mute preserves individual category choices. Restoring Master restores those selected category multipliers. The component does not spawn, stop, replay or seek a sound. Whether a silent loop continues or resumes depends on the host's real sound virtualization/concurrency setup. Epic documents `PlayWhenSilent` as continuing a voice while silent, with resource implications; configure it deliberately for suitable owned loops rather than assuming every source resumes automatically. [S6]

**Radio volume and playback are never mission authority.** The retained full transcript and explicit Continue acknowledgement are unchanged. There is no `OnAudioFinished` connection to mission completion. A player can complete First Signal with all sound muted, no voice file, or no audio-options component. Full in-engine confirmation remains an acceptance scenario, not a claim that it was executed here.

## 7. Lifecycle and boundaries

The audio component does not take input locks, change pause/input mode, install mappings, write campaign files, consume inventory, acknowledge transcripts or choose interaction targets. Pausing does not invalidate its mix. Settings apply can submit audio commands from the existing paused menu; actual source playback during pause stays the host's policy.

If the fixed controller/device/class binding becomes invalid, the running component disables itself, emits a diagnostic, and releases its owned mix. Its strong device handle lets cleanup target the original device even when the world points at another one. It never silently rebinds to the replacement. Ordinary release, unregistration and EndPlay are idempotent: at most one matching pop for its push. The mix remains referenced by the component through cleanup rather than intentionally discarding that reference before collection.

Cleanup removes this owner's contribution; the host's underlying authored mix remains. This can restore underlying volume, including after a prior mute, so teardown is not a promise to keep controlling audio after the feature has been removed. Do not remove an initialized volume owner as a gameplay mute operation. Keep it ticking; call its supported release before deliberately disabling its lifetime.

No global mix clear, base mix replacement, sound-class asset mutation or unrelated modifier removal is performed. Another independent mix can still affect the same classes. The component does not identify or arbitrate such competing writers and does not claim exclusive control merely because its own transient mix is unique. Use one volume owner and dedicated classes, and test with one PIE/game profile writer. Cross-world shared-device behavior and simultaneous editor instances are not certified.

## 8. Preference schema 3

The slot names remain `CoastalLocalOptions_v1_A` and `CoastalLocalOptions_v1_B`, user index 0. The `v1` suffix remains a namespace label; it is not the payload version. These slots remain outside the campaign catalogue and contain no inventory, mission, map, transform or receipt data.

| Source format | Payload / complete envelope | Read behavior |
|---|---|---|
| M1.6 schema 1 | 40 / 60 bytes | Preserve five values; default Sprint to Hold and all volumes to 100%. |
| M1.7 schema 2 | 44 / 64 bytes | Preserve all six values, including Toggle; default all volumes to 100%. |
| M1.8 schema 3 | 60 / 80 bytes | Preserve all ten values. |

The first six field positions, identifier and generation encoding remain unchanged. Four unsigned 32-bit percentages are appended at payload offsets 44, 48, 52 and 56. Each supported schema must have its exact length. Unsigned upper limits are checked before narrowing; the percentage step constraints are validated afterward. CRC-valid out-of-range or malformed data still fails, and failed decoding clears the output record.

Old records load without being rewritten. The next explicit verified save writes schema 3 to the inactive slot with generation+1. The other valid slot is retained by that operation, but subsequent saves alternate normally; it is not a permanent legacy archive. Session-only changes never upgrade disk. Back up **both** preferences before upgrading. M1.6 and M1.7 readers cannot read schema 3; deliberate downgrade requires the backed-up old pair restored outside play, not deletion of unrelated campaigns.

The original highest-unambiguous-generation, damaged-slot recovery, equal-generation refusal, unsupported/read-error blocking, observed-file checks and failed-write lock remain. A failed write may still leave valid new bytes on disk; relaunch can correctly load them. CRC and exact read-back detect accidental corruption, not authentication, an operating-system lock or proven crash-safe atomicity. The inherited raw platform read allocates its returned file before bounded inner decoding. Use disposable backups for fault injection.

## 9. Verification and provenance

The actual supplied M1.7 encoder was compiled before edits to create a frozen schema-2 record at generation 29 with values 175/225/95/140, invert=true, Toggle=true. Bytes and source/output hashes are in `tests/fixtures/m1_7_options_fixture.json`; the matching header is used in tests. This is not the new encoder round-tripping itself and being called an old-format test. The retained actual-M1.6 fixture is also exercised.

New suites cover scalar bounds/mute/channel isolation, isolated class metadata, command-lifecycle idempotence, both older preference formats, exact lengths, explicit upgrade in either slot, failed/read-error writes, external changes and preserved category settings after mute. The retained codec's corrupt-bit and truncation coverage automatically expands to the 16 added bytes. Two retained six-row cycling expectations were updated to use the new last row; an initial test run caught those stale expectations and its output is preserved.

All new and retained standalone suites run against the exact shared headers used by native source, not a mock Unreal or AGIS. See [current validation](../VALIDATION_REPORT.md) for executed counts and logs. Four additional native tests bring the supplied Unreal inventory to 30; all remain unrun here. Follow [local audio wiring](M1_8_AUDIO_WIRING.md) and the [36-case ledger](../data/m1_8_audio_acceptance.json), then the retained full campaign gates. Only the real launched Windows test-room campaign loop passes M1.

## References

The foundation and M1.7 source establish the product and unfinished work. New topology, settings defaults, lifecycle and schema policy above are original implementation choices. Official API documentation checked September 6, 2026 supports engine entry-point meaning, not compatibility certification or measured results.

- [S1 — Epic FAudioDevice](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FAudioDevice): push, override and pop signatures.
- [S2 — Epic FAudioDeviceHandle](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FAudioDeviceHandle): strong handle, device identity and reset.
- [S3 — Epic USoundClass](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/USoundClass) and [Sound Classes](https://dev.epicgames.com/documentation/unreal-engine/sound-classes-in-unreal-engine): class metadata, inheritance and passive modifiers.
- [S4 — Epic USoundMix](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/USoundMix) and [FSoundClassAdjuster](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FSoundClassAdjuster): mix/adjuster fields.
- [S5 — Epic SetSoundMixClassOverride](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UGameplayStatics/SetSoundMixClassOverride): scalar, interpolation and child-application meaning.
- [S6 — Epic EVirtualizationMode](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/EVirtualizationMode): silent playback/voice behavior.

Pages currently display UE 5.8. This package does not pin or certify that engine or the user's asset compatibility.
