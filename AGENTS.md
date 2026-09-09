## User stop checkpoint — September 9, 2026

USER SAID "stop after this" during the first native crawl test. That test finished;
work stopped afterward. Do not automatically continue the goal without a fresh
user instruction. Full objective remains unfinished; do not mark it complete.
The subsequent user request authorizes committing and pushing this source
checkpoint only. Development remains stopped. Pre-commit checks: Python70 and
3841 structural checks pass; all168 pending files are text source/scripts/docs,
with no binary payloads or credential-pattern matches. Paid assets and execution
evidence remain in the private external host/evidence folders.

Native UCoastalProneComponent now exists, C/controller B toggles, native crouch
capsule/floor handling, start/loop/stop/entry/exit timing, long-body box sweep,
standing clearance and restore tuning. Activity animation takes explicit phase
time. Character tag gates conflicting actions/interaction, emotes, grapple,
zipline and weapon spawn. New Foundation ICoastalCharacterStance interface lets
save capture normalize to a validated standing location or existing dry fallback;
restore/recovery reset stance only after a full standing destination passes.
No pawn/controller swap or competing MaxWalkSpeed writer. Native input revision
requires button release after menus. Helper whitelist now permits C/controller B.

Build PASS222143 (no deprecated crouch API); native61 PASS report222225. Final
input-revision guard build PASS222432 (not separately native-suite rerun).
Fresh coastal_test_m3_prone_0909a creator18 PASS. check_m3_prone.py FAIL after10
checks at prone hands meet ground: generated head is -8.54cm relative to floor,
despite native capsule-floor preservation passing. Thus generated body sits too
low after native crouch. No rendered crawl capture produced before failure;
body movement, cover, blocked stand, save/reload and controller-exit cases later
in the script were NOT executed. Report m3-prone-first-a-failed-contact.json.
Next task when authorized: fix generated mesh/crouch parent offset, then rerun
contact and remaining live cases, actual menu/held-input/recovery cases, and
combat/rope/camp regressions because shared placement and animation changed.
Editor PID34400 is outside PIE; no pending Python test callback. No source edits
after final222432 build except docs/test script. Full pending table remains below.

## Crawl preparation — September 9, 2026

Full goal ACTIVE; previous turn made concrete asset/evidence progress.
Nine Free_Crawl_Animation clips retargeted and bound as prone_enter/exit/idle,
prone_start_l/r, prone_loop_l/r, prone_stop_l/r. Outputs private CC_anim_* under
/Game/Coastal/Character/Animations. tools/unreal/retarget_m3_prone.py preserves
all other definition bindings (FName dictionary keys must remain FNames; action
clip uses get_editor_property). Repeated run passes. m3-prone-retarget.json.
tools/unreal/audit_m3_prone_poses.py samples45 poses, verifies finite bone data,
root-motion disabled and root-lock enabled. m3-prone-pose-audit.json. Entry/exit
2.5s, start2.633s, loop/idle1s, stop1.633s. Prone head38cm/pelvis12cm/hand4cm
from floor; long-body reach80–90cm requires supplemental body sweep in addition
to low native capsule. Entry/exit include raised kneeling poses. No crawl native
component/input/movement implemented yet; no rendered crawl acceptance claimed.
Next implement real prone state, body collision/stand clearance, transitions and
native recovery. Left-stick click is sprint; C and world B appear available.
Existing native crouch hooks may preserve floor/capsule geometry, but must audit
host crouch flags and recoveries. Sprint owns MaxWalkSpeed; do not compete with it.
Editor PID7476 remains outside PIE; source unchanged this batch. Zipline retarget
report was temporarily overwritten by the copied script; moved the new rows to
the correct prone report and reran original zipline script to restore its report.
See docs/M3_ACTIVITY_EXPANSION.md for full pending scope.

## Zipline expansion in progress — September 9, 2026

This supersedes the historical “zipline absent” notes below. Full goal ACTIVE.
Native ACoastalZipline uses real Dynamic Rope GuaranteedWrap, 32 particles,
32 iterations and inextensible segments. UCoastalZiplineRiderComponent sweeps
native CharacterMovement in custom mode42; V/right-stick click boards/releases.
Y remains inventory. Original SM_ZiplineTrolley and retargeted
CA_Two_Handed_Zipline_Anim are private under /Game/Coastal. The existing activity
blend presents the ride. No pawn/controller swaps. Scoped boarding rolls back
blocked moves. Native capsule collision, landing and menu release are retained.

The FirstSignal map has four zipline actors at start(12500,1700,600),
finish(14500,1700,490), plus supports. author_m3_zipline.py backs up the previous
gantry map and records the exact hash chain; check_m3_activity_preservation.py
accepts that chain only. The y1800 trial overlapped the dock recovery point;
y1700 passes startup. Earlier routes failed rock/pier capsule collision.
CoastalRopeWorldCollision excludes only the exact authored ComplexAsSimple terrain
asset's obsolete aggregate hull from the plugin analytic provider. Native triangle
collision remains. Grappling retains GDF; the fixed zipline uses analytic world
collision because camera-dependent GDF distorted it after riding. Diagnostic
all-collider exclusion, CPU forcing and reduced gravity are not production settings.

Current final source build and native61 PASS (report 20260909T220740Z, 60 clean,
one known AGIS warning); Python70 passes. Fresh i passes 29 zipline checks:
full ride, keyboard/controller release, native landing, menu cleanup and
hand-to-bar error0.26/0.71cm (g was0.17/0.47cm). m3-zipline-live-final-i.json; viewed g capture shows
cable, trolley and two-handed contact. First f pass predates the tighter contact
and fixed-line GDF changes; post-ride distortion failure is archived separately.
Fresh i also passes creator18, grapple48, combat103, actions40 and zipline
safety20. Reports m3-zipline-{grapple,combat,actions}-regression-final-i.json,
m3-zipline-safety-final-i.json. Mutable RefreshPresentation now skips children
under a URopeComponent so appearance updates cannot unhide the stowed hook.
The prior h failure is retained; new idle-hook assertion passes in i.
Fresh j passes creator18, ride29, safety20 and lifecycle26. Lifecycle covers
missing/restored endpoint, creator regeneration, campaign reload during a ride,
new session epoch boarding and native defeat recovery during a ride. Reports
m3-zipline-{creator,live,safety,lifecycle}-final-j.json. The initial i screenshot
had an invisible cable despite valid simulation. Rebuilding its render state
restored the same steel material. ACoastalZipline now MarkRenderStateDirty once
when Wrapped tension commits. Fresh j (no diagnostic swaps) visibly renders the
cable; screenshot viewed, grip errors0.16/0.48cm. Source build220724 and native61
220740 pass. Editor PID7476 is outside PIE after j; no pending test callbacks.
Packaged acceptance and the remaining full activity table still apply.
Safety uses the new editor-only SpawnZiplineTestObstacle helper, restricted to
standalone LocalHost58 coastal_test_ campaigns and a bounded dock region.
The creator smoke now waits for saves.is_configured() before starting the campaign.
Do not claim full zipline or overall activity completion yet. See the full
requirement table in docs/M3_ACTIVITY_EXPANSION.md; every pending row still applies.

## Full animation/activity goal — September 9, 2026

