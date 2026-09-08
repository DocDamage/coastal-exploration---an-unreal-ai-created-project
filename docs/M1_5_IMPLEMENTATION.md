# M1.5 — Safe Return and Dry Checkpoints

**Project:** Coastal Exploration / First Signal  
**Section date:** September 5, 2026, America/New_York  
**Version:** `0.1.5-m1-safe-return-source`  
**Delivery:** Source implementation with standalone verification. No Unreal executable, generated map or real vendor acceptance is included.

## 1. Scope and sequencing

The foundation's player section specifies a forgiving return from deep water or a fall to a validated dry checkpoint, preserving inventory and progress. Its milestone plan puts the complete presentation/recovery pass in M3. This increment deliberately brings that game-owned recovery source forward for the existing systems test room. It does **not** claim that recovery was the previously agreed next production milestone or that M1's engine/vendor/Windows gate has passed.

The supplied M1.4 gate is still unrun. The actual host project, purchased AGIS/Hyper source and Unreal toolchain are not mounted in this delivery environment. Another audit checklist cannot close that gate. This package adds actual gameplay source while preserving the requirement to validate the test room locally before assembling M2's coast.

No replacement inventory, focus scanner, swimming, damage/health, lost-item penalty, boat, combat, new destination or paid artwork is introduced. AGIS remains inventory authority; Hyper selects targets and presents prompts. The existing game bridge and native UI remain their single owners.

## 2. Source implementation

| Source | Responsibility |
|---|---|
| `CoastalPlayerRecoveryComponent` | One optional possessed-character owner; checked initialization, safety-volume sampling, checkpoint visits and lifecycle. |
| `CoastalRecoveryExecution.cpp` | Fade, bounded destination attempts, post-placement checks, stability phase, cleanup and terminal failure. |
| `CoastalRecoveryInput.cpp` | Temporary gameplay input blocker below the existing menu owner, plus held-key suppression on release. |
| `CoastalSafetyVolume` | Authored upright box for deep water, route boundaries or checkpoint regions; separate dry return marker. |
| `Core/RecoveryRules.h` | Shared phase timing, checkpoint dwell, bounded candidate order and capsule/box geometry. |
| `CoastalSaveRecovery.cpp` | Coordinator-owned relocation reservation, revalidated teleport, final checkpoint commit and queued save. |
| Existing startup/UI/placement source | Optional recovery binding, feedback, menu exclusion and hazard-aware safe placement. |
| `tools/recovery_layout.py` / existing room generator | Validated, opt-in four-volume development layout before creating a fresh map. |

These are original project class names, not invented AGIS or Hyper APIs. The complete updated plugin folder is included, not just a patch.

## 3. Runtime return sequence

A configured recovery owner samples the bound character after movement/physics, before the save coordinator's deferred tick. It checks cached safety-volume identities and current shape geometry. An active campaign and normal world-input permission are required to start a new return. A paused/modal session does not start an invisible return underneath its UI.

Entering a deep-water or out-of-bounds box, or taking the character center below the configured fall plane, begins an exclusive coordinator recovery reservation. While held, ordinary mutation, save, load and new-campaign IO cannot begin. Already queued autosaves remain pending rather than being consumed. This is a new mode of the existing operation gate, not a second save coordinator.

The bridge receives a unique blocker, invalidating prior focus and input permission. Storage/transcript contexts and queued presentation requests are cancelled. The component stops jump/movement state, takes one move/look ignore count, disables CharacterMovement during placement and installs its own temporary input-stack blocker.

The source progresses through fade-out → placement → fade-in → stable-ground observation → release. It tries the saved dry checkpoint first and the original startup fallback only when distinct. Each candidate is attempted at most once. Every attempt checks the current destination, calls the engine's checked teleport, then validates the **actual resulting transform**. It never searches random positions, teleports to world origin, consumes equipment, reconstructs inventory or respawns the pawn.

After placement, normal walking physics resumes while player input remains blocked. Ground support must remain valid and be continuously observed before releasing the reservation. A destination becoming hazardous, losing support or failing to settle terminates recovery rather than causing a repeating teleport cycle. A later deliberate walk into a hazard after a completed return is allowed; there is no arbitrary three-strikes punishment.

Successful completion commits the final dry transform to the existing checkpoint field and queues the normal full campaign save. Inventory, item instances, receipt ledger, mission, world objects and journal are not mutated by the return implementation. The subsequent save uses the original coherent snapshot/export path. The HUD says save **queued**, not saved; only the coordinator's existing verified-write notice establishes a saved generation.

## 4. Checkpoints and safety geometry

`ACoastalSafetyVolume.Kind` is `DeepWater`, `OutOfBounds` or `DryCheckpoint`. `Bounds` is an upright, yaw-rotatable box with positive dimensions and scale. Its explicit geometry queries do not depend on overlap callbacks, collision presets or overlap-event ordering. The standard upright character capsule is tested against the box, including rounded corners and end caps, rather than treating the character center as its whole body.

Checkpoint volumes contain a `ReturnPoint` scene component. Its world position is the desired **capsule center**, not the feet or skeletal mesh origin. Character destinations are unit scale. The initial fallback passed into the existing startup call remains the fallback for the current fixed map; the latest successful checkpoint is persisted by the existing campaign schema.

A checkpoint visit records only after continuous dry grounded sampling. Leaving, an ineligible sample, a menu/session change or a large frame hitch clears partial dwell. Remaining in a recorded region does not repeatedly request saves. Two overlapping checkpoint regions are ambiguous and record neither, instead of depending on actor iteration order. Hazards take precedence over checkpoint recording.

The optional editor recipe adds two checkpoint regions and two hazard proxies to the existing M1 room. It uses four non-colliding floor-marker squares as explicit development presentation, not water art. Safety actors are not `CoastalWorldObject` instances: the original seven persistent world IDs, item grants and strict mission-room manifest remain unchanged.

