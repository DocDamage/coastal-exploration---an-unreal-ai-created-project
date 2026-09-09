## Additional M3 asset batch — September 8, 2026

The user added all 16 archives in `G:/coastline/even newer assets and animations`
to M3 planning. See [the integration plan](M3_NEW_ASSET_BATCH.md) and
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

# M3 — Presentation, input and stability

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

## Historical publication checkpoint

Development is paused by explicit user instruction. The GitHub publication is a
source checkpoint, not a completed M3 release. The shooter/editor-input-probe
build succeeded on September 8 at 17:52 UTC; the latest native run is still the
46-test pre-combat run. Combat live checks and four new native combat cases have
not run. The tracer/spread mismatch and shot-cooldown clock review findings need
follow-up; their private vendor patch was interrupted at the pause. Corrected
swimming seam and Morbid live acceptance also remain pending. All 66 Python tests
passed when preparing this source publication. See AGENTS.md for resume context.

## Verified integration increment - September 8, 2026

The isolated UE5.8 host passes all46 native tests (45 clean, one existing AGIS
warning), plus the seven-record old-save read-only load, all seven destination
Bridge/journal interactions, quest save/reload, and old/completed campaign switching
with correct optional-state reset/restoration and unchanged save hashes.
Evidence: `../local-evidence/m3-destination-*-ue58.json`.

Day/night drives the existing sun/fog and passes native menu pause/resume checks.
Its clock remains transient. PCGEx schedules six north-trail grass instances;
near generation, distance cleanup, and deterministic regeneration passed.
Original grass actors are editor-only authoring references.

Shooter recheck found the actual Fab source package: 123 files / 187311088 bytes
verified. The earlier public demo was binary-only; that finding does not apply
to this actual package. Source and project combat wrapper are enabled only in
Host58 and compiled successfully. Original UE5.7 remains preserved. The shared build includes
a native water-edge overlap fix and shelter pause-menu correction found by live
checks. Combat, corrected seam, and Morbid runtime acceptance remain pending.
See AGENTS.md for the active checkpoint.

## Expanded scope — September 8, 2026

### UE5.8 migration trial

The user asked to upgrade for the newly selected features. UE5.8.1 is already
installed at `C:/Program Files/Epic Games/UE_5.8`. An independent project copy at
`../LocalHost58/CoastalExploration` preserves the working `../LocalHost` UE5.7.4
project, its content, saves and original graphics/window preferences.
Custom plugins are actual copies, without shared writable junctions or hardlinks.
DayNightSystem is rebuilt from source as a trial-local plugin.

The complete UE5.8.1 editor build passed all 168 actions
(`../local-evidence/m3-ue58-baseline-build.execution.json`). No game C++ API fixes
were required. The trial Target.cs files required V7 build settings and Unreal5_8
include order because the installed editor rejected V6's shared-build settings.
The successful run used `-UBANoDetour -MaxParallelActions=2`; content copying was
paused to remove severe disk contention. The downloaded shooter/health plugins'
BuildId also matches installed5.8.1, but they have not been loaded/integrated.

The independent 24.12GB asset copy is complete: all 10,904 expected files exist
with matching sizes. Two Fisherman's Cabin source junctions were materialized
as independent trial folders. All 43 baseline native tests passed (42 clean,
one existing AGIS warning case; zero failures), with original critical file hashes
unchanged. Evidence: `m3-ue58-native-tests.execution.json` and the corresponding
native report. The trial editor opened First Signal with all 2369 actors, no
dirty packages, and a map check reporting zero errors/warnings. The copied-save
baseline PIE smoke now PASSES: old swimming campaign loads at generation12,
player mesh and animation Blueprint are valid, UI initializes, walking mode and
swimming/camping/recovery components are present. Loading wrote no trial saves;
all28 original critical file hashes remain unchanged. Evidence:
`m3-ue58-baseline-live.json`; recipe `tools/unreal/check_ue58_baseline_live.py`.
An initial helper-only failure called an Objective method not exposed by the
baseline build; removing that invalid introspection call allowed a clean rerun.
Test PIE is ended. Actual swimming, combat, quests, visuals and package acceptance
remain separate gates; this is baseline migration compatibility evidence.
Compilation and native tests do not establish Blueprint, old-save or gameplay
compatibility. Tools `prepare_ue58_trial.py`, `copy_ue58_content.py`,
`build_ue58_trial.py`, `check_ue58_trial.py`, and `launch_ue58_trial.ps1` reproduce
the isolated process. Retain the old host until runtime acceptance passes.

Destination quest source and four authoring/live-check recipes are ready in the
repository. Terra's independent review found a public mission-completion flag
could unlock a destination without the required acknowledgment journal entries;
Sol fixed the common offer/execution predicate and added regression assertions.
Configured bridge mutation rejection and real coordinator switch/rollback still
need runtime evidence. These new quest changes are deliberately absent from the
baseline trial build so engine migration can be checked against the prior game.
`sync_ue58_source.py` copies the reviewed source into the independent trial for
the next build, including swimming collision validation and the shelter recovery
guard. No destination quest actors have been authored yet.