Latest contact-fix batch supersedes the gap warning below: 47 live grapple
checks PASS in fresh `coastal_test_m3_grapple_0909h`, report
`m3-grapple-contact-live-final-h.json`. Rope-eye error is ~1.8e-12cm at commit
and under tension. Actual D-key steering moves the suspended pawn laterally;
the embedded hook stays fixed. Player-side contact screenshot was viewed.
Root cause was the plugin adding RopeRadius (2cm) as SurfaceOffset after
computing the socket-tail position. Uniform anchor scale did NOT solve it.
The private host now has Plugins/DynamicRope, copied from the paid installed
plugin excluding Binaries/Intermediate. Only its socket-pierce commit resets
SurfaceOffset to zero. Engine installation is unchanged (all 757 file hashes).
Use tools/stage_m3_dynamic_rope_contact_fix.py and
tools/check_m3_dynamic_rope_contact_fix.py; do not copy paid payloads into Git.
The grapnel component now uses absolute transforms to prevent late hand updates
dragging an embedded hook. Editor-only input helper permits WASD/Space only in
the guarded disposable campaign. Final build passes; native61 report ends
201413Z, plugin Pierce9 report ends 201443Z, Python70 passes. Full goal ACTIVE;
zipline and the other activity rows remain required.
Follow-up in the same batch: pickup movement cancellation exposed a tick-order
defect during a long capture frame (velocity 330cm/s but the action still active).
Mutable presentation now has a native CharacterMovement tick prerequisite.
Final build/native61 report ends 203231Z. Fresh k campaign passes 40 interaction
checks, including real held movement input and all three existing camp actions;
report m3-grapple-contact-action-regression-k.json. The helper preserves guards
and measures game time. Earlier h/i/j failures remain archived for diagnosis.
Final k also passes 103 combat checks and reruns all 47 grapple checks after the
movement-order fix: m3-grapple-contact-combat-regression-k.json and
m3-grapple-contact-live-final-k.json. End state is outside PIE with no dirty
map/content, native source sync0, original 150 saves and 757 engine-plugin files
unchanged. Next work must retain the complete objective; zipline gameplay is
still absent, and grapple range/cover/recovery/throw-pose acceptance remains.

Latest hang/hook batch: the full goal remains ACTIVE. The generated animation
proxy now blends a retargeted Hold_Rope_Idle loop during a real plugin hang.
Pose audit proves right hand lower/left upper; Dynamic Rope pins the curled
middle_03_r / middle_03_l grips. The original SM_Grapnel now uses authored
HookPoint/RopeEye sockets, appears while deployed and stows on release/menu.
Armed combat suppresses grappling. Final compiled fresh campaign
`coastal_test_m3_grapple_0909e` passes 41 grapple checks including keyboard
reel-out, controller throw/release/reel in/out and hang retirement; report
`m3-grapple-hang-live-final-e.json`. It also passes 103 combat-animation checks
(`m3-grapple-hang-combat-regression-e.json`); prior d campaign passes 39 action
regressions. Build/native61 report ends 195552Z; Python70 and preservation pass.
IMPORTANT follow-up: a stricter added rope-eye contact assertion FAILS.
GetNodePosition(last) is 2.03cm from RopeEye at commit and 2.27cm after 0.3s.
Reports: m3-grapple-eye-first-frame-e.json and m3-grapple-eye-settled-e.json.
Current check_m3_grapple.py retains the failing <0.5cm assertion; the 41-pass
report predates it. The player-side hook capture was viewed and shows the gap.
Resolve this before accepting hook contact. Possible lead, unproven: gantry
has nonuniform (.8,.8,4.2) scale; the plugin freezes LocalMeshTransform and
composes it with the binding transform, while the pinned tail uses an inverse
transformed point. Inspect RopeComponentTip.cpp and RopeComponentThrow.cpp:1107.
Read docs/M3_ACTIVITY_EXPANSION.md for remaining swing/recovery/throw-pose,
zipline and full activity scope. Do not claim overall goal completion.

Latest Dynamic Rope batch: installed Dynamic Rope 1.0.1 is now enabled as an
explicit UE5.8 expansion dependency. UCoastalGrappleComponent subclasses the real
plugin wielder; UCoastalGrappleRope uses its GuaranteedWrap targeting, solver and
native movement constraint. Q / D-pad up toggles throw/release, Z/right reels in,
X/left reels out. A native wrap-provider gantry is authored at (13200,1800,950).
Initial live checks reached actual wrapping/reel movement. They exposed and fixed
generated-mesh cleanup destroying the rope and Releasing being misreported as an
active grapple. Final disposable `coastal_test_m3_grapple_0909c` passes 21 live
checks, saved as `m3-grapple-live-final-c.json`. The braided-rope/airborne-character
capture was viewed. Final source build and native61 pass (report ending 193859Z).
Read docs/M3_ACTIVITY_EXPANSION.md for the exact remaining verification scope.

The UE5.8 descriptor and FirstSignal gantry edit have verified backups and hash
records; preservation now has 160 unchanged protected files, one reviewed map
and one reviewed plugin enable. All 150 baseline saves remain untouched. An
original three-prong grapnel was imported as /Game/Coastal/Activities/Rope/SM_Grapnel;
runtime tip binding, hand/socket alignment, hang/throw animation, broad swing and
reel-out/gamepad/recovery checks and zipline gameplay are pending. Do not claim
complete grappling from the component or first successful reel alone.

Latest gesture/aim batch: sustained pistol aim now plays the raise transition once
and holds its final frame; hold-idle still loops. G / D-pad down cycles four
retargeted RamsterZ gestures; 18 seconds of safe unarmed idle permits a relaxed
standing variation. Existing HUD input hints expose the control through the
optional character interface. Movement/jump, menus, encounters and native action
ownership interrupt or gate gestures. Actual input tests pass 43 gesture checks
and 101 combat-animation checks in `coastal_test_m3_gestures_0909b`; reports are
`m3-gesture-live-final-b.json` and `m3-aim-hold-live-b.json`. Four generated gesture
captures were viewed. Build/native61 and Python70 pass. The source/retarget work
remains uncommitted; see the full checklist for still-pending activities.

Latest pistol follow-up: owned body/slide/barrel/magazine are merged privately
into `/Game/Coastal/Activities/Combat/SM_CoastalPistol`. Source pose sampling proved
these clips aim with `hand_l`, so the FirstSignal director now attaches there;
the visible barrel tip supplies the muzzle offset. Forward side-view inspection
passes; fresh `coastal_test_m3_pistol_0909e` passes 93 combat-animation checks.
See `m3-pistol-alignment-live-e.json` and `m3-pistol-binding.json`. The FirstSignal
map is intentionally updated and backed up; 161 other protected files, including
all 150 original saves, remain unchanged. Broader directional contact review
remains open; sustained aim is covered by the newer batch above.
This supersedes the provisional-mesh statements in the first-batch history below.

The user explicitly requested implementation of the complete animation/activity
table, including Dynamic Rope ziplines/grappling, a drivable M3 speedboat,
motorcycle, fishing, ladders, crawling, melee, camp actions, gestures and gliding.
This full goal is ACTIVE and incomplete. Read `docs/M3_ACTIVITY_EXPANSION.md`
for its requirement-by-requirement checklist; do not reduce it to combat alone.

