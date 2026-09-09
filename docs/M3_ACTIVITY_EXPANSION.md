# Animation and activity expansion

The September 9 goal authorizes the complete coverage below, superseding the
original v0.1 exclusions. The existing UE5.8 host is the integration target.
Existing uncommitted creator, merchant, preview and audio work is retained.

| Requirement | Required completion evidence | Current status |
| --- | --- | --- |
| Pistol equip/hold/directional aim/fire/unequip | Accepted combat events, moving upper-body blend, hand/muzzle alignment, playback and interruption checks | Assembled pistol, left-hand/barrel binding and sustained aim pass; broader directional contact review pending |
| Front/back/left/right damage reactions | Accepted incoming damage selects a facing-relative clip; rejected/lethal damage does not play | Implemented; facing and moving playback checks pass |
| Fire/tent/sleep/wake/backpack/eat/tent setup | Actual interactions, matching clip segments and prop contacts, cancellation | Pending |
| Locomotion/falling transitions | Compared clips, jump/fall/landing playback while preserving native movement | Pending |
| Gestures/idles | Selected clips, accessible emote input and interruption/idle variety checks | Four emotes and relaxed idle implemented; 43 live checks and four gesture captures pass |
| Crawl | Entry/idle/start/loop/stop/exit, low collision, stand clearance and input | Native implementation builds; first live test finds generated body too low after crouch; contact fix and remaining acceptance pending |
| Ladders | Supplied wooden art, top/bottom entry/exit, climb up/down/idle and collision | Wooden mesh/textures imported; gameplay pending |
| Melee | Punch/kick/combo/stance playback, timed hits, one hit per target, interruption | Pending |
| Motorcycle | Supplied rigged enduro art, driving, rider contact, mount/dismount and collision | Five rigged parts/textures imported; assembly/gameplay pending |
| Fishing | Supplied rod and fish, casting/catching gameplay, animation and prop alignment | Rod and Java barb textured/imported; gameplay pending |
| Chopping | Wood/axe props, timed interaction and animation | Pending |
| Zipline | Dynamic Rope plugin, authored endpoints, movement/animation and safe exits | Ride/contact29, obstruction/input20 and lifecycle26 pass; fresh-session cable rendering verified; packaged acceptance pending |
| Grappling | Same Dynamic Rope plugin, valid anchors, pull/swing/release and interruption | 48 checks pass including stowed-hook visibility, exact rope-eye contact and short steered swing; broader trajectories, range/cover, recovery and throw-pose acceptance pending |
| Gliding | Gameplay, prop, installed animation and safe landing | Pending |
| Speedboat | M3 ShipAndSea speedboat art, water driving, boarding/exits/collision | 39-file package copied and hash-verified; gameplay pending |

Native compile, scripted gameplay, visual/contact review and packaged acceptance
remain distinct evidence. None of the pending rows is completed by asset presence.
Purchased payloads and execution evidence stay outside Git. Save/appearance,
inventory, interaction, camera and recovery authorities must remain coherent.

## Crawl preparation

Work stopped at the user's request after the first native crawl test. The native
component, C/controller B input, crouch geometry, body sweep and phase animation
are implemented. An optional stance interface normalizes saved positions to safe
standing geometry and resets stance during validated native placement. Build
passes; native61 passes before the final input-revision guard; that final build
also passes. These checks do not establish complete crawl gameplay.

Fresh a creator smoke18 passes. The live crawl test passes10 checks, including
entry, low capsule, preserved capsule floor, idle selection and conflict gates,
then fails hand contact. The generated head is -8.54cm relative to the floor:
the mesh is too low after native crouch. `m3-prone-first-a-failed-contact.json`
records the failure. The later movement, obstruction, standing and save/reload
cases did not run. Fix generated-mesh alignment, then run those cases and shared
save/recovery, combat, camp and rope regressions before accepting this work.

`retarget_m3_prone.py` binds nine installed Free Crawl clips under `prone_*`
in the private character definition, preserving every other action binding.
Outputs are `/Game/Coastal/Character/Animations/CC_anim_*`. Root motion is
disabled and root lock enabled so native movement can own translation.
Entry/exit last 2.5s, each start 2.633s, each loop/idle 1s and each stop 1.633s.
`m3-prone-retarget.json` records exact assets and durations.

