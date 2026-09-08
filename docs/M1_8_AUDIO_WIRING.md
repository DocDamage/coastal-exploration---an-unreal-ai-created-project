# M1.8 — Local Audio Routing and Volume Options Wiring

## 1. Install without discarding host work

Back up/commit the actual host project, real AGIS adapter, campaign saves and both local preference files. Replace only the complete `Plugins/CoastalFoundation` directory from this package. Keep paid content, input assets, host configuration and any project-owned vendor wrappers. Rebuild the actual Development Editor / Win64 target against your locally verified engine/toolchain. An old M1.7 DLL cannot provide the new reflected audio component.

No host `.uproject`, `.umap`, sound asset, purchased source, compiled DLL or executable is included. Do not import the entire ZIP into Content or overwrite the host configuration with a generic template. Existing [startup](M1_3_STARTUP_WIRING.md), [interaction](M1_4_INTERACTION_WIRING.md), [recovery](M1_5_RECOVERY_WIRING.md), [look/options](M1_6_OPTIONS_WIRING.md) and [sprint](M1_7_SPRINT_WIRING.md) requirements still apply. Their historical schema-1/2 new-write sizes are superseded by schema 3 in this increment.

## 2. Prepare three actual sound classes

In your project-owned Content area, create or identify three dedicated Unreal Sound Class assets for **Ambience**, **Effects** and **Radio**. These names describe roles; no exact package path is assumed or loaded by the plugin. Record their actual paths in your local evidence copy rather than marking the shipped unresolved specification as a tested asset register.

Keep these classes independent: no parents, children or passive SoundMix modifiers. They must be three distinct assets. Do not use an engine-wide Master class, make Radio a child of Effects, or attach these to a vendor demo hierarchy. The source intentionally rejects those topologies. Authored class Volume must be finite in [0,1] and Pitch finite in (0,4]; 1/1 is a simple neutral setup. Source asset volume/attenuation still sets the game's actual mix balance.

Route every intended game sound to the appropriate class using its **actual installed sound asset/component API**. For example, owned shoreline/wind/room-tone sounds belong in Ambience, existing footstep/door/pickup sounds in Effects, and a real transmission source in Radio. These examples are classifications, not a claim that those sound files or playback systems exist in this delivery. Inspect Sound Cue, Sound Wave, MetaSound or component-level overrides in the real host; a correct-looking class asset does not prove a playing source uses it.

Master affects these three categories only. Any UI sound added later must be intentionally routed, for example to Effects, or it will not necessarily follow Master. No operating-system audio, editor preview sound or unrelated vendor class is controlled. Do not edit paid pack defaults blindly; prefer project-owned sound wrappers/assets and keep provenance.

## 3. Put the optional owner on the PlayerController

Add exactly one `CoastalAudioOptionsComponent` to the same local PlayerController that owns `CoastalUISessionComponent`. Do not add it to the character or director. Set these properties before play:

| Property | Required local value |
|---|---|
| `AmbienceClass` | Your actual dedicated ambience Sound Class asset. |
| `EffectsClass` | Your actual dedicated effects Sound Class asset. |
| `RadioClass` | Your actual dedicated radio Sound Class asset. |
| `bUseDedicatedSoundClasses` | True only after the actual sound routing is configured. Default is false. |

A missing component is supported. With no owner or incomplete configuration, the four volume controls remain unavailable while the rest of the available options work. Duplicate components are not partially initialized. The owner requires a registered/ticking component, a standalone game/PIE world with one local PlayerController, initialized options and a valid world audio device.

`IsAudioReady()` means those native command prerequisites currently hold. It does not certify actual source routing, a functioning speaker, a completed mix update or a listening test. Read `LastDetail` and the UI/log diagnostics when unavailable. Do not make this return unconditional true to hide a missing device.

## 4. Keep the original startup call

After real AGIS readiness, possession and valid dry placement, retain the single existing call:

```text
Bootstrap.StartTestRoom(ActualDirector, ActualAGISAdapter, ValidatedInitialDryCheckpoint)
```

The native UI initializes its one preference object and discovers/initializes the audio owner automatically. Do not add another audio initialization or preference load graph. The component references are captured for that initialized lifetime; editing the properties during play does not reconnect them. Stop/relaunch after a configuration change.

**Start real game sounds after local options/mix initialization.** The mix is seeded with the loaded values before its native push and has zero startup fade, but it cannot silence sound that another actor already played earlier in BeginPlay. Do not add an arbitrary delay and call that ordering verified: connect the host's real readiness flow, then test a persisted Master=0 relaunch for an audible burst.

Keep the volume owner for the UI/controller lifetime. New, Continue, pause and safe return do not require repushing a mix or reloading preferences. Do not add per-tick calls that repeatedly activate it. Other systems must not replace or clear global sound mixes to update one category.

## 5. Use the existing options screen

Session/Pause → Player options now has ten fields. The original six stay first; Master, Ambience, Effects and Radio follow. Previous/Next selects a field and Decrease/Increase uses five-percent steps. There is no separate audio widget or second focus owner. Existing keyboard/D-pad/confirm/back rules and text scaling apply.