The first batch builds and passes 61 native tests (60 clean, one known AGIS
warning). Disposable `coastal_test_m3_activities_0909c` passes 90 combat-animation,
18 creator, 39 existing-action and 16 existing-combat assertions. Directional
hits and pistol layering/input are integrated; final pistol art/muzzle alignment
is still pending. Standing hits retain full-body motion; moving hits and pistol
actions blend above the spine. Keep the native movement/ammo/save authorities.

The M3 ShipAndSea folder is copied privately (39 SHA256-verified files). The
supplied motorcycle's five skeletal parts, rod, wooden ladder and Java barb are
imported and textured under `/Game/Coastal/Activities`. They have no new gameplay
yet. Dynamic Rope 1.0.1 is installed at the engine Marketplace path
`D:/Unreal/UE_5.8/Engine/Plugins/Marketplace/DynamicRa1b70179ea1eV4`; its actual
headers and README were inspected. Rope gameplay is not implemented/enabled by
this batch. Do not substitute a generic cable component for the requested plugin.

The preservation baseline covers 150 pre-existing saves and 12 maps/descriptors;
all 162 are unchanged. New source remains uncommitted and purchased payloads,
backups and reports remain private. The detailed document records the report
names and remaining work, including the provisional weapon-art limitation.

## Mutable creator and player animations — September 9, 2026

The private UE5.8.2 host now has generated Mutable Body/Head art, 16 controls
under Pause → Customize character, groom integration, and per-campaign GUID
appearance sidecars. Native pawn, movement, input, inventory, interaction,
recovery and action owners remain authoritative. The generated meshes retarget
the existing locomotion/swim/camp/shelter poses. Two owned retargeted clips add
successful pickup and nonfatal player-hit reactions with interruption gates.
See [M3 character creator](docs/M3_CHARACTER_CREATOR.md) for exact paths,
versioned persistence, authoring scripts and current acceptance evidence.

The sample's 774-file expanded closure plus its default instance are staged
privately. Mutable/HairStrands/MutableClothing/IKRig are enabled in the host;
character cook directories are configured. This is not a completed packaged
build. Native build and 60 tests pass (59 clean, one existing AGIS warning).
Scripted creator save/discard/reload, all 16 control variants and generated
hair, and movement/pickup/hit/camp/shelter playback pass. Source changes remain
uncommitted; purchased payloads/evidence remain private. Earlier unintegrated
creator/animation statements below are historical and superseded for this
bounded selection. Do not bind vehicles, ladders or magic without gameplay.

Final combined `g` PIE passes all six suites: 18 creator assertions, 39 action
assertions, 16 combat assertions, 56 variant assertions, swimming (824 generated
samples) and invalid-sidecar guards. The custom animation proxy must retain its
registered graph root; earlier weaker action checks admitted idle motion and
are superseded by this run. All 111 pre-existing saves, both maps and the
rollback descriptor are unchanged. The editor is outside PIE with 2,935 actors,
no dirty packages and restored background throttling. Final evidence:
`../local-evidence/m3-character-final-suite.json` and preservation/editor reports.

## Private staging import and animation audit — September 9, 2026

The authorized private migration from `G:/coastline/M3DestinationStaging57`
to `G:/coastline/LocalHost58/CoastalExploration` is complete. It added **2,408
files** totaling **10,971,245,578 bytes**, retained **746 identical existing
files**, and found **0 conflicts**; every new-file SHA256 passed. Source map,
controller, Config and descriptor content were not replaced. Evidence:
`../local-evidence/m3-staging-additions-copy.json`.

The registry audit loaded **296 animations** with valid skeleton and duration,
including **97 motorcycle animations**. This is source loadability only; no
retargeting, playback, contact, or gameplay acceptance passed. Three hard
missing references are the motorcycle demo's old `CarInteractAnimVol1`
ControlRig paths, and 12 other missing references are soft paths. One
732-byte Retropunk Saloon material package is unregistered but matches the
source. Evidence: `../local-evidence/m3-staging-animation-audit.json` and
`../local-evidence/m3-staging-import-registry.json`; registry validation remains
open because of those missing references.

The preview refresh build passed. Final `j` PIE passed **53 assertions** in
`coastal_test_m3_preview_0909j` in 16.375 seconds: colored battery/fuse 3D
captures, all rotation inputs, stale GUID/revision refusal, AGIS/inventory and
camera preservation, cleanup, recovery, save/reload, and pre-existing save
preservation. Default fuse orientation art polish remains open; physical,
performance, and package acceptance remain separate. Native editor is outside
PIE with zero dirty packages. Purchased payloads and evidence remain private
and source changes remain uncommitted.

Combat source now compiles after the root `TObjectPtr` fix, but it is not bound
to imported assets and has no runtime or gameplay acceptance.

Final source sync matched 198 native source files across three plugins to the
active host by SHA256. The editor was closed outside PIE with 2,935 actors, no
dirty maps or content packages, and restored background throttling.

## Authoritative scope change — player-facing Mutable character creator — September 9, 2026

The next M3 scope is a player-facing Unreal Mutable character creator. This
replaces the earlier fixed-character plan; do not author or describe a fixed
character creator as the target. The installed UE5.8.2 Mutable plugin is at
`D:/Unreal/UE_5.8/Engine/Plugins/Mutable/Mutable.uplugin`. Mutable supports
runtime skeletal-mesh, material, and texture generation, but the installed
plugin does not provide customizable human art by itself. See the official
[Mutable skeletal mesh generation documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/mutable-skeletal-mesh-generation-in-unreal-engine).

The user delegates animation selection to the agent: inspect the signed-in Fab
library and use as many useful clips as make sense without overlap. The full
31-product animation inventory and candidate allocation are recorded in
`docs/M3_ANIMATION_SELECTION.md`; do not ask the user to list these again.
Mutable Sample is installed at `G:/coastline/MutableSample` (association 5.8),
including a character graph, base body/head, clothing and groom sources. Inspect
its dependency closure and skeleton before private integration. Preserve the
existing native character/movement owners and backward compatibility of saves.
The Motifect Locomotion download was blocked by Chrome ERR_BLOCKED_BY_CLIENT;
no successful download is claimed. Do not bypass browser protections.

The merchant and item-preview increment currently has a combined native build
and **59 native tests** passing (**58 clean, one known AGIS warning**). The
merchant actor is saved in the host (+1, **2,935 actors**). Final preview PIE
`coastal_test_m3_preview_0909j` passes **53 assertions** with colored battery
and fuse imagery; default fuse orientation art polish remains open. Work
remains uncommitted and purchased assets/evidence remain private. Read [M3
character creator](docs/M3_CHARACTER_CREATOR.md) for the bounded plan; this
scope change does not overwrite earlier checkpoint claims.

## Destination music/thunder checkpoint - September 9, 2026

The independent UE5.8.2 host now integrates nine selected clips from seven music/
thunder archives: seven destination scores and two distant thunder clips. One
controller soundscape owner uses existing destination records and the campsite
marker, two-second sequential score fades, boundary hysteresis, and actual overhead
collision cover for thunder gain/low-pass. Music uses the existing Ambience channel
(now labelled Music & ambience); thunder uses Effects. No campaign/preferences
schema, inventory, quest or weather-simulation change is introduced.

Native build and **56 native tests** pass. The fresh disposable
`coastal_test_m3_soundscape_0909e` campaign passes **49 gameplay assertions**;
all **seven mixer checks** pass. Music at 50% measures **0.4995 of baseline RMS**.
Music, Effects and Master mute produce zero output in the relevant captures.
The thunder sources retain 5.56/7.19 seconds of silent pre-roll; signal probes seek
to eight seconds. Earlier harness/API and silent-intro capture reports are retained.
These results are scripted gameplay/mixer evidence, not listening acceptance.