### Requested feature expansion

The latest user request explicitly adds all-water swimming, playable combat,
the German Shepherd as a trusty sidekick, destination quests/interactions,
Morbid animation gameplay, richer dressing and scan/terrain edge blending.
Atlantis Ruins and Modular SciFi Station acquisition/assembly remain selected.
The complete additional 24-asset request is in `data/m3_requested_assets.json`;
all identities are now known (the first three were confirmed by the user).

Downloads and installations are authorized. The PCGEx Scheduling Policies source
was downloaded from the publisher-linked MIT repository to
`../LocalVendor/PCGExSchedulingPolicies`, revision
`54ac7cb6f7959db63e6aa84b10d3851761880b12`. It currently declares UE 5.8 and
publishes no 5.7 branch/tag; it is not enabled or claimed compatible with this host.
The requested DayNightSystem 1.0 already exists in UE 5.7's Marketplace directory.
Its host plugin entry is now enabled; scene wiring and runtime checks follow the
combined swimming/shelter build. Easy Custom Notifications advertises UE 5.8;
its compatibility also needs checking after acquisition.

Fab acquisition is incomplete: the browser and Computer Use kernels both fail
initialization with `failed to write kernel assets` / path not found, while the
Playwright connector cannot find its Chrome extension. Native Launcher control
and authenticated Fab downloads are therefore unavailable in this session.
No paid purchase has been made. These are access/runtime blockers, not an approval
request and not evidence that any remaining asset has been installed.

The shooter publisher's linked documentation and 644,259,440-byte demo have now
been downloaded independently to `../LocalVendor/AdvancedNetworkedShooter`.
`ShooterDemo.zip` has SHA-256
`c2efc98cc880402c8a44f2f0f2ae41bcae9fb886dc6f919a1e7be3f14cb47cb3`.
It contains AdvancedShooterSystem and AdvancedHealthSystem, both declaring UE 5.8,
with binaries and generated intermediates but no original C++ Source folders.
The project itself declares 5.8. They are staged, not enabled in this 5.7.4 host.
Descriptor inspection and download provenance are beside the archive.

The combined all-water/shelter native batch compiled successfully in 438.11 seconds
(`../local-evidence/m3-expanded-editor-build.execution.json`). All 66 Python tests
and 43 native Coastal tests pass; one existing AGIS case retains vendor warnings.
The later live water check exposed a PhysicsVolume transition failure leaving the
sheltered basin despite successful global-water samples. The narrow fallback fix
is being implemented and requires the next native build and renewed seam check.
These passing native tests do not establish complete all-water traversal acceptance.

Implementation sequence: extend water coverage; bind Morbid shelter action;
wire installed day/night lighting; then add a saved destination investigation
chain using real interactable records and backward-compatible campaign restore.
Combat/targeting/health, companion follow/wait/regroup, rope/traversal, environmental
audio/weather and the other selected animation packs follow actual asset inspection.
Retain single-player, AGIS inventory, one interaction/input authority and First Signal.

Started September 7, 2026. Workspace: `F:/coastline`. The former C: compatibility junction has been removed; `C:/dev/Astra Project` is now a separate empty directory. Work on the source in `F:/coastline/CoastalExploration` and the host in `F:/coastline/LocalHost/CoastalExploration`. M2's First Signal coast and Windows archive remain the baseline. Gameplay/device acceptance remains deferred by the user's prior direction until the integrated continuation described below.

## First increment: native UI presentation

The existing UMG menu and HUD now share a coastal palette: dark blue-green surfaces, warm paper text, amber notices and a lighter teal focus state. Command buttons have explicit normal, hover, pressed and disabled brushes with padded touch targets. The objective panel wraps within a compact width that follows text scaling and the viewport. Campaign and inventory copy now describes player actions instead of exposing development/provider terminology.

UI ownership remains with `UCoastalUISessionComponent`. Panels pull their content on refresh and retain the existing tick updates for notices, countdown and focus. The HUD retains its 0.2-second refresh. Existing modal input ownership, command IDs, enabled predicates, scroll navigation, focus restoration and transcript/display paint gates are unchanged. Styles are copied into widget-owned storage; this increment adds no delegates, timers, popups, save data or teardown requirements. The existing session teardown still removes widgets and releases its bindings.

The HUD width is bounded by available local viewport width after its left position and border padding. Menus retain their proportional canvas bounds, wrapping text and scroll container; larger padded actions remain scrollable. Actual composition at supported resolutions and text sizes remains to be inspected when UI testing resumes. This is an initial presentation pass, not final accessibility or art acceptance.

## Second increment: graphics drafts and input presentation

The existing display owner now holds an independent graphics draft. The Display & Graphics menu offers Low/Medium/High/Epic presets, VSync, and 30/60/90/120/144/165/240/unlimited frame caps. Opening, cycling and leaving a draft do not apply settings. The full existing scalability snapshot and custom frame cap are retained until their respective controls are edited. A selected preset uses the installed engine's quality mapping, including render scale.