`audit_m3_prone_poses.py` samples five times per clip and checks finite bone
positions and the translation settings. `m3-prone-pose-audit.json` records the
45 sampled poses. Idle pelvis is approximately 12cm above the floor, head38cm,
hands4cm; hands/feet extend roughly 80–90cm along the body. Entry and exit pass
through a raised kneeling pose before reaching standing. Collision must cover
that full body footprint and reserve standing clearance before an exit starts.
A short upright capsule alone does not cover the prone body. These are pose
measurements, not proof of playable crawling or rendered skin contact.

Native prone ownership, input, transition timing, body sweeps, blocked standing
and start/stop selection are now present but await the live checks noted above.
Keep the current pawn/controller, sprint speed owner, native floor movement,
AGIS and save/recovery authority. Left-stick click already belongs to sprint.

## Zipline ride and obstruction checks

The final g campaign passes 29 live checks (`m3-zipline-live-final-g.json`): full
keyboard ride, right-stick boarding/release, native landing, menu cleanup and
original pawn/controller ownership. The viewed `m3-zipline-ride.png` shows the
steel cable, trolley and two-handed pose. Hand-to-bar errors are 0.17cm and
0.47cm. The continuous handle accommodates the generated body's hand spacing.
The final h campaign passes 20 obstruction/held-input checks
(`m3-zipline-safety-final-h.json`): failed boarding rolls back position, movement
and presentation; a mid-ride obstacle stops the capsule before crossing and
releases it; held release/menu keys cannot reboard. Temporary blockers are
transient PIE actors made by a bounded, disposable-campaign editor helper.

Final build and native61 pass (60 clean, one known AGIS warning), report
`m3-ue58-native-report-20260909T220740Z`. Python70 and preservation checks pass.
The i campaign also passes 26 lifecycle checks: endpoint loss/reinstatement,
appearance regeneration through the creator, campaign reload while riding,
boarding in the new session epoch and native defeat return while riding.
`m3-zipline-lifecycle-final-i.json` records the results. No owner swaps occur.
Packaged acceptance remains open.

Fresh campaign i repeats zipline29 and safety20 and passes creator18, grapple48,
combat103 and existing camp actions40. Reports use `m3-zipline-*-final-i.json`.
The grapple regression exposed Mutable presentation overriding the idle hook's
hidden state. Appearance refresh now leaves descendants of URopeComponent under
their gameplay owner's visibility control; idle and released hook checks pass.
The i ride capture also exposed an invisible cable despite valid simulation.
Rebuilding the scene proxy restored it with the same steel material. The native
line now refreshes render state once when its wrap tensions. Fresh j verifies
the cable renders with the original material and no diagnostic swaps. Its
ride29, safety20 and lifecycle26 pass; grip errors are 0.16cm and 0.48cm.
Reports are `m3-zipline-{live,safety,lifecycle}-final-j.json`. The screenshot was
viewed. Diagnostic material swaps changed only the earlier i PIE session.

`ACoastalZipline` establishes a real GuaranteedWrap cable between authored anchors;
`UCoastalZiplineRiderComponent` samples the plugin centerline and sweeps the existing
CharacterMovement capsule in custom mode 42. V / controller right stick click boards or releases.
The original trolley and retargeted Two_Handed_Zipline clip use the existing
activity blend. There is no pawn/controller replacement. Failed boarding reverts
its scoped movement; collision, menus and invalid session ownership release it.