All **82 pre-existing save files**, both host maps and project descriptors are
SHA256-verified unchanged (86 files). The UE5.7 rollback host remains intact.
Editor is outside PIE with 2,934 actors, no dirty packages, and normal background
settings restored. Source changes remain uncommitted on `codex/m1-native-integration`;
HEAD remains `f1f7ca8`. Purchased payloads and execution evidence remain private.

Read [soundscape](docs/M3_SOUNDSCAPE.md). Evidence is under `../local-evidence/m3-soundscape-*`:
build, native tests, live gameplay, mixer output and preservation reports. Next work
is merchant animation, isolated pickup/rotation integration, then fishing/combat
audio. Five work archives remain unimported; Bonus Vol.01 is separately reserved
for tonal review. Listening, art polish, physical controls, performance and Windows
package acceptance remain open. Earlier checkpoints below are historical.

## Interaction/UI audio checkpoint — September 9, 2026

The first ordered increment of the new asset batch is integrated in the independent
UE5.8.2 host: ten selected WAVs from Fantasy UI Essentials, Horror Interaction and
Cozy Everyday Objects. Existing bridge/UI owners drive one bounded feedback voice
for successful actions, menu navigation, cancel/error, inventory, storage, notes,
records and doors. Automatic panels do not duplicate the action cue. Reload,
recovery, pause and routing teardown retire the appropriate voice.

Native build and all **54 native tests** pass (53 clean, one retained AGIS warning).
Source checks pass **70 Python tests and 3,538 consistency checks**.
The disposable `coastal_test_m3_audio_0909e` campaign passes **69 scripted assertions**.
Real mixer captures pass: Effects/Master mute each produce zero signal; Effects
50% measures 0.4988 of baseline RMS. Background capture temporarily bypasses app
focus mute and solos PIE audio, then restores both. Listening/tonal, physical-device,
spatial listening, performance and Windows-package acceptance remain separate.

All **72 pre-existing save files**, both host maps and project descriptors are
hash-verified unchanged (76 files). The original UE5.7 rollback host is intact.
Editor is outside PIE with 2,934 actors, no dirty packages and normal background
settings. Purchased payloads and evidence remain outside the source repository.

Read [interaction audio](docs/M3_INTERACTION_AUDIO.md). Evidence: `m3-interaction-audio-build.*`,
`m3-interaction-audio-native-tests.*`, `m3-interaction-audio-live.json`,
`m3-interaction-audio-output.json` and `m3-interaction-audio-preservation-after.json`
under the workspace's `local-evidence`. Earlier failed harness/silent-output reports
are retained. Only the generated PCH cache was moved to the current user's SSD,
with SHA256 verification and its HDD original preserved; see `m3-ue58-ssd-pch-0909.json`.

The remaining twelve archives are not imported. Bonus Vol.01 is reserved for tonal
review. Next ordered work is music/thunder, merchant animations, isolated
pickup/rotation review, then fishing/combat audio. This is an interaction-audio
checkpoint, not completion of all 16 archives or of M3.

## Additional M3 asset batch — September 8, 2026

The user added all 16 archives in `G:/coastline/even newer assets and animations`
to M3 planning. See [the integration plan](docs/M3_NEW_ASSET_BATCH.md) and
`data/m3_new_asset_batch.json` for the inventory, intended uses and acceptance.
Work covers interaction/UI/fishing/combat audio, music/thunder ambience, merchant
animations, and pickup/rotation inspection. Status is planned, not imported.
The native merchant archive is encrypted; its separate FBX archive is readable.
Pickupable & Rotatable Items declares UE5.6 and needs isolated UE5.8 review.
Existing gameplay acceptance below remains valid for the previous scope.

## Selected M3 gameplay acceptance â€” September 8, 2026

The isolated UE5.8.2 trial now passes all three selected workstreams in scripted
PIE. Swimming passes 22 water/checkpoint cases plus 11 actual route waypoints;
Morbid shelter passes 4 animation/menu/cancellation cases. The German Shepherd
passes 7 follow/wait, movement/animation, nonblocking collision, water/recovery and
campaign-reload cases. The final epoch cooldown fix is compiled and verified.

Both owned destination packs were staged through a separate UE5.7 project and
copied into the independent UE5.8 trial: 598 files, each SHA256 verified. Atlantis
upper terraces and the monitoring station have dry arrivals, access paths,
interiors and investigation records. Both directions and interior walks pass.
The legacy seven-record campaign retains its progress, completes the two new
records through real interaction/journal UI, and retains all nine after reload.

Editor is outside PIE with 2,934 saved actors, no dirty packages, background
throttling restored and MCP connected. Final native build and all 52 native tests pass. Python 70 tests and 3434 structural
checks pass after the documentation update. All 58 pre-existing save files
remain byte-identical. Original UE5.7 rollback content was not edited. No commit
or push was made. Physical controls, visual/art polish, performance and packaged
acceptance remain; this is not a claim that the whole M3 release gate is complete.

Evidence under `../local-evidence`: `m3-outer-coast-build.execution.json`,
`m3-outer-coast-native-tests.json`, `m3-all-waters-live-hardened.json`,
`m3-swimming-route.json`, `m3-morbid-shelter-live.json`,
`m3-coastal-companion-live.json`, `m3-staged-destination-content-copy.json`,
`m3-outer-coast-traversal.json`, `m3-outer-coast-quests-live.json`, and
`m3-selected-three-preservation-after.json`. Earlier checkpoints below are
historical where they conflict with this acceptance.

## Companion checkpoint — September 8, 2026

One German Shepherd director is saved in the independent UE5.8 trial (2,494 map
actors). Follow/wait uses the existing pause-menu owner via an optional Foundation
interface. Collision-aware movement, supplied animation, nonblocking player
collision, current-water regroup and player recovery pass six scripted PIE cases.
All52 native tests passed after the per-frame movement and wet-displacement fixes.

The final campaign-reload probe found the previous regroup cooldown surviving the
epoch change. Source now resets that timer and stuck tracking, but the final DLL
link is blocked by a separately opened trial editor (PID14860 at this checkpoint).
Close that instance, rerun `tools/build_ue58_trial.py`, then relaunch and rerun
`tools/unreal/check_coastal_companion.py` in a fresh disposable campaign. The latest
full live report is NOT passed; six earlier rows pass. Evidence:
`m3-coastal-companion-live-epoch-delay.json`, `m3-companion-final-native-tests.json`,
`m3-companion-epoch-build-locked.execution.json`. A prior rerun crashed in
D3D11RHI/SlateRHIRenderer; its log is retained as `m3-companion-renderer-crash.log`.

Swimming also passes the full11-point dry dock/ramp/swim/return traversal, in
addition to the22 all-water cases and4 Morbid shelter cases below. All58
pre-existing save files remain unchanged. Physical controls/package acceptance
remain separate. `docs/M3_COMPANION.md` describes behavior and limitations.

Both missing destinations are owned in Fab. A separate UE5.7 staging project is
at `G:/coastline/M3DestinationStaging57/M3DestinationStaging57.uproject`; the user
is handling Launcher downloads. At this checkpoint its only asset directory is
CarpentersWorkshop; Atlantis and SciFi Station are not yet verified or assembled.
The original UE5.7 rollback host remains untouched. No commit/push was made.