Apply uses `UGameUserSettings::ApplyNonResolutionSettings`; it does not call `ApplySettings` or the window-resolution API. This engine path also reapplies current dynamic-resolution, audio-quality and HDR preferences. Apply and save explicitly requests `SaveSettings`, whose void result does not verify disk IO. The engine settings file is shared: a later explicit engine-settings save can also persist earlier session-applied graphics. No campaign or Coastal player-options schema changes were introduced.

Graphics commands require the existing opted-in Windows standalone display binding, no active trial/restoration, and an unchanged engine-settings snapshot. Settings-object changes outside this menu require Refresh. This does not detect every console-variable or driver override or prove effective FPS/VSync. The existing display trial and its paint/deadline checks remain the window-change authority.

The UI now owns a weak-reference Slate input observer registered with menu-input setup and removed during teardown. It observes without consuming input. Meaningful analog motion, key presses, mouse movement and clicks choose native menu/HUD hints; neutral analog values and tiny cursor deltas do not switch them. Gamepad hints use positional face-button names. Hyper's own prompts are unchanged; platform-specific icon artwork and input remapping remain outstanding.

Tab/Shift-Tab now include the campaign-name field, whose focus can be restored after panel refresh. Gamepad/D-pad navigation continues through enabled commands, including automatic fresh-name generation. Text entry still receives Enter/Space while the field has focus. All existing gamepad Back and command gates remain in place.

The [acceptance cases](../data/m3_graphics_input_acceptance.json) record pending native scenarios; none have been run. Engine API signatures and behavior were inspected in the installed UE 5.7 source. All 52 existing Python tooling tests and 3,354 source consistency checks pass (`local-evidence/m3-batch2-python.log` and `m3-batch2-source-check.log`); these do not execute the new native graphics or input implementation. Native compilation and visual/device testing wait for an integrated checkpoint under the user's batching direction.

## Third increment: inventory and journal reading

Inventory presentation is separated into `CoastalUIInventoryPresentation.cpp`. It displays occupied-space totals calculated from validated AGIS footprints, a selected-item description, contextual transfer labels and up to six directly selectable item rows. Previous/Next continue through the entire view and follow the selected page. Free-space totals are informational; AGIS still decides whether an item's shape fits. No icon assets or replacement inventory have been introduced.

Direct selection commands contain the existing item-instance GUID. The command gate first checks that the row was offered and enabled, then selection rereads AGIS and resolves that GUID in the refreshed selected container. A disappeared item produces a notice, not an index-based replacement. Transfers still use the existing refresh/revision check, bridge, operation-ID retry and coherent-save boundary. Pending transfers prevent changing selected items or sides.

Journal presentation now offers a title index and shows one discovered entry at a time beside the current objective. Titles are localized through the story library. Reading state is transient and cleared with the UI session; only IDs returned by the current save coordinator are selectable. The radio transcript acknowledgement path is unchanged, and journal reading does not play audio or complete the mission.

Selection refreshes return the scroll to the reading text. Refresh also restores focus by command identity when that command remains enabled, with the existing valid-index fallback. The underlying modal owner, scroll container, paint gates and no-mouse-required controls remain intact. See the [pending native cases](../data/m3_inventory_journal_acceptance.json); layout, physical-device behavior and native compilation remain unrun for this batch.

## Remaining M3 work (current)

## Swimming continuation — September 8, 2026

This increment adds surface swimming in the sheltered basin west of
the existing pier. A crosswalk joins the pier at `(12500, 0, 250)` to a timber
ramp at `(11750, 0, 250)`. The ramp slopes to `(11750, -2400, -250)` beneath the
existing water surface at Z `-90`; eight floating markers identify the basin.
The original terrain, pier deck collision, seven WorldObject IDs and dry
checkpoints are retained. One 300 cm west handrail segment is hidden and set to
NoCollision to open the access between its existing piles. The crosswalk is
260 cm wide and its landing depth is 240 cm; these dimensions avoid a raised
edge blocking the return up the ramp. The first two native route checks exposed
those obstacles, and their failed reports are retained. This is an initial
swimming area, not final environment polish.
Visual review also moved three original shoreline rocks west of the marked basin
where their bounds protruded through the ramp. Their exact before/after transforms
are in `m3-swimming-approach.json`; terrain geometry is unchanged.

`UCoastalSwimmingComponent` owns the temporary mesh/animation and movement tuning;
`ACoastalSwimmingZone` is a real `APhysicsVolume` using native `MOVE_Swimming`.
The player keeps the existing horizontal movement input. Walk down the ramp to
enter and up it to leave; jump does not force an artificial water exit. The
supplied Quinn Simple mesh and idle/forward clips share the same inspected
SwimmingAnimationPack skeleton. Properties on the player Blueprint select those
assets; reusable native code contains no vendor asset paths.

Bootstrap initializes the optional owner against the actual interaction bridge.
Campaign/input/pause/session and animation ownership checks bound its lifetime.
Only a valid active swim inside the authored volume and depth limit exempts the
player from DeepWater recovery. Other hazards take precedence regardless of actor
iteration order. Swimming ticks before recovery and releases presentation before
recovery relocation. Normal locomotion returns after exit; no campaign schema or
inventory authority changes were introduced.