The FirstSignal line currently runs from (12500,1700,600) to (14500,1700,490).
Its four authored actors and map backup/hash chain are recorded by
`author_m3_zipline.py` and `m3-zipline-authoring.json`. The first route was blocked
by simple rock collision; a subsequent route had a pier pile in the capsule path.
The current route passed a full capsule sweep. The terrain's unused aggregate
hull also pushed the cable beneath the map: native terrain uses complex triangle
collision, but the plugin still gathered that hull. `CoastalRopeWorldCollision`
excludes only the exact authored terrain asset with ComplexAsSimple collision
from the analytic provider; native triangle collision remains. Grappling retains
world distance fields. The fixed zipline uses analytic world colliders and the
native rider capsule: camera-dependent terrain distance fields distorted the
distant line after a ride. All-geometry exclusion, CPU forcing and reduced
gravity were disposable diagnostic probes, not production settings. The line
uses 32 particles, 32 iterations, inextensible segments and normal gravity.
Readiness rejects excessive segment stretch. Right-stick click avoids the
existing controller Y inventory binding. The original 29-check f pass is retained
as `m3-zipline-live-first-f.json`; it predates the stricter bar-contact adjustment
and fixed-line distance-field change. Existing activity regressions are recorded
separately; this section does not claim completion of the full activity goal.

## Grapple hanging and hook presentation

The contact defect described in the historical measurement below is now fixed.
The final `coastal_test_m3_grapple_0909h` run passes 47 checks, recorded in
`m3-grapple-contact-live-final-h.json`. Socket-to-node error is approximately
1.8e-12cm at commit and under reel tension. Actual D-key steering moves the
airborne character sideways while the embedded hook stays fixed. Keyboard and
controller reel in/out, throw/release and menu cleanup pass. The final close-up
`m3-grapnel-contact.png` was viewed and shows the rope meeting the eye.

The root cause was the plugin adding its 2cm surface clearance to a position
already authored at the rope-eye socket. A uniform-scale probe still failed;
nonuniform gantry scale was not the cause. The independent host now carries a
private project copy of Dynamic Rope with one source fix: successful socket
pierce placement sets the anchor's SurfaceOffset to zero. The original engine
plugin remains unchanged; all 757 original file hashes and all other private
source/content files are verified by `check_m3_dynamic_rope_contact_fix.py`.
`stage_m3_dynamic_rope_contact_fix.py` records the exact source version and patch.
The paid plugin payload remains outside Git. Loaded module paths prove the
project copy is used. The hook also uses absolute transforms so later skeletal
updates cannot drag it with the animated hand.

Build and native61 pass (`m3-ue58-native-report-20260909T201413Z`); the plugin's
Pierce9 tests pass (`m3-rope-pierce-native-20260909T201443Z`); Python70 passes.
The editor-only disposable-campaign input helper now permits WASD/Space so
movement checks use actual input. One intermediate swing check hit that helper's
old whitelist, and another needed its target camera/character fixture restored
after swinging; these failed reports are preserved. No further map change was
needed. This does not complete broad swing/range/cover/recovery/throw animation
or the separate zipline/activity requirements.

A subsequent interaction regression exposed a presentation tick-order defect:
after a long capture frame, the character was moving at 330cm/s while pickup
presentation still read the prior stationary frame. The Mutable presentation
component now ticks after native CharacterMovement. Fresh
`coastal_test_m3_grapple_0909k` passes 40 interaction checks, including held input
and all three existing camping actions, and 103 combat-animation checks.
Reports are `m3-grapple-contact-action-regression-k.json` and
`m3-grapple-contact-combat-regression-k.json`. The final native61 report is
`m3-ue58-native-report-20260909T203231Z`. Earlier input/timing failures remain
archived; game-time input measurement alone did not fix the ordering defect.
The same final k campaign reruns and passes all 47 grapple checks after that
change (`m3-grapple-contact-live-final-k.json`). Final preservation confirms all
150 original saves and all 757 engine-plugin files unchanged; no additional
map or descriptor edit was needed.

The existing animation proxy keeps the native retarget graph running beneath a
full-body activity blend and the pistol/action layers. The private definition's
`grapple_hang` binding uses retargeted `CA_Hold_Rope_Idle_Anim` while the real
plugin reports a supported airborne hang. Landing, release, menus, generation
and incompatible activities retire the pose. No movement or possession owner
is replaced. Armed combat suppresses grappling and retains its existing layers.

Source and target pose samples prove this clip grips with the right hand lower
and left hand upper. Dynamic Rope's hang regrip attaches to `middle_03_r` and
pins the upper grip to `middle_03_l`; the curled fingers place the rope inside
the fists instead of at the wrist origins. The side capture was viewed.
`audit_m3_hang_grip.py` records source/target bone samples. The original grapnel
has HookPoint (20,0,0) and RopeEye (-4.2,0,0) sockets. Plugin socket alignment
places it during a throw/wrap; it is stowed outside deployed phases.