## Swimming and shelter acceptance — September 8, 2026

The corrected swimming seam now passes scripted PIE in the independent UE5.8.2
host: 11 dry checkpoints, the low shore, seven water samples, basin overlap in
both directions, and boundary/deep-water safe return. Both overlap legs retain
swimming without the previous default-volume bounce. Evidence:
`../local-evidence/m3-all-waters-live-hardened.json` (`passed: true`).

Morbid shelter passes all four native-menu/gameplay checks: actual supplied clip
starts, completion restores the original mesh and animation, menu cancellation,
and jump cancellation. Evidence: `m3-morbid-shelter-live.json`. These are scripted
PIE checks; physical controls and packaged acceptance remain separate. The fresh
disposable campaign is `coastal_test_m3_water_shelter_0908a`; pre-existing saves
are verified unchanged in `m3-water-shelter-preservation-after.json`.

Companion implementation is in progress. Both missing destinations were confirmed
owned in the signed-in Fab library. Unreal downloads require the Launcher or
Fab plugin; staging payloads are still unavailable, so assembly remains pending.

## Combat continuation — September 8, 2026 (local time)

The isolated UE5.8.2 host now compiles the authoritative tracer/cooldown fixes and
passes **50 native tests plus 16 scripted combat PIE cases**. The final disposable
campaign is `coastal_test_m3_combat_resume_0908c`. The exact spread-adjusted vendor
ray drives the rendered tracer; slow/fast time-dilation probes pass. Mouse/gamepad
fire, keyboard/gamepad reload, paused input/reload, world cover, offscreen/visible
sentry behavior, defeat safe return and campaign-epoch cleanup pass.

Live checks found and fixed sentry damage missing the standard player's capsule:
cover is checked on Visibility, followed by a real Pawn-object hit, without changing
player collision globally. A separate cover fixture initially failed because its
wall was static; the corrected disposable movable fixture passes and restores its
original transform/mobility. Earlier failed reports remain in local evidence.

First Signal has **2,493 saved actors**, including six bounded harbour combat
actors using clearly provisional engine meshes. No final weapon/enemy art or
physical-device/package acceptance is claimed. Existing save files and the original
UE5.7 host map are hash-verified unchanged (53 preserved files). The trial map has
an explicit pre-combat backup. Editor is outside PIE, with zero dirty packages,
background throttling restored and MCP connected.

Evidence in `../local-evidence`: `m3-combat-resume-build.execution.json`,
`m3-sentry-fixed-build.execution.json`, `m3-sentry-fixed-native-tests.json/.log`,
`m3-coastal-combat-authoring.json`, `m3-coastal-combat-live.json`, and
`m3-combat-resume-final.json`. The second native build briefly hit the closing
editor's DLL lock; retry after shutdown passed. The purchased vendor's original
Staged payload remains intact; only its private Working/trial copies were patched.
See `docs/M3_COMBAT_SHOT_CONTRACT.md` (or that filename from this docs directory).

The existing journal data's stale unavailable-travel sentence was synchronized to
the already-implemented Industrial Harbour lead. Python checks pass (70 tests).
Remaining gameplay gates include the corrected swimming seam and Morbid shelter;
companion behavior, art polish, physical controls, performance and packaging remain.

## Resumed on relocated machine — September 8, 2026

The user explicitly resumed development and requested MCP startup. Work now lives
at `G:/coastline/CoastalExploration`; the independent host is
`G:/coastline/LocalHost58/CoastalExploration`. Installed Unreal is **5.8.2** at
`D:/Unreal/UE_5.8`. Historical F: and C:/Users/Doc paths below belong to the prior
machine and must not be used as current execution paths.

MCP is running at `127.0.0.1:30020`, with verified project/engine identity.
First Signal contains 2,487 actors, with no PIE session or dirty packages.
All **50 native Coastal tests pass** using the existing compiled binaries,
including the four previously unrun combat rule tests. The AGIS Capacity case
retains its vendor GameplayTag warnings. This is not combat PIE acceptance or a
new native build. All **70 Python tests pass** on Python 3.10.
Evidence: `../local-evidence/m3-relocated-mcp-startup.json`,
`m3-relocated-native-tests.json/.log`, and `m3-relocated-python-tests.log`.

Launch/build/source-sync tools now resolve the workspace relative to the repository
and the engine from the installed Launcher manifest/registry, with
`COASTAL_UE58_ROOT` as an explicit override. The asset importer no longer depends
on Python 3.12's `Path.is_junction`; junction/symlink rejection remains intact.
`tools/unreal_mcp_call.py` checks host identity before editor operations.
The updated launcher was syntax checked and its path resolver exercised; the
current editor was launched directly with the resolved paths. No native rebuild
or source sync was executed in this continuation. Background throttling is restored.

Next gameplay work remains the authoritative shooter tracer and cooldown-clock
fixes, followed by combat PIE, corrected swimming seam and Morbid shelter checks.
Do not treat this tooling/native-test increment as completion of those gates.

# Historical paused checkpoint (superseded by resume above)

# Active UE5.8 migration trial: September 8, 2026

PAUSED BY USER: do not resume development until the user explicitly says go.
The subsequent request authorizes staging, committing and pushing this source
snapshot only. Latest editor build (including shooter and input probe) succeeded
at 2026-09-08T17:52:33Z. The latest native test run remains the 46-test pre-combat
run below; the four added combat tests and combat PIE acceptance have not run.
Terra review found an unspread project tracer differing from the vendor's spread
damage ray, and a shot-cooldown clock mismatch under time dilation. A private
vendor authoritative-shot callback patch was interrupted; the repository wrapper
still uses TraceEndpoint and needs that follow-up when development resumes.
The live harness now queues mouse/gamepad fire and keyboard reload through the
guarded editor input probe, but these live cases remain unrun. No editor is open.
Publication check: all 66 Python tests passed; public source excludes the external
host, purchased vendor payloads, campaign saves, generated binaries and caches.
The earlier progress paragraphs below are historical where they conflict here.

Finish-integration checkpoint: isolated Host58 build passed then all46 native
tests passed (45 clean, one existing AGIS warning). Latest report before combat:
../local-evidence/m3-ue58-native-report-20260908T171318Z. All28 original critical
hashes unchanged. Exact First Signal authored startup extension fixed M1-only
manifest rejection of seven destination records. Live old-save read-only load,
all7 real Bridge/journal quest interactions, save/reload generation9->17, and
old/completed campaign replacement reset/restore now PASS. Evidence:
m3-destination-old-save-live-ue58.json, m3-destination-quests-live-ue58.json,
m3-destination-campaign-switch-ue58.json. Completed disposable save is
coastal_test_m3_quests_ue58_0908b; original swimming campaign remains unchanged.
Day/night real sun/fog, pause/resume PASS (m3-day-night-live.json).
PCGEx bounded6 grass instances: generation/cleanup/deterministic regeneration
PASS (m3-pcg-route-live.json); original six are editor-only authoring references.
Map saved outside PIE with2487 actors, zero dirty packages, then editor closed.