Reproduction: build the native host and editor helper, run
`tools/unreal/build_swimming_approach.py`, then
`tools/unreal/configure_sheltered_swimming.py` in the First Signal editor outside
PIE. The latter calls the project-owned editor helper
`BuildSwimmingZoneBox`, which uses Unreal's actual volume brush factory so
immersion queries and cooked collision have real brush geometry. The helper is
mirrored in `tools/unreal/host_editor/CoastalHostEditor`. The host character owns
one native swimming subobject, enables `bCanSwim`, and lets the swimming owner
consume jump while active. Preserve those local host hooks.

Evidence is under `../local-evidence/m3-swimming-*`. The map, player Blueprint
and 22 existing save files were backed up or hash-recorded before this increment.
Seven ramp collision samples confirm the slope. All 66 Python tests pass.
The first UBA build was stopped after no completed compiler actions and retried
with the command-line `-NoUBA` switch. The retry also waited on the 2.416 GB
memory-mapped UnrealEd PCH on F: under memory pressure. A sequential read warmed
that file, after which compilation progressed. Keep concurrency bounded; neither
project configuration nor compiler-cache location changed. One compiler error
(a local name shadowing `ABrush::Brush`) was corrected. The final editor build
passed in `m3-swimming-editor-build-fixed.execution.json`.
All 43 native Coastal tests passed in `m3-swimming-native-report/index.json`:
42 without warnings, one AGIS capacity test with existing vendor GameplayTag
warnings. The two new swimming tests cover unavailable defaults and transition
failure rules; these do not substitute for the live ramp, pause and recovery checks.

The final scripted route passed in 35.641 seconds after those placement fixes:
11 waypoints cover the original dock, crosswalk, ramp, four actual
`MOVE_Swimming` samples and the return to `MOVE_Walking` on the pier. The original
mesh and AnimBP are verified restored at the end. Separate interruption checks
passed pause/resume, stationary pause presentation, save while swimming,
reload onto the existing dry checkpoint and native movement out of the bounded
zone followed by recovery. The wet save was generation 7 and reloaded at 8.
Live presentation readback confirms the supplied idle clip and the forward clip
at 300 cm/s. These are scripted native checks, not physical keyboard/gamepad or
Windows packaged acceptance. The final view is `m3-swimming-final00000.png`.
Final preservation checks confirm all 22 pre-existing save hashes and seven
WorldObject IDs are unchanged. The saved map contains 2,368 editor actors with
no dirty map/content packages; Unreal is outside PIE showing the basin and its
original background throttling is restored. The source gate passes 3,579 checks
with zero failures in `m3-swimming-source-final.execution.json`.

Remaining work: authored Morbid animation use, destination dressing and scan/terrain
edge blending, physical device traversal, performance and M3 Windows packaging.
Atlantis and Modular SciFi Station still require their Fab Unreal downloads.

The September 8 all-water continuation extends native surface swimming across the
single authored `Cove and open sea` plane inside the established First Signal
safety perimeter. A lower-priority coastal water volume covers X `-34700..45700`,
Y `-41700..34700` and Z `-690..-90`. The priority-10 sheltered basin, its ramp and
markers remain. The coastal volume uses priority 5 so the basin retains local
authority. A 300 cm strip before the southern edge of `Deep sea` removes the
swimming exemption while the existing hazard still overlaps the player; the west,
east and north edges meet their existing `OutOfBounds` walls. Water outside those
limits remains visible scenery: 10,300 cm west, 9,300 cm east, 15,300 cm north and
8,300 cm south including the transition strip. Later underwater ruins placed
beneath this plane inside the perimeter inherit swimming automatically. A new water
surface outside it requires its own bounded volume and safety review.

`tools/unreal/configure_all_waters_swimming.py` derives these bounds from the saved
water and safety actors, creates a real 80,400 by 76,400 by 600 cm brush, and rejects
geometry drift. It also preserves and checks all 24 campaign-save hashes and the
seven WorldObject IDs. Configuration evidence is
`m3-all-waters-configuration.json`; the saved map has 2,369 actors and no dirty
packages. The combined editor build and all 43 native Coastal tests passed before
authoring. Eleven dry checkpoints plus a collision-derived low shore above Z `-90`
stayed dry. A submerged shore probe and water near Powell Dock, Industrial Harbour,
the sea platform, southern open sea, western open sea and eastern open sea all used
native `MOVE_Swimming`, retained deep-water protection and moved about 199 cm from
the existing input in one second.