The player-side close-up `m3-grapnel-contact.png` was viewed. An additional
numeric contact assertion exposes an unresolved 2.03cm gap between the last
simulation node and RopeEye at commit, and 2.27cm after 0.3 seconds. These failures
were retained in `m3-grapple-eye-first-frame-e.json` and
`m3-grapple-eye-settled-e.json`. The live test retains its stricter <0.5cm
assertion; the newer 47-check run above passes. The 41-pass report below predates
that assertion. The initially suspected nonuniform-scale cause was ruled out.

Fresh final compiled campaign `coastal_test_m3_grapple_0909e` passes 41 checks
in `m3-grapple-hang-live-final-e.json`, including actual gamepad input, keyboard
reel-out, full hang weight and release cleanup. The same campaign passes 103
combat-animation checks in `m3-grapple-hang-combat-regression-e.json`, including
armed grapple suppression and clearing the hang layer. Previous d campaign's
39 existing action checks also pass. Native61 (60 clean, one known AGIS warning)
passes in report `m3-ue58-native-report-20260909T195552Z`; Python70 passes.
All 150 baseline saves remain unchanged. No further map/descriptor edit occurred.

An intermediate repeated reel check stopped after horizontal movement while
still walking and then timed out waiting for a hang. The harness now keeps
reeling until airborne; that failed report is retained as
`m3-grapple-hang-grounded-d.json`. The initial wrist/regrip-order preview was
corrected from measured clip poses. Broad swing trajectories, overhang/cover,
range boundaries, recovery/session transitions, throw/stow animation and zipline
gameplay still require implementation or acceptance. This batch does not close
those requirements or the other activity rows.

## First integrated source batch

The Mutable animation proxy now has a separate spine-filtered pistol layer under
the existing full-body action slot. Its standalone sequence player supplies hold
and directional aim; accepted shots supply directional montages. Hit presentation
uses the accepted damage causer's bearing relative to the character. Audio retains
the existing accepted-hit cue. Missing/coincident damage sources default to front.
Aim uses RMB or gamepad LT and requires release after input interruption.
Weapon lifecycle observation waits for valid presentation, including after a
checkpoint save; it does not create another weapon or ammunition authority.

Fifteen owned clips are retargeted into `/Game/Coastal/Character/Animations` and
bound in the existing character definition. Existing pickup and appearance data
remain. `m3-combat-actions-retarget.json` records their exact source and duration.
The first client call timed out during Mutable compilation; the original editor
operation subsequently completed, and its saved output was verified without
restarting the mutation.

The final source build and 61 native tests pass (60 clean, one retained AGIS
warning). The disposable `coastal_test_m3_activities_0909c` passes 90 combat
animation assertions, 18 creator assertions, the existing 39 action assertions
and 16 combat assertions. Checks include real mouse/gamepad input, visible
generated-hand firing motion in all four directions, four hit bearings at two
character orientations, moving hit reactions, moving/airborne firing, native
leg motion, pause/held-aim handling and equip/unequip cleanup. Earlier harness
property/rotation failures and partial reaction observations are retained.

Standing hits use their full-body clip; moving hits use the upper layer. Accepted
combat receipts survive a slow next animation frame, but explicit lifecycle
interruption clears them. Hit-motion checks include onset, not just recovery
frames. The retained native combat checks cover rejected input, point damage,
cover, ammo/reload, time dilation, defeat recovery and campaign-epoch teardown.

Final reports are `m3-combat-animation-live-final-c.json`,
`m3-activities-creator-smoke-c.json`, `m3-activities-existing-actions-c.json`,
`m3-activities-existing-combat-c.json`, `m3-combat-animation-native-final.execution.json`
and `m3-combat-animation-build-reactions.execution.json` under `../local-evidence`.
The source checks also pass 70 Python tests and over 3,750 structural checks.