All-water samples passed dry checkpoints and seven regions, but actual basin
seam exposed native ImmersionDepth full-depth fallback when capsule edge enters
higher-priority brush before vertical centerline. Root added IsOverlapInVolume
surface-footprint trace and fallback respects it; currently building, unverified.
Root also moved shelter_rest from wrong Session menu to correct Pause menu;
Morbid live check pending rebuilt DLL. All-water test now uses simulation time
and authored default locomotion refs so low background FPS/reruns are valid.

Combat actual vendor Working source/content + CoastalExpansion58 copied/enabled
only in Host58. Shared71-action build running with root seam/menu and Sol recovery
hook. Terra combat_review performs independent focused review. No package built.
User shooter recheck succeeded: actual Fab payload DOES include source.
123 files / 187311088 bytes independently size/SHA256 verified in
F:/coastline/LocalVendor/AdvancedShooterSystem/Staged/Advanced22548e075794V1.
Earlier binary-only finding applies to the separate public demo, not this pack.
Acquisition evidence: ../LocalVendor/AdvancedShooterSystem/Evidence/acquisition-report.json.
Sol completed new disabled-by-default Plugins/CoastalExpansion58,
private vendor Working patches, combat author/check scripts, and narrow
CoastalPlayerRecoveryComponent.h / CoastalRecoveryExecution.cpp defeat-return hook.
No original health-system source appears in that shooter manifest. Root owns host,
build/editor, startup fix, ledger. No paid acquisition or project promotion occurred.

PCGEx source and bounded procedural runtime graph verified in Host58.
German Shepherd 27 assets imported (20MB); companion behavior not authored yet.
ONLY generated UnrealEd PCH cache is a junction to the SSD cache. Source/content
remain independent real copies. tools/use_ue58_ssd_pch.ps1 preserves original HDD
cache; do not rerun blindly. Use two compile workers and pause large transfers.

The user asks to upgrade to unlock selected UE5.8-only features. UE5.8.1 is already
installed at C:/Program Files/Epic Games/UE_5.8. Root is creating an independent
copy at F:/coastline/LocalHost58/CoastalExploration using tools/prepare_ue58_trial.py.
Preserve F:/coastline/LocalHost/CoastalExploration as the UE5.7.4 rollback host.
The trial has real content and plugin copies, not shared junctions/hardlinks.
Do not launch the original host under 5.8 or rewrite original assets/saves.
Trial build/verification evidence uses m3-ue58-* under ../local-evidence.
The original editor was closed cleanly with 2369 actors and no dirty packages.
The swimming seam fallback compiled in the baseline; its hardened collision
checks and seam live acceptance remain pending the next source sync/build.
Morbid shelter/day-night/dressing recipes exist but are not yet authored/verified.
UE5.8.1 baseline editor build PASSED all 168 actions. Evidence:
../local-evidence/m3-ue58-baseline-build.execution.json and .log. Trial targets use
V7/Unreal5_8; V6 was rejected by the installed editor's shared build requirements.
Use -UBANoDetour and two compile workers on this machine; content transfer must
pause during compilation to avoid disk contention. Runtime validation/promotion
remain pending until native/live gates pass. The independent content copy is now
complete: 10,904 files, zero missing or wrong-sized files, two vendor junctions
materialized as independent real folders. All 43 baseline native tests passed
(42 clean, one existing AGIS warning case), original critical hashes unchanged.
Trial editor PID 22252 opened First Signal on 5.8.1 with all 2369 actors,
zero dirty packages and a clean map check. Baseline copied-save PIE smoke now
PASSES: old swimming campaign loads at generation12 without writes, player mesh
and animation class load, UI initializes, walking mode and swimming/camping/
recovery components are present. Evidence: m3-ue58-baseline-live.json. All28
original critical file hashes remain unchanged. Test PIE is ended. This verifies
the baseline stack, not actual swimming/combat/destination gameplay or packaging.
Destination quest review found and Sol fixed a mission/journal prerequisite bypass;
the reviewed source is still absent from the baseline trial and needs sync/build.

# Expanded user scope: September 8, 2026

The user now requests swimming in all actual playable waters, Morbid gameplay,
Atlantis/SciFi Station acquisition and assembly, destination quests/interactions,
richer dressing and scan/terrain edge blending. All 24 new Fab selections are
recorded in data/m3_requested_assets.json, including the German Shepherd as a
trusty companion. The user explicitly confirmed playable combat and authorized
needed downloads/installations. Earlier no-combat/no-swimming exclusions are
superseded. Single-player, existing First Signal progress and coherent saves remain.
Integrate overlapping interaction/movement plugins deliberately; do not create
competing AGIS inventory or input authorities. Acquisition is not integration.
Root coordinates Astra High, one Sol High implementation worker at a time, Terra
Extra High for bounded independent review. No project-local model override found.

# Current integrated checkpoint: September 8, 2026

Latest continuation: bounded surface swimming is implemented in the sheltered
dock basin. The editor build and all 43 native Coastal tests pass (one AGIS
capacity test has vendor GameplayTag warnings); all 66 Python tests pass. Scripted
entry, forward swimming, ramp exit, pause/resume, wet save/load and out-of-zone
recovery passed. The final 11-waypoint route takes 35.641 seconds. All 22
pre-existing saves and the original seven WorldObject IDs remain unchanged.

Read **Swimming continuation** in docs/M3_IMPLEMENTATION.md. Preserve the actual
water physics volume, swimming component and host character hooks, player
Blueprint animation references, the opened west pier handrail segment, narrow
landing and three shoreline-rock clearance moves. Unreal is open outside PIE
showing the basin with no dirty packages; background throttling is restored.
Native build: ../local-evidence/m3-swimming-editor-build-fixed.execution.json.
Final source evidence: ../local-evidence/m3-swimming-source-final.execution.json.
No M3 Windows package or physical-device acceptance was performed.

Next bounded work is authored Morbid animation use, followed by destination
dressing, scan/terrain edge blending, physical-device traversal, performance and
packaging. Atlantis and Modular SciFi Station still need their Unreal downloads.
All four omitted packs and the initial prison/village assemblies are retained.

September 8 scope correction: Swimming Animations, Morbid Motions and Haunted
Prison were omitted from the prior seven-destination/Camping list. Their archives
already exist directly under `F:/coastline` and all four packs are now imported.
Swimming gameplay is now verified above; Morbid animation use remains pending. The user confirmed Medieval Italian
Village as the fourth omitted item; `MedievalItalianTown.zip` is also present.
Read the September 8 scope correction in `docs/M3_IMPLEMENTATION.md`
before treating the earlier list as exhaustive. The new swimming request takes
precedence over older first-build exclusions; retain safe water recovery until
the actual swimming transition is implemented and checked.

Latest continuation adds five destination assemblies, five routes, six dry arrivals
and two usable Camping actions. See **Destination and camping continuation** in
`docs/M3_IMPLEMENTATION.md`. Atlantis and Modular SciFi Station are owned but their
Unreal packages are still unavailable locally. Preserve the host character camping
hooks, expanded boundary actors, corrected Baelo material and original seven saved
WorldObject IDs. Latest editor build:
`../local-evidence/m3-expansion-checkpoint-build.execution.json` (exit 0).
Both camping clips complete and restore locomotion; movement/jump/menu cancellation
and all six arrival points passed scripted PIE checks. Final physical traversal,
environment polish and packaging remain outstanding. The checkpoint fix retains
an already-recorded visit across menu/save interruptions and rendering hitches.
Four native recovery tests pass. Live checkpoint generation stayed at 39 through
two pause/back cycles and a screenshot; an older campaign loaded successfully and
all 18 original save hashes remain unchanged. Compiler cache and editor background
throttling are restored. No M3 Windows package was built in this continuation.