That first all-water PIE pass also exposed an Unreal overlap transition defect:
after leaving the higher-priority basin, the capsule selected `DefaultPhysicsVolume`
for several frames instead of the still-overlapping coastal brush. The failure is
retained in `m3-all-waters-live.json`; it is not accepted as a seamless transition.
Source now resolves only this active-swim/default-volume case by selecting the
highest-priority valid Coastal swimming brush that actually overlaps the capsule.
It does not replace foreign volumes or create a fallback on dry land. The correction
also hardens the engine-owned path: Unreal 5.7 and 5.8 select a primitive's physics
volume from cached overlaps only when both sides generate overlap events and use
query collision. The zone constructor and editor brush helper now require the
`OverlapAllDynamic` profile, `QueryOnly`, generated overlaps and an `Overlap` Pawn
response; authored validation rejects every missing condition. The next recipe run
rebuilds both coastal and sheltered brushes with that contract. The correction and
updated shore/ramp jump guidance await the next batched native build, followed by
the hardened report's minimum 60 moving frames in each seam direction, captured
volume-change events, and west/south recovery checks. The original failure report
is preserved separately. Physical device and Windows packaged acceptance remain
separate.

## Omitted packs continuation — September 8, 2026

All four previously omitted local packs are now installed in the private host:
Haunted Prison (481 files), Medieval Italian Village (3,247 files including its
2,696 vendor-owned external actors), Swimming Animations (293 files) and Morbid
Motions (380 files). The importer copies selected Content roots, validates CRC
and existing-file SHA-256, and publishes without overwriting a different target.
Unrelated template content and external actor trees were excluded. An original
import survived a tool timeout; its competing retry was detected and stopped.
The successful import reports distinguish new writes from verified existing files.

First Signal now contains a six-cell prison assembly connected to the harbour,
and three vendor-authored Italian houses around a paved terrace connected to the
existing village trail. Both have dry arrivals and persistent actor tags for
repeatable authoring. The original seven WorldObject IDs and campaign schema are
unchanged. These are initial destination assemblies: edge blending, denser
street dressing, destination-specific interactions and final presentation remain.
The village retaining wall uses a project-owned world-space stone material to
avoid stretched vendor UVs; originals were not edited.

Scripted CharacterMovement traversal passed from the harbour through two prison
cell entrances (39.5 seconds), and from the trail around the village courtyard
(41.8 seconds). The disposable `coastal_test_m3_omitted_0908a` campaign reloaded
successfully after adding the village. These checks exercise real movement and
collision; they are not physical-device input or packaged acceptance. No native
C++ changed and no new native build or Windows package was produced. All 66
Python tests pass. The source/data consistency check also passes; its execution
record is `../local-evidence/m3-omitted-source-final.execution.json`.

Unreal registered 218 Swimming clips and 275 Morbid clips plus 28 pose assets.
Two clips from each pack were loaded and their durations/skeletons recorded in
`../local-evidence/m3-omitted-animation-samples.json`. Each pack has its own UE5
skeleton asset. At this import checkpoint neither pack was bound to the player.
The swimming continuation above now implements and checks its player binding,
entry/exit and bounded recovery exception. Morbid gameplay use remains pending.
Atlantis Ruins and Modular SciFi Station still need their Unreal downloads.

Recipes: `tools/unreal/build_prison_destination.py`,
`tools/unreal/build_italian_village.py`, and
`tools/unreal/fix_village_terrace_material.py`; shared authoring helpers are in
`tools/unreal/m3_destination_authoring.py`. House art reports are exported using
`tools/unreal/inspect_vendor_destination.py` with the three
`/Game/ItalianMedievalTown/Levels/Level_Instances/LI_House_0{1,2,3}a` maps.
For a fresh village build, apply the retaining material recipe after placement.
Traversal and PNG evidence is under `../local-evidence/m3-omitted-*`.
The pre-edit map backup is `m3-omitted-before-L_FirstSignal.umap` in that folder.

Automatic approval review rejected removal of the 1 MiB import orphan
`../LocalHost/CoastalExploration/Content/HAUNTED_PRISON/Textures/.extract-kvrtpi9r.tmp`
as “blocked by policy,” with no more specific reason. It remains excluded from
all asset counts. The 3.66 GB staging ZIP also remains at
`C:/Users/Doc/AppData/Local/Temp/coastline-import-medieval-f76a60cbb1b940db8b1cf8612df610e7/MedievalItalianTown.zip`;
its original archive on F: is untouched. Do not retry the blocked cleanup via
another mechanism.

Final preservation check: all 20 pre-existing save files retain their SHA-256
hashes, and all seven WorldObject IDs are unchanged. Final source validation
passes 3,535 checks. Unreal is left outside PIE on First Signal, camera facing the
village, with zero dirty map/content packages and background throttling restored.
Final inspected images: `../local-evidence/m3-omitted-prison00000.png` and
`../local-evidence/m3-omitted-village-final00000.png`.

### Scope correction — September 8, 2026

The seven-destination/Camping list below was incomplete. The user also selected
**Swimming — Animations**, **Morbid Motions — Animations**, and **Haunted Prison**.
Their Unreal archives were already present in `F:/coastline` and were overlooked:
`AnimSwimPack.zip`, `MorbidMotions.zip`, and `HauntedPrison.zip`. Archive inspection
confirmed the `SwimmingAnimationPack`, `MorbidMotions_Pack`, and `HAUNTED_PRISON`
content roots. Their completed imports and current gameplay status are recorded
in the continuation above; Camping integration does not implement these packs.