The first side-view capture exposed a wrong-hand attachment. Component-space
sampling of the source clips confirms the left arm performs the directional
aiming while the right hand remains low. The retargeted clips preserve this.
`assemble_m3_pistol.py` combines the owned body, slide, barrel and magazine into
`/Game/Coastal/Activities/Combat/SM_CoastalPistol`, preserving their materials.
`bind_m3_pistol.py` binds it to `hand_l` and places the muzzle at the barrel tip.
The forward aiming capture `m3-pistol-aligned-preview.png` was visually reviewed
and shows the assembled pistol in the raised hand pointing forward.
Fresh campaign `coastal_test_m3_pistol_0909e` passes 93 combat-animation checks,
including the actual left-hand socket, assembled mesh and barrel-tip position.
Evidence: `m3-pistol-alignment-live-e.json` and `m3-pistol-binding.json`.
The 16 existing combat assertions also pass with the relocated muzzle, including
damage/ammo, cover, reload, defeat recovery and campaign teardown. Report:
`m3-pistol-combat-regression-e.json`. The editor is outside PIE with no dirty
packages after this run. No native source changed in the pistol art follow-up.
Broad directional contact review remains open. The point clips transition into
their aim pose; the new sequence-player setting plays these once and holds the
final pose while aim remains held. The separate pistol hold loop remains looping.
The live test now checks sequence time beyond the transition length to distinguish
held aiming from a repeatedly raised arm.
`m3-pistol-bearings.json` measures final-pose barrel errors of 0.001, 1.083,
0.011 and 0.741 degrees for front/right/back/left, all within the five-degree
tolerance. This geometric check does not establish hand contact during motion.
Physical-device, broad art review and packaged acceptance remain separate.

Imported props are private under `/Game/Coastal/Activities`. The FBX importer
preserved the motorcycle's five independent skeletal parts and their skeletons;
it did not produce a single `SK_Enduro` mesh. All eight imported meshes have
source texture materials. Reports: `m3-activity-prop-import.json` and
`m3-activity-prop-materials.json`. The rod is about 241 cm high with an offset
source pivot, and the wooden ladder module is about 180 cm high. Contact and
assembly authoring must account for these measured bounds.
The Java barb is about 23 cm long. Its supplied alpha map masks the fins. Its
source thumbnail was inspected alongside the fantasy-styled Colorful Fish;
the naturalistic barb was selected for freshwater fishing content.

The preservation baseline covers 150 pre-existing saves and 12 maps/descriptors.
All 162 remained byte-identical after the first source and asset batch. Purchased
assets, backups and runtime reports remain outside the source repository.
The final editor check is outside PIE with no dirty maps or content; source sync
reports no outstanding copies. Structural validation passes 3,761 checks.

The subsequent pistol binding intentionally updates only the UE5.8 FirstSignal
map. Its original is backed up as `L_FirstSignal-before-pistol.umap`; the binding
report records exact before/after SHA256. Preservation now reports 161 unchanged
protected files and one explicitly recorded map edit, with all 150 original saves
unchanged. The preservation checker accepts only that exact new map hash with a
verified original backup, never arbitrary map or campaign changes.

## Gesture and sustained-aim batch

`CoastalCharacterGestures.cpp` adds G / D-pad down emote cycling, exposed in the
existing keyboard/gamepad HUD hints. The optional character interface supplies
the hint without a foundation dependency on the UE5.8 expansion. Four owned
SillyGesture clips use a separate UE4-to-Mutable retargeter; Standing_Idle adds
a relaxed five-second variation after 18 seconds of standing idle. No movement,
inventory or gameplay state is supplied by the clips. Movement/jump, modal UI,
camping/native montage ownership, appearance regeneration and encounters gate
or interrupt cosmetic playback. Held emote input cannot replay through a menu.
Accepted interaction/hit cues take precedence over gestures. Definition arrays
control the emote cycle and idle selection; existing action bindings remain.

