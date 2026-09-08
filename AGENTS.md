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