Edits are drafts and do not change sound while moving the selection. **Back** discards unapplied changes. **Defaults** changes only available fields in the draft. **Apply for this session only** updates live values and submits changed effective gains without writing. **Apply and save local options** updates live values and submits audio changes only after exact preference read-back succeeds.

Master and category multiply once: 50% × 20% means a 0.10 linear multiplier on that class. This is not a decibel scale or perceived-loudness percentage. Each explicit update interpolates over the original 0.1-second setting; source playback and audio-thread scheduling must be observed locally. Zero is an actual mute request, not a near-zero workaround.

A failed/unverified preference write retains the previous live values and does not issue the new audio values. An observed lost audio binding blocks application of changed audio draft fields. A binding failure after a successful preference operation is reported separately; saved preferences are not falsely reported as rolled back. Reopen options to review available controls and relaunch after fixing a lost binding.

## 6. Mute, loops and the radio

Set Radio=0 and play the complete mission. The transcript must still show its full text and explicit Continue must finish the message. Audio completion events must not call mission completion or acknowledge a token. The new component creates no playback system and cannot verify the host has not added an unsafe duplicate callback; inspect your actual graphs.

Master mute retains category values. Unmuting Master restores those values rather than resetting them. For ambience loops or a finite transmission that should continue silently, inspect the real virtualization/concurrency policy. `PlayWhenSilent` is one engine-supported option for appropriate sources, but it continues using a voice. The source does not rewrite those asset settings or respawn a stopped sound on unmute. Record whether each actual sound resumes, continues or restarts and choose the intentional result.

Paused audio behavior belongs to the host's actual source policy. Applying options from a paused menu can submit mix commands without changing the UI's pause/input ownership; it does not force all world audio to continue under pause.

## 7. Preferences, upgrade and rollback

The two user-index-0 slot names remain:

```text
CoastalLocalOptions_v1_A
CoastalLocalOptions_v1_B
```

Schema 1 (M1.6) loads five values with Hold sprint and 100% volumes. Schema 2 (M1.7) loads all six values, preserving Toggle, with 100% volumes. Loading alone writes nothing. The next explicit verified save writes schema 3: a 60-byte payload inside the retained 20-byte envelope, 80 bytes total. It uses the inactive slot and increments the selected generation; the other slot stays unchanged by that write.

Old M1.6/M1.7 readers reject schema 3. Back up both preference files before installing. For deliberate downgrade, close every game/editor instance and restore the backed-up old pair. Do not remove campaign saves to fix local options. The same slot namespace is retained so old preferences are not silently lost or hidden by a rename.

Run a single writer per profile. Equal generations, future schemas, unreadable files and ambiguous pairs do not authorize automatic replacement. Failed writes block further writes in that preference-owner lifetime, though explicit session-only apply remains usable. A write may have reached disk despite its failure return; relaunch may load that new valid generation. CRC/read-back is not an OS lock or crash-atomicity proof. The existing envelope inspection tool verifies the envelope only, not a full preference/campaign payload.

## 8. Ownership loss and teardown

The component retains the exact audio device handle used at initialization. If the world's device identity changes, it disables itself and pops only its own mix from the **original** device. It does not automatically take over the new device. Class/controller binding loss is similarly a feature failure, not a campaign reset. Correct the host and relaunch.

Release, unregistration and EndPlay are guarded against an extra pop. The owner never clears all mixes, changes base mixing, removes another modifier or rewrites class asset properties. Removing its contribution exposes the host's underlying mix, which may be louder than a previously muted contribution; do not remove the owner to implement gameplay mute. Use the actual volume setting instead. Release it before deliberately disabling its lifetime.

Other independent mixes affecting the same classes may combine with this one. Such competing same-class volume ownership is not automatically detected. Use dedicated classes and one owner. A separate unrelated mix must continue to work and remain installed through this owner's shutdown. Shared-device/multiple-PIE edge behavior must be checked locally; no guarantee is inferred from a source-level handle test.

## 9. Build and execute the real gate

Run all **30 supplied Unreal automation tests**: retained `Coastal.FirstSignal`, `Coastal.M1`, `Coastal.M1UI`, `Coastal.M1Startup`, `Coastal.M1Interaction`, `Coastal.M1Recovery`, `Coastal.M1Options`, `Coastal.M1Sprint`, plus the four `Coastal.M1AudioOptions` tests. All are unrun in this delivery. The new native tests check fail-closed entry points and shared rules; they do not play sound, initialize real user preferences or validate AGIS.

Execute all [36 audio acceptance cases](../data/m1_8_audio_acceptance.json), then retained campaign/UI/startup/interaction/recovery/look/sprint gates. Test each real category in isolation, muted startup, silent-loop behavior, readable muted radio, both input devices, all ten fields at 720p/1080p and 100–150% text, slot faults and cleanup. Use disposable backed-up files, not valuable player saves, for corruption tests.

Package Windows and launch outside the editor. Complete New → pickup → storage round-trip → repair → transcript → safe return → verified campaign save → exit/relaunch → Continue, with audio preferences restored separately. Record actual engine/toolchain/vendor versions, class/sound paths, build/runtime logs, real audio observations and screenshots in a separate local evidence copy. Source/helper checks do not pass this gate.

**M1 remains unpassed until that actual packaged result exists. M2 coast assembly, final audio/character/environment art, graphics/remapping and complete accessibility remain later work.**