Native build and 61 tests pass (60 clean, one known AGIS warning); 70 Python tests
pass. The asset script is `retarget_m3_gestures.py`; its five outputs are recorded
in `m3-gesture-retarget.json`. All four generated gesture captures were visually
reviewed. The first harness failure used wall-clock completion deadlines; the
rerun uses game time for animation durations and preserves that failed report as
`m3-gesture-live-timing-a.json`. The final disposable `coastal_test_m3_gestures_0909b`
passes all 43 gesture checks (`m3-gesture-live-final-b.json`) and all 101 combat
animation checks (`m3-aim-hold-live-b.json`), including sustained aim beyond the
transition length in all four directions. These checks use actual mapped inputs,
not just direct calls to the gesture API.
The existing 39 interaction/camp/creator assertions pass in fresh campaign
`coastal_test_m3_gestures_0909c` (`m3-gestures-action-regression-c.json`). The old
right-hand assertion was corrected to check the actual left-hand socket and
world position; its failed report remains available. No further native edits
followed the successful build and native suite.
The naturally scheduled relaxed idle was captured and visually reviewed in
`m3-idle-variation.png`; its temporary camera setting was restored. Final editor
state is outside PIE with no dirty packages, and source sync reports zero changes.
All 150 baseline saves remain unchanged; the only protected map difference is
the previously backed-up pistol binding.

## Dynamic Rope integration in progress

Dynamic Rope 1.0.1 is now an explicit expansion dependency and enabled in the
independent UE5.8 descriptor. `enable_m3_dynamic_rope.py` records its actual
installed location and backs up the descriptor. Preservation distinguishes that
exact edit from the earlier pistol map edit: 160 protected files remain unchanged,
including all 150 original campaign saves.

`UCoastalGrappleRope` uses the plugin's GuaranteedWrap contract, 25-metre reach,
native solver/rendering and wielder movement constraint. `UCoastalGrappleComponent`
inherits the plugin wielder and adds campaign, menu, recovery, appearance, camping,
swimming and encounter gates. It attaches to the generated hand, cancels across
epochs, and uses the plugin's reeling and release APIs. Q / D-pad up toggles the
grapple; Z / D-pad right reels in; X / D-pad left reels out. The existing HUD shows
these controls. Open-space throws are refused; authored anchors use the real
`URopeWrapTargetComponent` provider and `Coastal.GrappleAnchor` tag.

The wrapper and native anchor compile, and the 61-test native suite passes.
First live verification found and fixed generated-mesh cleanup deleting the rope:
the rope now moves to the native mesh before generated children are removed, then
reattaches to the replacement hand. The same native movement owner successfully
reels toward an authored gantry at `(13200, 1800, 950)`; the anchor is highlighted
orange on two support posts. Its map edit has its own backup and exact hash chain
after the pistol edit (`m3-grapple-gantry.json`). The second live check reached
successful wrapping and movement, but reported the plugin's visual Releasing
phase as still deployed. The plugin constraint was verified absent; the wrapper
now distinguishes release presentation from an active grapple. Final campaign
`coastal_test_m3_grapple_0909c` passes all 21 checks in
`m3-grapple-live-final-c.json`: generated-hand attachment after creator reload,
open-space refusal, targeting the real authored anchor, wrap, reel shortening,
native character movement toward the anchor, emote refusal while deployed, and
menu release with the plugin constraint explicitly absent. Pawn/controller remain
the same. The side capture `m3-grapple-reel.png` was viewed and shows the plugin's
braided rope and airborne native character; it also shows that dedicated hanging
presentation is still needed. Final native61 passes in
`m3-ue58-native-report-20260909T193859Z`; Python70 and 3,794 structural checks pass.
The same final campaign passes 39 existing pickup/camp/hit/creator-regeneration
assertions (`m3-grapple-action-regression-c.json`). Editor cleanup leaves 2,938
actors, outside PIE with no dirty content or maps; source sync has no pending
copies. The preservation check still confirms every baseline save unchanged.

`build_m3_grapnel_mesh.py` creates original 1,372-triangle three-prong geometry;
`import_m3_grapnel.py` imports it with steel material as
`/Game/Coastal/Activities/Rope/SM_Grapnel`. Its +X-forward bounds are verified.
Tip sockets/hand alignment and runtime tip binding remain pending, as do grapple
throw/hang animation, full swing/reel-out/gamepad/recovery coverage and ziplines.
Enabling the plugin is not acceptance of either complete rope feature. Initial
native report: `m3-ue58-native-report-20260909T192547Z`.