Retain the earlier inventory icons and exact cabinet-book Visibility fix. Do not
run the native AGIS test suite concurrently with PIE: its live provider populates
the shared vendor item table and contaminates the tests. Isolated AGIS rerun passed.

The user authorized getting the open Unreal project running and configuring MCP. Source is at `F:/coastline/CoastalExploration`; host is `F:/coastline/LocalHost/CoastalExploration`. The C: project directory is no longer a junction. The accumulated M3 source now compiles: `../local-evidence/m3-integrated-editor-build-ssd.execution.json` reports exit 0. All 40 native Coastal automation tests pass in `../local-evidence/m3-native-report/index.json`. These supersede the earlier pending-compilation statements below; physical-device, complete M3 presentation and packaged acceptance are separate gates.

The existing Codex Unreal MCP client uses a project-local editor Python listener; see `docs/UNREAL_MCP.md`. Preserve loopback-only binding and verify project identity before editor mutations. Continue batching native builds at meaningful checkpoints. See the latest integrated continuation in `docs/M3_IMPLEMENTATION.md` for live editor/campaign results and remaining work.

# Current local milestone: M3 in progress

The user explicitly requested batching work instead of building after every feature (September 7, 2026). Use lightweight source checks between features. Run native builds/package work at meaningful integrated checkpoints, not for each small increment. The first M3 UI build was cancelled under this direction; do not claim it passed. Gameplay/device acceptance remains deferred.

Read `docs/M3_IMPLEMENTATION.md` and `../M3_HANDOFF.md` first. M3 has started with a bounded native UI presentation pass. Gameplay/device testing remains deferred; do not infer acceptance from compilation. The M2 archive is unchanged and does not contain the M3 source increment. Remaining art, audio, input, graphics and performance work is listed in the M3 document.

The second M3 batch adds graphics drafts through the existing display owner, weak Slate input observation for native hints, and campaign-name Tab focus. Preserve explicit Apply/Save, custom settings snapshots, active-trial exclusion, and input-observer teardown. See `data/m3_graphics_input_acceptance.json` for unrun native cases; source checks are not graphics/input acceptance.

The third batch adds read-only inventory row presentation and a journal index. Direct item selection must reread AGIS and resolve the instance GUID; never replace it with a stale row index or direct mutation. Journal browsing must remain separate from transcript acknowledgement. Preserve command-based focus restoration and consult `data/m3_inventory_journal_acceptance.json` at the later integrated checkpoint.

The fourth batch is saved environment construction; read `docs/M3_ENVIRONMENT.md`. Do not duplicate the dressing or offset basin scenery twice. Map backups and editor evidence are under `../local-evidence`. M3 native source remains uncompiled; the editor asset pass used the existing binaries and does not pass gameplay acceptance.

Surface continuation adds project-owned ground/rail materials and hides the original deck visuals while retaining collision. Preserve `T_RockNormal` in the M3 texture folder: the vendor rock normal is configured as linear color and cannot be sampled as Normal directly. Fresh and repair recipes are documented in `docs/M3_ENVIRONMENT.md`; final captures use `m3-surfaces-final-*`.

## Retained M2 handoff

Read `docs/M2_IMPLEMENTATION.md`, `docs/M2_SCOPE_AUDIT.md` and `../M2_HANDOFF.md` for the actual local state in `F:/coastline`. M2 implementation is complete: native editor and Windows targets compile, the persistent map is assembled and visually inspected, saved actor properties have been read back, and the Windows archive is built at `../LocalPackageM2/Windows`. Gameplay acceptance remains deferred. The temporary SSD compiler cache was restored to F: and removed from C:. Retain the real AGIS/Hyper authorities and the distinct `level.first_signal` save identity.

The user explicitly deferred testing until the game is more fully built. Do not run or claim M2 gameplay/device acceptance as part of this handoff. Build and asset-construction evidence are separate from that deferred acceptance. Earlier source-only status and test instructions below are historical and must be read in light of the user's current direction and the current evidence.

---

# Current source increment: M1.10

Read this addition before the retained earlier guidance below. M1.10 is `0.1.10-m1-display-settings-source`. The current source has 38 supplied Unreal tests, all unrun in this environment. Older counts below are historical.

Display is owned only by the existing UI through an internal UObject, opt-in false. Read `docs/M1_10_IMPLEMENTATION.md`, `docs/M1_10_DISPLAY_WIRING.md` and `docs/WHAT_REMAINS.md`. Trial requests do not stage or save engine preferences; only explicit painted, current, before-deadline Keep can request an engine save. Do not call ApplySettings during a trial, replace the actual rollback snapshot with last-confirmed config, or infer verified IO from SaveSettings. Preserve the early real-time watchdog and cancellation hooks. No campaign or player-options schema change. Do not bypass unavailable native host checks to make PIE look passed.

The actual M1 host build/vendor/Windows campaign gate remains unpassed. No helper/structural count can pass it. Do not silently expand into unrelated gameplay or declare M2 assembled. Run all retained source tests and new display tests after changes; preserve honest native `not_run` evidence.

---

# Execution instructions for this project

## Fixed direction

Build a third-person, single-player coastal exploration game in Unreal for Windows. The opening mission is First Signal. No co-op, combat, hunger/thirst, procedural world, swimming, or boat-driving scope in v0.1. Do not ask the user to reconfirm these decisions.

## Read order

Read `README.md`, `docs/COASTAL_EXPLORATION_FOUNDATION.md`, `docs/UNREAL_SETUP.md`, `docs/INTEGRATION_CONTRACTS.md`, and `VALIDATION_REPORT.md` before modifying the project. Inspect the actual local host project and vendor packages before naming vendor classes or APIs.

## Implementation sequence

Establish engine and asset compatibility, compile the original source plugin, connect real AGIS and Hyper in the systems test room, implement coherent saving, and package that room. Only then assemble the coastal route and finish its UI/art. Record results in the acceptance ledger.

## Non-negotiable engineering rules

Use AGIS as the only inventory authority. A missing adapter is an explicit failure, never successful mock behavior. Do not ship a competing inventory to hide an integration gap. Never remove a world pickup before inventory insertion succeeds. Required items must remain recoverable. Repair consumption must be atomic and idempotent within a campaign.

Persist inventory, transaction ledger, world state, journal, and quest in a coherent save generation. The M1 coordinator adds disk-save source; the quest snapshot helpers alone are not a save system. Keep every mutation inside the coordinator boundary and validate the real provider. Keep the previous valid save when load/write fails. Do not silently start a new campaign after a failed load.

Keep vendor content distinguishable from original work. Use project-owned wrappers where possible. Never commit paid raw assets to a public repository by assumption. Keep credentials out of source and manifests.

Keep files focused and generally below 300 lines when reasonable. Use stable logical IDs. Update JSON and source together when requirements change. Run standalone checks plus real Unreal compile/tests; one does not substitute for the other.

Do not show buttons for unimplemented travel, fishing, construction, or shops. Label development proxies and unresolved references. Do not claim final character art or a playable game when only a source scaffold exists.

## Handoff truthfulness

