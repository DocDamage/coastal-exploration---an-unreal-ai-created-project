# M1.7 — Walk/Sprint Controls and Preference Upgrade

**Project:** Coastal Exploration / First Signal  
**Section date:** September 6, 2026, America/New_York  
**Plugin:** `0.1.7-m1-sprint-source`  
**Delivery:** Original source implementation and standalone verification; no native Unreal or Windows playable build.

## 1. Scope and source basis

The supplied foundation section 6 establishes normal movement at an initial 330 cm/s, sprint at 520 cm/s, no stamina drain, and a Hold/Toggle choice. M1.6 explicitly leaves sprint mode unfinished. This increment implements that bounded player-control feature against the actual supplied M1.6 source ZIP. It advances an outstanding input/presentation requirement into the existing systems room; it is not evidence that M1 has passed or permission to claim M2's coast.

There is still no supplied host `.uproject`, purchased AGIS/Hyper API or mounted Unreal installation. Real integration and the packaged campaign gate cannot be established by another standalone suite. The source remains designed for the fixed third-person single-player character. No stamina, auto-run, combat, crouch mechanic, swimming, boats or alternative inventory is introduced.

## 2. Delivered source

| Path under the plugin module | Responsibility |
|---|---|
| `Public/Core/SprintRules.h` | Hold/Toggle state, input freshness, transition rearming and exclusive speed-property ownership, shared by native code and standalone tests. |
| `Public/CoastalSprintComponent.h` / `Private/CoastalSprintComponent.cpp` | Optional real-character consumer, checked opt-in, native speed application, input sampling, lifecycle and diagnostics. |
| `Public/Core/PlayerOptionsRules.h` | Sixth field: Sprint mode, default Hold. Existing five fields retain their semantics and defaults. |
| `Public/Core/OptionsPersistence.h` | Current schema-2 encoding and strict schema-1/schema-2 reading. |
| `Public/Core/OptionsSession.h` | Retained explicit verified-save protocol; added read-only legacy-record metadata. |
| `Private/CoastalLocalOptions.cpp` | Actual platform wrapper remains; legacy and upgrade notices added. |
| `Private/CoastalUIOptions.cpp` / session source | Optional consumer initialization, sixth settings row, availability, draft/default/apply/save and teardown. |
| `Private/Tests/M1SprintAutomationTests.cpp` | Four supplied but unrun Unreal automation tests. |

No campaign serialization, mission rule, receipt, world/item ID, provider mutation, recovery component or Hyper focus source is changed. This is a source-delta statement, not proof an old binary campaign save loads under a new engine build.

## 3. One speed owner, not another movement framework

Add zero or one `CoastalSprintComponent` to the actual possessed `ACharacter`. The native UI's existing local-options initialization discovers and initializes a present component. The bootstrap signature/order stays unchanged. Absence is supported: old host movement still owns its own speed, and the new setting is disabled rather than pretending to work.

The component defaults `bUseHostSprintEvents=false`. A true property means the host intentionally routes real Sprint state and relinquishes other writes to `CharacterMovement.MaxWalkSpeed`; it is not proof that a Blueprint binding exists. Readiness checks require the registered/ticking fixed character, local controller, actual movement component, initialized options and configured bridge in a standalone world. Duplicates and incompatible ownership leave the option unavailable without poisoning a healthy campaign.

Only `MaxWalkSpeed` is owned. Existing movement vectors, acceleration, braking, rotation, jump/gravity, animation and camera stay with the host. No movement vector is injected and no input context or action asset is created. `MaxWalkSpeedCrouched` and other mode speeds are not modified. Epic documents that `MaxWalkSpeed` also limits lateral falling speed [S1]; returning that cap to walk during a jump can therefore affect the host's air-control/deceleration behavior. This is deliberate bounded behavior to test, not a claim that only ground physics can change.

Default caps are 330 and 520 cm/s. Optional authored tuning requires finite walk in [100, 1000], sprint in [walk, 1500], and a nonnegative actual `MinAnalogWalkSpeed` no larger than walk. Bounds are original safeguards, not gameplay calibration. Values are captured at initialization; editing component properties during play does not retune the lease. No UI speed slider is added.

A speed cap is not a measured actual speed. Analog magnitude, collisions, acceleration, floor slope and root motion/custom movement can change actual behavior. This implementation targets the standard fixed-character host, not arbitrary locomotion frameworks.

## 4. Hold and Toggle behavior

**Hold:** after a real released sample on a later eligible frame, pressing Sprint selects the sprint cap. Holding does not retrigger anything; release returns the cap to walk.