`MedievalItalianTown.zip` is also present, with the `ItalianMedievalTown` content
root. The user confirmed **Medieval Italian Village** is the fourth omitted item.
All four are selected for M3; remaining integration is recorded above. The old list is not
exhaustive. No additional download is required for these four archived packages.

Swimming animation integration requires an explicit playable water transition
that cooperates with the current deep-water recovery owner; copying clips alone
does not implement swimming. The older no-swimming first-build notes must not be
used to omit this new request. Morbid motions still need an authored gameplay
use; the prison assembly/route now passes scripted walking, with final presentation
and packaged acceptance still pending. The
current First Signal map and save schema remain the baseline during this work.

### Destination and camping continuation — September 7, 2026

The user explicitly selected all seven named destinations and Camping Animations.
Five destinations are now assembled as scenery in the saved `L_FirstSignal`:
Hallsands, Baelo Claudia, Powell Dock, Industrial Harbour and Abandoned Sea Platform.
The first two use normalized owned survey scans; Powell preserves a selected
authored dock cluster; Harbour and Sea Platform use focused modular assemblies.
Five original connecting routes, arrival signs, six query-only dry checkpoints
and an overlook campsite have been added. Collision measurements exposed and
corrected the two initial scan-approach height/extent gaps. Destination gameplay,
physical traversal and final environment presentation are not yet accepted.

Atlantis Ruins and Modular SciFi Station are confirmed owned in Fab. Their Unreal
files require Epic Launcher/Fab plugin access; current browser controls cannot
operate that native download UI. Those two remain acquisition/assembly work.