Report separately: source implemented; source checked; Unreal compiled; vendor integration tested; editor gameplay tested; Windows package tested. Never infer completion from file counts or passing a pure rules test. List actual blockers and exact next steps without inventing a repository, asset paths, measurements, or screenshots.

## Current section

Read docs/M1_IMPLEMENTATION.md, docs/M1_BLUEPRINT_WIRING.md and docs/M1_ADAPTER_CONTRACT.md before continuing. M1 source is implemented but the Unreal/vendor/packaged gate is NOT RUN. Use the native CoastalMissionDirector instead of the M0 generic director. Do not claim editor-generated maps or Blueprint adapters exist until actually created locally.

## Current continuation: M1.2

Read `docs/M1_2_IMPLEMENTATION.md`, `docs/M1_2_UI_WIRING.md` and the current validation report. Native development widgets now exist in source. Use one `CoastalUISessionComponent` on the local PlayerController instead of duplicate hand-wired M1 widgets. Keep AGIS as authority; implement the new read-only `ReadContainerView` from the real provider. No Unreal/vendor/gamepad/Windows acceptance gate has passed here. Do not treat the catalogue's filenames as valid campaign metadata. M2 remains gated on the real packaged test-room loop.

## Current continuation: M1.3

Read `docs/M1_3_IMPLEMENTATION.md`, `docs/M1_3_STARTUP_WIRING.md`, and current validation. The preferred host path is one `CoastalSessionBootstrapComponent` on the same local controller as the UI. Its explicit `StartTestRoom` replaces the three manual binding calls; do not execute both. Real vendor initialization and Hyper focus/Interact wiring remain the host's responsibility. Native startup checks are not runtime acceptance. Strict room checks apply only to the initial authored M1 room. A partial binding failure requires relaunch, not a poison reset. No M2 assembly or engine/vendor/Windows pass is established.

## Current continuation: M1.4

Read `docs/M1_4_IMPLEMENTATION.md`, `docs/M1_4_INTERACTION_WIRING.md` and current validation. One optional controller-owned `CoastalInteractionRelayComponent` routes the actual Hyper target/input. Keep Hyper focus/prompt and AGIS authority. Select exactly one input owner; no direct duplicate world-use handler. Forward fresh selected-target samples every frame and actual input-down/released state in VendorEvents. Bootstrap initializes a present relay before UI. Bridge dispatch now guards same-frame/reentrant actions, but legacy direct calls across frames still require their own pressed-once input. No engine/vendor/Windows gate passed. Do not add another source-only diagnostics layer as a substitute for the actual host integration and packaged M1 gate.

## Current continuation: M1.5

Read `docs/M1_5_IMPLEMENTATION.md`, `docs/M1_5_RECOVERY_WIRING.md` and current validation. Recovery source was brought forward from the foundation's M3 plan, not treated as proof of M1 acceptance. One optional character recovery owner is initialized by bootstrap before UI. It reserves the existing operation gate across a return; no provider mutation or parallel inventory is allowed. Keep the safety actor set fixed, place valid capsule-center markers, and keep gameplay input below its temporary priority-90 blocker. The native menu stays at 100. Preserve actual-transform checks after teleport. Source tests do not establish collision, fades, held-input behavior or packaged play. Do not claim M2 or keep replacing the required real host/vendor integration with another source-only diagnostic layer.


## Current continuation: M1.6

Read `docs/M1_6_IMPLEMENTATION.md`, `docs/M1_6_OPTIONS_WIRING.md` and current validation. Native UI owns one local preference object, separate from campaigns. Text options are automatically connected. One optional `CoastalLookInputComponent` belongs on the fixed character and is initialized by the UI; do not initialize it twice or fabricate input mappings. Opt into host look routing only after routing real separate mouse/stick samples once. Preserve real neutral stick input after permission transitions; never fake neutral on menu close. Keep preference slots separate from campaign slots and preserve previous live settings on an unverified write. Native text scaling does not claim vendor-font integration. No audio/resolution/remapping/sprint or complete accessibility feature was silently added. This advances bounded M3 presentation source, not the unpassed M1 integration gate or M2 coast. The next required gate is still actual host/vendor/Windows execution.


## Current continuation: M1.7

Read `docs/M1_7_IMPLEMENTATION.md`, `docs/M1_7_SPRINT_WIRING.md` and current validation. One optional opted-in character `CoastalSprintComponent` owns only MaxWalkSpeed and consumes actual aggregate host Sprint state once per frame. The existing UI options owner initializes it; do not call twice, install a second movement framework or fabricate release samples. Preserve real release/repress after interruption and stale-input cancellation. Keep its tick running; unexecuted code cannot expire a speed. Foreign property changes disable this owner without overwriting the other writer.

Preferences now READ schema 1 and 2, WRITE schema 2 only on explicit verified save, and keep the existing slot namespace. Preserve old values and default legacy Sprint to Hold; never auto-upgrade on load. Older plugin builds cannot read upgraded slots. Do not store active sprint intent or change campaign schemas. Actual engine/vendor/controller/Windows gates remain unrun. This is the foundation's outstanding sprint-control source, not M2 or evidence that M1 passed. Do not replace the required host integration with another diagnostic-only increment.


## Current continuation: M1.8

Read `docs/M1_8_IMPLEMENTATION.md`, `docs/M1_8_AUDIO_WIRING.md` and current validation. One optional opted-in LOCAL CONTROLLER `CoastalAudioOptionsComponent` owns a private transient mix, initialized/released by the existing UI preference owner. Require three actual independent dedicated classes. No parent/child/passive mixing in this bounded topology; fold Master into each channel once. Do not create fake audio sources, claim asset routing from an opt-in flag, clear unrelated mixes, mutate sound class assets or gate a transcript/mission on audible playback. Void audio API submission is not output verification. Cleanup targets the captured device and may expose the host's underlying mix; keep one owner for its intended lifetime.

Preferences READ schemas 1/2/3 and WRITE schema 3 only on explicit verified save in the existing namespace. Preserve original values and default absent audio to 100%. No campaign/schema/receipt/mission change. Older plugins cannot read upgraded options. Actual Unreal, audible output, vendor/controller/Windows acceptance remain unrun; M1 is not passed and M2 is not delivered. Continue the actual host integration gate rather than treating source diagnostics as playable evidence.

## Current continuation: M1.9

Read `docs/M1_9_IMPLEMENTATION.md`, `docs/M1_9_PLAYBACK_WIRING.md` and current validation. One optional opted-in LOCAL CONTROLLER `CoastalAudioPlaybackComponent` is initialized by the existing UI after its volume routing and widgets. Assign real sources; never fabricate sound assets or treat metadata as audible-output evidence. One local looping bed pauses/resumes under UI and safe return. A radio voice starts once per current, presented top transcript ticket; cancellation/session changes retire it. Never bind audio completion to mission/acknowledgement, autoplay journal voice, or poll IsPlaying to retry dropped/finished/muted sources.

Stop owned voices before releasing their volume mix, including independent routing release. Preserve the guarded pre-release notification and balanced delegate/tick cleanup. No automatic device or source rebind. No campaign/preference schema or settings-row change; current options still write schema 3 explicitly. This is playback SOURCE, not supplied audio content, footsteps, ambient zones, a spatial radio emitter, a native test pass or M2. Actual host/vendor/audio/controller/Windows integration remains the required production gate.