**Toggle:** an intentional press selects sprint; release keeps that selection; the next intentional press selects walk. The selection persists while stationary only while current host samples remain fresh. It never supplies a direction or forces the character to keep moving.

The request cancels when an observed permission/session/input-mode transition occurs: native menus, pause, campaign epoch change, bridge input revision change, independent move-input lock, leaving normal `MOVE_Walking`, crouch, Hold/Toggle change or expired input samples. Recovery already changes bridge permission and movement state, so the same gate applies. It does not bypass the recovery component or reset its locks.

After cancellation, no input is accepted on the enabling frame. A **real released state on a later eligible frame**, followed by an intentional press on a later frame, is required. Holding through menus, loading or returning from a hazard cannot re-enable sprint by itself. Release while ineligible does not count as release after returning; the host must forward actual current state again.

A jump is canceled when the non-walking state is observed. The component does not zero velocity or intercept Jump. Depending on native movement timing, a jump can begin before its falling mode is seen by the next Sprint update. Airborne inertia and braking remain native behavior requiring actual tests; no same-instant zero-speed guarantee is claimed.

## 5. Actual input and freshness

The project-owned method is:

```text
Sprint.SubmitSprintInput(ActualAggregateSprintActionIsDown)
```

Forward one actual logical down/released sample per local frame after actual host input evaluation. Aggregate all bindings: releasing Shift while left-stick click remains held is not release. Design defaults remain Left Shift and left-stick click from `data/input_map.json`; this package does not install them.

Do not synthesize false after `Triggered`, on `Canceled`, or on menu close. Context cancellation is not evidence the user released the physical input. Do not simply cache true from `Started` forever. A host that only forwards pressed callbacks has not implemented this contract. The existing input owner must continue supplying true current state, including neutral/released state after returning to gameplay.

A sample is valid on its frame and the following two engine frames. Missing samples cancel the request and require rearming. This gives bounded tolerance for host ordering, not a two-frame input queue or wall-clock guarantee. At low FPS the time span is longer. A short pulse between sampled frames may not be observed; it is never replayed later. One sample per frame is accepted; repeated bindings in the same frame cannot flip Toggle twice.

The native tick runs in PrePhysics, follows the controller actor tick and is a prerequisite for the movement component tick [S2]. Host sampling in another object/tick group must be integrated with that ordering. The component also checks permission immediately when accepting a sample and before applying speed.

The component must keep ticking. If the host disables both its tick and all submissions while a speed is active, unexecuted code cannot expire that speed. Call `ReleaseOptions`/remove the component through its supported lifetime before disabling it; do not claim heartbeat safety for a non-running consumer. External state that changes and changes back between observations, without a bridge revision/epoch change, is similarly not detectable.

## 6. Speed ownership and failure

Initialization captures the original `MaxWalkSpeed`. Every write verifies that the current value still equals this owner's last applied value within a 0.0001 tolerance. A distinct foreign write stops this sprint owner, preserves that value and produces a development log/HUD/options diagnostic. There is no automatic reacquire or per-frame fight against another locomotion owner.

On ordinary teardown, the original speed is restored only if the component still owns its last applied value. Tick prerequisites are removed, and references are released. A lost fixed pawn/movement/controller binding ends the feature rather than silently rebinding. Native UI and campaign lifecycle still enforce their existing stronger failure rules independently.

These checks do not identify a second writer assigning the exact same value or changing and restoring it between samples. They are a bounded ownership check, not a universal movement arbitration system. Run one speed owner; do not rely on conflict detection to reconcile incompatible character frameworks.

## 7. Existing options, sixth field

Sprint mode is a child-screen option alongside mouse sensitivity, stick sensitivity, FOV, text size and invert-Y. Existing ticket/frame/epoch/focus rules still dispatch every command. Previous/Next cycles six fields. The command changes Hold/Toggle in the draft, not immediately in the world.

Back discards unapplied edits. Defaults changes only available fields. Apply for this session changes live preferences without IO. Apply and save changes live values only after exact read-back confirmation. Missing/lost sprint consumers disable the field and prevent applying a changed sprint mode from an old draft. Unrelated text and compatible camera/look options remain available.

The current mode may persist. The transient **sprint request does not**: no held key, toggle latch, input frame, movement cap or speed lease enters preferences or a campaign snapshot. New/Continue retains local preferences but resets active intent through the existing session epoch. No additional preference manager is created.

## 8. Schema-1 read, explicit schema-2 upgrade

The slot namespace remains:

```text
CoastalLocalOptions_v1_A
CoastalLocalOptions_v1_B
```

The `v1` suffix is now a retained namespace label, not a claim that every payload is schema 1. Renaming slots would hide existing preferences, so the new reader accepts both schemas in the same pair. Campaign catalogue names and campaign schema do not change.

| Format | Payload | Encoded file | Behavior |
|---|---:|---:|---|
| M1.6 schema 1 | 40 bytes | 60 bytes | Five original fields load exactly; Sprint defaults Hold. No write on load. |
| M1.7 schema 2 | 44 bytes | 64 bytes | Sixth unsigned 32-bit field at payload offset 40 encodes Hold=0, Toggle=1. |

The outer retained CRC32 envelope, magic, generation and first five field positions are unchanged. A supported schema must have its **exact** size. Invalid Boolean encodings, truncated v2, v1 with appended data, unsupported schemas and invalid original fields are rejected. Failed decoding clears the output record.

The existing slot-selection policy remains: highest unambiguous valid generation; explicit damaged-slot recovery; refuse equal generations, unsupported formats and read errors; no silent replacement of unselectable files. A legacy maximum-generation record can load but cannot be upgraded by incrementing beyond the limit.

Loading legacy data does not rewrite it. The next explicit Apply and save writes **schema 2** to the inactive slot, generation+1, then verifies exact bytes and fields. Only verification changes live preferences. The previous valid legacy slot remains untouched by that write. A session-only change never upgrades disk. A failed/ambiguous write blocks further writes for the lifetime; relaunch may legitimately find a new valid schema-2 generation if bytes reached disk despite failure reporting.

**Downgrade boundary:** the original M1.6 reader rejects schema 2. With either upgraded slot present it refuses safe automatic selection rather than silently clobbering it. Back up both preferences before upgrading; do not run an older plugin against the upgraded pair. Deliberate rollback uses the backed-up old pair outside play, without removing unrelated campaigns. Forward read compatibility is tested with an actual old-encoder fixture; native platform IO and old binary game compatibility remain untested.

CRC and verified read-back detect accidental corruption; they are not authentication, cross-process exclusion or proof of crash-safe storage atomicity. Run one game/editor writer per profile. The inherited raw read can allocate a full file before bounded decoding. No new security guarantee is claimed.

## 9. Verification and local gate

Two new standalone suites exercise the shared sprint/speed rules and preference upgrade protocol. An exact golden fixture was generated by compiling the actual supplied M1.6 encoder, not by round-tripping the new encoder and assuming compatibility. Its bytes, generation, original values, source archive hash and output hash are recorded in `tests/fixtures/m1_6_options_fixture.json`.

The retained codec tests now traverse four additional encoded bytes, and their unsupported-version sentinel is 3 because version 2 is intentionally supported. All retained mission, campaign, envelope, UI, interaction, startup, recovery and option suites are rerun. Counts and execution details are in [current validation](../VALIDATION_REPORT.md).

Four new `Coastal.M1Sprint` Unreal tests cover unbound rejection and native compilation entry points for shared transition, ownership and codec rules. Together 26 engine tests are supplied; none ran here. These tests do not simulate real AGIS, render UI, measure a character moving, or call platform preferences in this environment.

Follow [local wiring](M1_7_SPRINT_WIRING.md), run the [35-case acceptance ledger](../data/m1_7_sprint_acceptance.json) and all retained gates, and package the real test room. Only actual pickup -> storage -> repair -> transcript -> safe return -> verified save -> relaunch -> Continue in the launched Windows package passes M1. M2 remains Cabin Cove, Shoreline Trail, Old Dock and optional Rail Overlook after that gate.

## References and boundaries

Design values and required Hold/Toggle behavior derive from the supplied [foundation](COASTAL_EXPLORATION_FOUNDATION.md), section 6. M1.6 supplies the actual code basis and unfinished-feature boundary. The new mode policy, lease, lifecycle and preference upgrade are original project implementation choices.

- [S1 — Epic UCharacterMovementComponent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent): `MaxWalkSpeed`, `MinAnalogWalkSpeed` and movement-mode semantics.
- [S2 — Epic Actor Ticking](https://dev.epicgames.com/documentation/unreal-engine/actor-ticking-in-unreal-engine?lang=en-US): tick groups and prerequisites.
- [S3 — Epic AController](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/AController): move-input ignore state and ownership.

Checked September 6, 2026. Current pages may display UE 5.8; they do not certify this source, the user's asset compatibility or a selected engine version. The exact engine remains a local compatibility decision.