The native owner caches volumes at initialization. Keep this compact test map's safety actor membership fixed; do not stream, add or destroy safety actors during play. Current bounds/placement are still checked, and destroyed/invalid cached volumes fail closed. This is not a scalable streaming-world safety registry. Placement validation also inspects loaded safety volumes when called by startup/load/return/checkpoint capture; it is not an every-frame target scanner.

## 5. Timings and input behavior

The following are original source defaults, not measurements: fade-out 0.18 seconds, fade-in 0.22 seconds, continuous ground stability and checkpoint dwell 0.35 seconds, and an unsettled-return limit of 2 seconds. They live in the shared core header. JSON repeats them for consistency checking; it does not configure runtime properties. Stable samples longer than 0.25 seconds do not count as observed grounding. All timers use game time; an external pause suspends progress.

The default fall plane is Z = -1500 cm, measured at the character center. Startup requires it to sit at least 500 cm above enabled engine KillZ. That minimum is a setup guard, **not** proof against every fall velocity or hitch. Test the actual host's maximum falling speed and frame-time spikes and place a generous recovery boundary above destruction. This component cannot recover a character already destroyed by engine bounds checks, and it does not disable those checks automatically.

Move/look ignore counts alone do not prevent a separate Jump callback. Recovery therefore pushes a private Enhanced Input component at priority 90 with input blocking, below the existing Coastal menu component at 100. It defines no new actions or mappings and does not clear other contexts. Standard character gameplay must remain below that priority; nonstandard higher-priority handlers or direct Blueprint tick actions require explicit host integration. Pause/Inventory/Journal opening is rejected during active return.

On success, only the recovery input component and its own move/look/blocker ownership are released. A mapping rebuild flushes old Boolean-action state and requires held keys to be released. The existing relay additionally sees the blocker revision change, so it needs fresh focus and an intentional Interact press. Real vendor input must continue to supply actual released/down state as specified in M1.4. None of this is claimed controller-tested here.

Camera fading is optional and does not fade audio. A fade already active when the return starts is not taken over. When recovery owns a fade, the host must not concurrently start another fade on that camera manager. Owned fades are cleared on completion/failure/teardown so a terminal recovery screen is not deliberately left behind a black screen. This is not a general cinematic fade compositor.

## 6. Failure and ownership boundaries

No valid primary/fallback, lost player/session, invalidated safety geometry or unsafe/unstable arrival triggers the existing coordinator recovery-required state. Disk files are not modified by this failure path; pending writes are cancelled and future mutations/IO remain locked. The native UI displays its existing non-dismissible recovery screen and exit-without-writing path. This is an explicit failure, not a silent load, new game or claim that inventory rollback occurred.

The fixed M1 pawn lifecycle remains. An initialized recovery component must not be removed during a running campaign; explicit destruction fails the campaign closed, and active teardown does not allow a stranded relocation lock to become an ordinary save. Owned input and camera resources are cleaned up on teardown. No automatic pawn rebinding is implemented.

AGIS must still obey the existing coordinator boundary. This feature cannot stop a vendor demo or an unrelated Blueprint from mutating inventory behind that boundary, nor does a successful return prove real provider serialization or atomicity. The original integration requirements are unchanged.

## 7. Corrections to existing source

`UpdateDryCheckpoint` now requires an active campaign, uses the coordinator mutation boundary and queues a full save. Previously it updated the in-memory transform without that complete lifecycle. Its Blueprint signature is retained.

The existing load/rollback placement path now validates the actual character transform after `TeleportTo`, too. The engine may adjust a requested location to fit; validating only the requested transform was insufficient. Hazard-aware/upright placement now applies to startup, checkpoint capture, return and load. This can deliberately refuse an old unsafe position or checkpoint; it is not a save migration and has not been demonstrated by loading an old Unreal binary save here.

The debug HUD omits empty status lines, includes recovery feedback and has a taller status area. No screenshot or rendered/readability pass is claimed.

## 8. Validation and next work

See [current validation](../VALIDATION_REPORT.md), [local wiring](M1_5_RECOVERY_WIRING.md), and [the recovery acceptance ledger](../data/m1_5_recovery_acceptance.json). Standalone tests exercise the exact shared helpers, not physical Unreal collision, controller input, water or AGIS. Four new Unreal automation tests join the retained fifteen, and all nineteen remain unrun in this environment.

M1 still requires actual Unreal compilation, installed AGIS/Hyper wiring, the full coherent campaign loop and a launched Windows test-room package. Only after that gate does M2 assemble Cabin Cove, Shoreline Trail, Old Dock and Rail Overlook from the owned art. This recovery increment is not that map or a packaged game.

## References and source boundaries

The supplied foundation section 6 and M3 plan establish the recovery requirement; supplied M1.4 establishes the current source and unpassed gate. New component design, timings, geometry policy and code are original implementation choices. Epic documentation was checked for engine API meaning, not engine/plugin compatibility certification:

- [AActor::TeleportTo](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/AActor/TeleportTo): checked fitting may adjust the requested destination.
- [APlayerCameraManager::SetManualCameraFade](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/APlayerCameraManager/SetManualCameraFade): manually controlled opacity and explicit stop.
- [APlayerCameraManager](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/APlayerCameraManager): camera-fade methods/state.
- [APawn](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/APawn): possessed-pawn and movement-input lifecycle.
- [UEnhancedInputComponent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/EnhancedInput/UEnhancedInputComponent) and [FModifyContextOptions](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/EnhancedInput/FModifyContextOptions): retained input-stack/rebuild entry points.

The documentation pages may display UE 5.8. No engine version has been pinned or certified by this source package.