Camping's 23 clips are installed. Two player actions (`Warm hands`, `Rest by the
fire`) are implemented through the existing pause UI near `Coastal.Campsite`
markers. A character component validates dry capsule placement and a swept path,
temporarily uses the matching Camping Quinn mesh, and restores the prior mesh and
AnimBP on completion/cancellation. Movement, jump, menu, load and recovery paths
cancel presentation without adding inventory or save mutations. Independent
review caught an overly tight vertical movement tolerance; it now permits UE's
normal floor-gap adjustment while retaining a tight lateral movement limit.

The integrated UE 5.7.4 editor build passed in 227 seconds:
`../local-evidence/m3-expansion-editor-build.execution.json`. All 61 existing
Python tests passed during source integration, and 3,487 source consistency
checks pass after distinguishing portable-package fields from local host
observations in the asset register. Both actions start through the native pause
menu, play through completion, and restore the original Quinn mesh and unarmed
AnimBP. Movement, jump and menu cancellation were checked in PIE. All six new
arrival points pass native dry-placement checks and remain at their destinations
after game ticks. A fresh disposable campaign starts and continues successfully.
The original seven WorldObject IDs remain intact and all 18 pre-existing save
files retain their SHA-256 hashes. Physical traversal, load/recovery cancellation
and packaged acceptance remain separate checks.
The three actual host character/module hooks are under
`../LocalHost/CoastalExploration/Source/TP_ThirdPerson`; preserve them with the
project-owned plugin component. `/Game/CampingAnimations` is in the host cook list.

Private content remains outside this source repository. Recipes are
`tools/prepare_expansion_scans.py`, `tools/extract_expansion_content.py`, and the
`tools/unreal/*expansion*.py` scripts; layout is `data/m3_expansion.json`.
The original map backup is
`../local-evidence/L_FirstSignal-before-destination-expansion.umap`.
PIE screenshots are `../local-evidence/m3-expansion-pie-*.png`; the corrected
Baelo atlas is shown in `m3-baelo-texture-final00000.png`. Its FBX material had
lost the texture link, so `repair_baelo_material.py` explicitly imports the owned
JPEG and binds it to a rough surface material. The tent was reduced to campsite
scale and `expand_safety_boundaries.py` moved the old walls outside the new routes.
Arrival results are in `m3-expansion-pie-arrivals.json`; animation results are in
`m3-camping-live.json`. These are initial assemblies: exposed scan boundaries,
plain connecting surfaces, sparse worksite dressing and pose grounding still need
an environment/presentation pass.

The live native suite reported 39 successes and two AGIS initialization failures
because PIE was accidentally started while the suite ran. A clean isolated rerun
passed both AGIS cases (`m3-expansion-agis-report/index.json`, one with existing
vendor tag warnings). Keep PIE stopped throughout native automation. Menu and
render stalls exposed repeated checkpoint saves during one stationary visit;
a focused recovery fix retains the recorded visit across transient interruptions
while requiring fresh dwell and eligibility afterward. Real departure and session
changes still reset it. The recovery core has 162 passing checks; the full
standalone core suite passes. Its follow-up editor build passed in 87 seconds
(`m3-expansion-checkpoint-build.execution.json`), and 3,493 structural checks pass.
All four native recovery tests pass (`m3-expansion-recovery-report/index.json`).
The rebuilt live campsite retained generation 39 through two pause/back cycles
and a screenshot (`m3-checkpoint-live-regression.json`). An earlier icons campaign
loaded successfully through the real coordinator at generation 7; the disposable
expansion campaign was restored before the next tick, and all 18 original save
hashes remain unchanged (`m3-expansion-old-save-load.json`,
`m3-expansion-save-preservation.json`). The editor remains open, with its background
throttle restored and the original F: compiler cache restored.
High-resolution editor capture forced long resource-streaming stalls. Use
`capture_expansion_pie.py` with an active disposable campaign for routine captures.

Scope correction, September 7: the user explicitly reminded us that requested
new places and new animations belong in the outstanding-work discussion. The
previous M3 summary omitted these workstreams by relying on the narrower opening
handoff. Track destination expansion and animation integration explicitly; do
not silently treat them as completed or defer them based on the old v0.1 plan.
The user subsequently explicitly instructed “well, add those”: all seven named
destinations are now selected for M3—Hallsands, Baelo Claudia, Atlantis Ruins,
Powell Dock, Modular SciFi Station, Industrial Harbour, and Abandoned Sea Platform—
along with Camping Animations. The older version-based deferrals are superseded.
Acquisition, assembly, player integration and acceptance are tracked separately;
being owned or copied into Content does not mean a destination is playable.

The fourth batch constructed the water/dock/shoreline pass in the actual editor map, including a basin beneath the seaward pier. The fifth added layered ground surfaces, rail ballast and dock visual cleanup, then repaired a rock-normal sampler mismatch in project-owned materials. Four corrected editor captures were reviewed. See [environment construction](M3_ENVIRONMENT.md) for the saved assets, recipes, backups and evidence boundaries. This asset work did not compile the pending native M3 changes.

| Area | Next implementation or evidence |
|---|---|
| New destinations | Import, assemble and connect all seven selected places with usable routes and save continuity; then integrate destination gameplay. The existing cabin/trail/dock/overlook is only the opening area. |
| New animations | Camping Animations is copied into the host (23 clips). Inspect skeleton compatibility and integrate usable player actions with the existing UI/input owners. Copying the pack is not animation acceptance. |
| UI and input | Inventory rows, journal index, native device hints and battery/fuse icons are implemented. Inspect panels at supported text sizes; finish vendor prompt polish, intended remapping and physical keyboard/controller navigation. |
| Graphics | Quality/VSync/frame-cap source is implemented; compile and inspect actual application/persistence at the integrated checkpoint. Preserve display Keep/Revert safety. Broader controls in historical documents are candidates, not automatically new requirements. |
| Character and camera | Inspect available suitable character assets, replace the mannequin when an actual compatible asset is available, tune cabin camera clearance and comfortable traversal. |
| Environment | Water, dock-board detail, a pier basin, initial shoreline dressing, layered ground and rail ballast are saved. Continue broad dressing/radio refinement and inspect physical traversal at the later checkpoint. |
| Audio | Refine installed ambience/radio/effects, add appropriate footsteps and environmental transitions after checking actual available assets. |
| Stability and performance | Profile the finished route on target hardware; inspect frame times, memory, save hitches, recovery and repeated-load behavior when testing resumes. |

The real AGIS/Hyper owners, coherent save coordinator, `level.first_signal` identity and existing save files remain intact. No purchased assets are added to the source repository. M3 is in progress; M4 packaging and complete delivery acceptance remain later work.

## Integrated continuation — September 7, 2026

The accumulated M3 source now compiles successfully under UE 5.7.4. The native
editor build completed with exit 0 in 83 seconds after moving only its generated
2.4 GB UnrealEd precompiled-header cache temporarily to C:. Evidence is
`../local-evidence/m3-integrated-editor-build-ssd.execution.json`. The first F:
attempt was stopped for measured disk contention; it is not a compile pass.
The original cache is restored on F:. Automatic approval review rejected deletion
of the temporary C: copy, which remains at
`C:/Users/Doc/AppData/Local/CoastalM3BuildCache/UnrealEd`, with an unused
`UnrealEd_M3TemporaryLink` beside the restored original. State is recorded in
`../local-evidence/m3-ssd-cache-state.json`.

All 40 native Coastal automation tests pass (`../local-evidence/m3-native-report/index.json`),
as do all 61 Python tooling tests (`m3-integrated-python-final.log`). The existing
Codex MCP client now connects to this host through the project-local Python
listener; see [Unreal MCP setup](UNREAL_MCP.md). Live editor identity, state,
Python execution, map import/save and PIE control were exercised. The listener
also guards nested Slate ticks during long import/save operations and reports
unserializable results as correlated protocol errors.

The first actual First Signal startup exposed two map defects hidden by the
earlier visual-only checks: the terrain faces pointed downward, and decorative
cabin collision obstructed unrelated locations. The native preflight correctly
rejected the unsafe spawn/return points. That failed run is retained in
`m3-firstsignal-campaign-new.log`; its manual exit 0 is not a successful campaign
test. The existing command-line campaign fixture also assumes systems-room
positions/heights and is not the evidence for the following coastal run.

`repair_m3_collision.py` created a project-owned terrain asset with reversed
winding and disabled collision on exactly five diagnosed decoration instances:
three tray/box components, another tray near the radio, and the cabin cobweb.
Vendor assets and structural walls/floors remain unchanged. The original map is
backed up as `L_FirstSignal-before-m3-collision-repair.umap` in local evidence.
The final terrain is `/Game/Coastal/M3/Geometry/SM_M3CoastalTerrain_CollisionFixed`.
Three downward checkpoint traces now hit dry floors with upward normals, and a
fresh PIE session passes the real startup preflight. The shared OBJ writer now
reverses winding when reflecting its coordinate basis, with a geometric regression
test. The fresh cabin builder excludes the diagnosed unsuitable decorative mesh
colliders so regeneration preserves the fix.

A dedicated `coastal_test_m3_live_0907a` campaign was started through the native
menu. Scripted, dry-validated positioning and the existing interaction bridge
successfully inspected the radio, collected both supplies, repaired the radio,
presented/acknowledged the transmission, and saved generation 9. The current
backpack UI displayed both supplied items before repair. After ending PIE and
starting a fresh session with the saved map, Continue restored generation 9,
radio repair and message acknowledgement, with the consumed inventory still empty.
See `m3-live-campaign-validation.json` and the `m3-live-dock00000.png`,
`m3-live-transcript00000.png`, and `m3-live-restored00000.png` captures.
All 14 pre-existing save files retained their SHA-256 hashes
(`m3-user-save-verification.json`).

This establishes a running editor game and a scripted core mission/save/reload
path. It does not pass physical traversal, controller navigation, graphics
persistence on a non-editor Windows build, audio-output, complete M3 presentation,
performance or packaging acceptance. The M2 archive remains unchanged. Continue
the remaining M3 table above using the repaired map and current native binaries.

## Inventory icon continuation — September 7, 2026

The native backpack/storage item buttons now include original battery/fuse icons.
`CoastalUISessionComponent::ItemIcons` holds the textures for the UI lifetime;
presentation resolves the stable AGIS item ID and keeps the existing instance-GUID
commands. The same button remains the focus/input owner. Missing optional images
leave usable text-only rows. The final layout uses 48px image boxes and adjacent
left-aligned item names; other command labels retain their centered layout.

Original PNGs and regeneration instructions are in `art/ui/icons`. The editor
importer `tools/unreal/import_item_icons.py` creates `/Game/Coastal/UI/Icons`, applies
UI texture settings and adds that directory to the actual host's cook rules.
These are original project assets, not vendor redistributions.

Live scripted checks in the dedicated `coastal_test_m3_icons_0907a` save verified
both real AGIS items in the backpack, both icon resources in the native widget
tree, text-only fallback, and a battery transfer into cabin storage and back.
The storage check exposed one decorative book intercepting the cabinet's
interaction trace. `repair_storage_visibility.py` changes only Visibility to
Ignore on `StaticMeshActor_780` / `Cabin_SM_Books_175`; its physical collision is
retained. The fresh map builder includes the same exact decoration rule. The map
backup and repair readback are `L_FirstSignal-before-storage-visibility.umap` and
`m3-storage-visibility-repair.json` in local evidence. No cabinet authority,
world ID, vendor mesh or campaign schema changed.

The icon integration compiled in `m3-icons-editor-build.execution.json`; all 40
native tests passed in `m3-icons-native-report/index.json`, all 61 Python tests
passed, and 3421 structural checks passed. A subsequent visual-only size/alignment
adjustment compiled in `m3-icons-layout-build.execution.json`. Live readback caught
that UE 5.7's `UImage::SetDesiredSizeOverride` is ignored before its Slate widget
exists. The final implementation persists the size through the brush and compiled
in `m3-icons-brush-build.execution.json`. All 16 pre-existing
save files retained their hashes (`m3-icons-save-verification.json`). Physical
input, supported-resolution/text-size coverage and Windows packaging remain
separate pending gates; the M2 archive is still unchanged.

Fresh-session native readback confirms both image widgets have 48 x 48 desired
sizes (`m3-icons-final-readback.json`), and the final rendered backpack capture is
`m3-icons-backpack-final00000.png`. The saved cabinet trace fix was exercised
successfully after restarting the editor. Final structural validation reports
3425 checks with zero failures (`m3-icons-final-source-check.log`). The editor was
left outside PIE with the repaired map saved and MCP connected.

## Initial source evidence (historical)

The initial native build was cancelled when the user requested batching work instead of building after each feature. Its attempt is recorded in workspace `local-evidence/m3-ui-editor-build.execution.json` and its log; no compile pass is claimed for this increment. Use lightweight source checks between features and native builds at integrated checkpoints. Runtime UI inspection, gameplay, physical-device and performance acceptance are deferred, not passed. The existing `LocalPackageM2` archive does not contain these new source changes.

The source/data checker passes all 3,348 checks (`local-evidence/m3-ui-source-check-final.log`). Its initial run exposed an existing stale exact-count assertion: the Foundation plugin contains 39 native tests after the earlier CapacityManifest addition, while the imported M1.10 register describes 38. The display checker now preserves that historical register and permits later tests, matching the earlier milestone checks. No native tests were executed by this source check.
