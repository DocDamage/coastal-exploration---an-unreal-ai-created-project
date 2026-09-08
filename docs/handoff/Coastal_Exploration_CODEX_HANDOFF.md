# Codex handoff — Coastal Exploration / First Signal

**Prepared:** September 6, 2026  
**Source baseline:** M1.10 — `0.1.10-m1-display-settings-source`  
**Next objective:** Finish real Unreal integration and the launched Windows M1 systems-room gate. Do not start another optional source-only feature increment.

## 0. Repository-specific transfer instructions

**Project repository:** `https://github.com/DocDamage/coastal-exploration---an-unreal-ai-created-project`  
**Observed default branch:** `main`  
**Observed visibility:** public  
**Observed root contents:** `README.md` only, when checked September 6, 2026 through the connected GitHub API.

The repository is initialized, not completely empty. The inspected default-branch root contains no imported M1.10 source. Recheck the current repository before acting: this is a handoff snapshot, not a guarantee that no later work exists. This handoff update made no GitHub commits, branch changes, uploads, or settings changes.

### Import before native integration

Use this repository as the canonical project source repository. Inspect an existing local checkout before cloning another copy. Verify the remote, branch, working-tree status, and applicable instructions. Do not reset dirty work, replace remotes on an unrelated checkout, create a competing `master`, or overwrite newer project work.

Create or safely resume one focused branch, suggested name `codex/m1-native-integration`, from the current appropriate base. Do not force-push or automatically merge to `main`. Repository creation alone is not permission to publish local files: prepare and review the import locally, and push/open a pull request only when the user requests publication.

Read this handoff and the source archive's existing `AGENTS.md` before importing. Verify the source archive and its internal manifest as described in section 2. Extract to a temporary location first. If the repository still contains only its introductory README, import the **contents** of `Coastal_Exploration_M1_10/` into the repository root so `Plugins/`, `docs/`, `data/`, `tests/`, and `tools/` remain together. Do not commit only a ZIP, place the source under `Content`, or make several competing milestone source trees.

Merge the repository README and package README deliberately; preserve the repository identity and the package's setup/testing information. Preserve the package's complete project `AGENTS.md`, including current M1.10 guidance. Merge any instructions already in the repository rather than replacing them with generic rules. Copy this handoff and the starter prompt into clearly documented locations. Preserve source provenance and historical evidence; a baseline manifest is not a certification of a subsequently modified working tree. Keep the unchanged source ZIP as an external reference rather than treating it as the editable project.

If an actual local Unreal host already exists, inspect its work before choosing a layout. Preserve its configuration, maps, owned packages, and implemented adapters; document its relationship to this repository and use one maintained plugin source. Do not create a second host merely because the package does not contain one. If no host exists, create one through the actual compatible Unreal installation as described later in this handoff.

### Public-repository boundary

The observed repository is public. For this handoff, keep purchased Fab/Leartes/AGIS/Hyper packages, vendor source, raw meshes/textures/audio, secrets, credentials, save profiles, and machine-specific private paths out of public commits. Do not assume that owning a package authorizes publishing its source. Do not change repository visibility or choose a project-wide license without the user's direction.

Inspect the package `.gitignore`; it excludes common build/cache outputs but does not enumerate the user's unknown vendor paths. Add path-specific exclusions based on the actual local project before staging. Do not blindly ignore every `Content/` directory or every `.uasset`/`.umap`: genuinely original project content still needs an intentional versioning plan. Review dependencies and provenance before treating any generated asset or wrapper as public-safe. Keep restricted dependencies external and document installation requirements without copying their content into the repository.

Inspect the complete staged diff and file list before any commit, and review committed history before any authorized push. A later ignore rule is not a substitute for checking what was already staged or committed. Use explicit reviewed paths rather than an unreviewed bulk upload. Local licensed vendor integration may proceed without making those dependencies public.

### First deliverables

Prepare the extracted original-source baseline, merged README/AGENTS guidance, reviewed ignore rules, and a dependency/setup record. Run the available source checks and record fresh results separately from imported historical reports. Then continue the native M1 integration assignment below in the same session wherever real tools and dependencies permit. Do not stop at repository housekeeping when the host can be built.

Report the actual branch, changed paths, commit/push status, source checks, Unreal build, vendor integration, and Windows acceptance separately. A repository import is not a playable-build milestone. The M1 gate and fixed product scope below are unchanged.

## 1. Your assignment

Take over the existing project, inspect the supplied source and actual local environment, and implement the remaining native integration. Continue the current architecture rather than rebuilding the game or replacing the selected inventory/interaction systems. A plan is the beginning of this assignment, not its final deliverable.

First report the actual remaining M1 blockers and an ordered execution plan. Then begin the first executable task in that same working session. Continue through implementation and verification wherever the available tools and dependencies permit. Do not stop after an audit when actionable work remains.

This document combines the supplied project's requirements with explicit execution instructions for this handoff. New report filenames and work-session procedures below are requested workflow, not claims that those files or integrations already exist. Existing project source, contracts, and current documentation supply the implementation details.

## 2. Establish the correct baseline

The supplied cumulative source archive is:

`Coastal_Exploration_M1_10_Source_Package.zip`

Its expected SHA-256, from the supplied delivery check, is:

`7ed7679ae9bd2d9d4a84d2cae7f62886839fc63ce23ebadd19c09840cceb4298`

It extracts to `Coastal_Exploration_M1_10/`. That directory contains `AGENTS.md`, `README.md`, `Plugins/CoastalFoundation/`, `docs/`, `data/`, `tests/`, `tools/`, and historical evidence. Verify the archive and `SHA256SUMS.txt` before modifying the baseline. The supplied record covers 338 hashed files plus the manifest itself. Preserve the original archive and historical evidence unchanged.

M1.10 is the current supplied source baseline, not a patch requiring sequential installation of M1.6–M1.9. Earlier archives may be retained for historical comparison; do not overlay their plugin files onto M1.10. A genuinely newer local host implementation may contain work not present in the archive: inspect differences and merge deliberately rather than downgrading it.

No real host `.uproject`, generated `.umap`, purchased asset package, sound recording, compiled plugin, or Windows executable is included in this delivery. Do not interpret this as proof that these are absent from the user's machine. Locate what is actually accessible.

If there is an existing host project, preserve its configuration, maps, purchased content, actual AGIS adapter, and vendor wrappers. Merge the updated plugin, not the entire archive into `Content` or `Plugins`. The plugin descriptor belongs at `<HostProject>/Plugins/CoastalFoundation/CoastalFoundation.uplugin`.

Read applicable existing `AGENTS.md` instructions before editing. The source package already has project instructions; preserve them. If the host is a separate repository, deliberately incorporate/reference relevant guidance in its instruction scope without replacing unrelated host instructions. Do not blindly generate a replacement `AGENTS.md`.

## 3. Read these documents before implementation

Begin with the source package's `AGENTS.md`, `README.md`, `VALIDATION_REPORT.md`, `docs/WHAT_REMAINS.md`, and `docs/COASTAL_EXPLORATION_FOUNDATION.md`.

Then inspect `docs/UNREAL_SETUP.md`, `docs/INTEGRATION_CONTRACTS.md`, `docs/M1_IMPLEMENTATION.md`, `docs/M1_ADAPTER_CONTRACT.md`, `docs/M1_BLUEPRINT_WIRING.md`, and `docs/ACCEPTANCE_TESTS.md`.

Read the current integration chain: `M1_2_UI_WIRING.md`, `M1_3_STARTUP_WIRING.md`, `M1_4_INTERACTION_WIRING.md`, `M1_5_RECOVERY_WIRING.md`, `M1_6_OPTIONS_WIRING.md`, `M1_7_SPRINT_WIRING.md`, `M1_8_AUDIO_WIRING.md`, `M1_9_PLAYBACK_WIRING.md`, and `M1_10_DISPLAY_WIRING.md`, all under `docs/`. Consult their companion implementation documents when changing a subsystem.

Inspect the actual source, dependency registers, `data/test_room.json`, acceptance ledgers, and `tools/unreal/` recipes, not just prose summaries.

Some retained documents describe earlier stages. Their old test counts, manual startup paths, generic director, and preference write formats are historical. Follow explicit superseding instructions in the current source and M1.10 documentation. Current UI/startup source already exists; do not recreate it because an M0 setup paragraph says it remains to be implemented. Resolve actual contradictions explicitly and document the decision; do not silently rewrite history.

## 4. Preserve the product direction

This is a **third-person, single-player, Windows-first coastal exploration game in Unreal**, with the opening mission **First Signal**. The tone is grounded with light mystery. The initial world is compact and handcrafted.

After M1 passes, the planned opening route is **Cabin Cove → Shoreline Trail → Old Dock**, with **Rail Overlook optional**. Preserve the existing mission, full transcript, discoveries, logical IDs, and guaranteed radio-part acquisition.

Do not add co-op, network accounts, combat, enemies, hunger/thirst, crafting trees, construction, a procedural world, swimming, boat driving, dynamic tides, or an economy to finish this opening. Do not add interfaces for unimplemented features. More owned assets do not expand scope automatically.

The foundation's target hardware is the user's RTX 3060 12 GB desktop class at 1920×1080, approximately 60 FPS. This is a target, not an achieved benchmark. Defer expensive rendering additions until measured performance supports them.

## 5. Current evidence: source exists, native acceptance is unproven

Implemented source covers mission/journal/transcript rules; coherent campaign coordination and save envelopes; inventory adapter contracts; persistent development objects; session, pause, inventory, storage, and journal UI; bootstrap/preflight; interaction routing; checkpoint/safe return; look/FOV/text options; Hold/Toggle sprint; preference upgrades; routed volume controls; optional ambience/radio playback; and M1.10 display testing with Keep/Revert.

The supplied M1.10 validation reports 18 standalone C++ suites with 7,674 assertions under each of GCC and Clang, successful sanitizer runs, 52 Python tests, and 1,592 structural checks. Treat these as historical source-environment results until independently rerun. They are not thousands of gameplay scenarios.

The package supplies **38 Unreal automation tests**, all unrun in the delivery environment. The **38-case display acceptance ledger is a separate inventory**, alongside the other retained ledgers. Native compilation, real AGIS/Hyper behavior, rendering/controller/audio/display verification, platform save IO, and Windows gameplay remain unrun in the supplied evidence.

Do not equate compiling helper headers with compiling the plugin. Do not report native testing as passed because a structural checker passes or because a script was written.

## 6. First work phase: inspect and compile the real environment

Inspect the current workspace, branch/status if Git exists, dirty files, applicable instructions, existing host project, installed Unreal builds, Windows C++ toolchain/SDK, and accessible purchased packages. Record real paths and versions; do not invent a repository, host path, asset reference, or vendor revision.

Use existing compatible local choices where established. UE 5.7 appears in original setup as a compatibility-test candidate, not a certified project pin. A documentation page displaying UE 5.8 is also not a project pin. Check the real installed packages and supported toolchain before selecting an engine. Do not upgrade the engine merely because a newer version exists.

If no host exists and the necessary local engine/tools are available, create the standard Third Person C++ host described in the setup guide, normally `CoastalExploration`, without overwriting any existing project. Build the untouched template first. Then integrate and compile CoastalFoundation using actual UHT/UBT and the selected Windows compiler, including Development Editor / Win64 and the game target needed for packaging.

Fix genuine reflected declaration, include, module dependency, API, linker, and packaging defects. Preserve behavioral contracts; do not remove tests or turn guards into unconditional success to obtain a build. Keep changes focused, generally below 300 lines per file when practical.

Inspect scripts before running them. Use project-specific, reproducible commands with captured output rather than guessing paths. Do not delete user content, wipe saves, reset dirty work, force-push, publish a repository, or upload purchased files as part of setup.

## 7. Second work phase: implement the real AGIS boundary

AGIS remains the **only inventory/container authority**. Inspect the installed package's actual classes, events, functions, persistence data, and transaction behavior before implementing the project-owned adapter. Prefer game-owned wrappers rather than edits to vendor originals.

Implement the real `CoastalInventoryAdapter` contract, including provider readiness, requirement queries/consumption, world pickup, container transfer, consistent export/validation/restore, read-only fresh-inventory construction, and the read-only container view required by the current native UI. Use the current headers and `M1_ADAPTER_CONTRACT.md` for exact signatures.

Before bootstrap, initialize a genuinely usable, empty, exportable provisional AGIS session with a valid GUID. Do not pre-grant the battery or fuse: the M1 copies exist as authored world pickups. Do not seed the same required item in both a container and a loose pickup.

Keep insertion and its receipt atomic. Never hide/destroy a pickup before the guarded collection succeeds. Keep transfers and repair consumption all-or-none and idempotent, with campaign-scoped receipts. Repeated use must not consume another battery/fuse or duplicate rewards. A reused operation ID with different contents must fail. `BuildNewInventory` is read-only; `RestoreInventory` commits the replacement.

Preserve actual instance identities, grids, quantities, containers, and the authoritative receipt ledger in the same snapshot. Critical items must remain recoverable in the player inventory or cabin storage until valid repair consumption. Block discard/destruction/other unauthorized use in the actual vendor UI as well as game logic. Vendor drag/drop must not bypass the coordinator's guarded mutation boundary.

The contract is synchronous. If the installed vendor API is latent/asynchronous, document that mismatch and implement an explicit compatible pending/completion design with tests before reporting success. Do not return success before a mutation finishes, sleep until it appears complete, maintain shadow item counts, or ship a fake provider.

## 8. Third work phase: wire the room, Hyper, startup, and input

Use `tools/unreal/build_systems_test.py` and `tools/unreal/audit_systems_test.py` as the supplied starting recipes. Run/adapt them through the actual compatible Unreal editor environment. Keep their overwrite and dirty-package safeguards. Inspect the resulting map; generating geometry alone does not connect the game.

Configure the real GameMode, possessed standard third-person character, local PlayerController, native mission director, AGIS adapter, UI, interaction bridge/relay, bootstrap, checkpoints, and hazards according to current wiring. Use native `CoastalMissionDirector`, not the superseded generic M0 director.

Keep one intentional startup path:

```text
Bootstrap.StartTestRoom(ActualDirector, ActualAGISAdapter, ValidatedInitialDryCheckpoint)
```

Call it after actual provider readiness, possession, and validated dry placement. Do not also execute the old manual initialization chain or initialize optional owners twice. Preserve one fixed local player and one owner per responsibility.

Hyper remains the selected-target/focus/prompt authority. Forward its real selected target and current input state to the current bridge/relay. Do not add another world-scanning focus system, guessed vendor methods, or a duplicate direct use handler.

Connect actual keyboard/mouse and gamepad input. Preserve reach/occlusion checks, modal input suppression, session/permission freshness, and release/repress behavior. Forward real neutral/released state after transitions; canceled input contexts are not proof that the physical controls were released.

Keep host movement/input mapping ownership. Route mouse deltas and normalized stick input separately for look, without duplicate rotation or double delta-time scaling. Sprint receives actual aggregate held/released state, has one speed-property owner, and must not resume merely because a held button survived a menu/load/recovery transition. Apply documented tick/lifecycle requirements.

Create `.umap`/`.uasset` content only through real engine/editor-supported operations. Do not fabricate text files with those extensions. When an essential Blueprint operation cannot be performed with available tools, specify the exact asset, nodes/properties, connections, and verification needed; record it as pending, not completed.

## 9. Fourth work phase: integrate existing presentation, not new features

Wire and test the existing UI, camera/look, sprint, recovery, text scaling, audio routing/playback, and display features. Do not spend this milestone adding graphics-quality presets or other optional source features while the native foundation is still unproven.

Use one `CoastalUISessionComponent` on the local controller. Optional look/sprint/recovery components belong on the character according to their guides; audio-options/playback owners belong on the controller. The M1.10 display owner is an internal UObject created by the UI, **not an additional actor component**. Opt-in flags are configuration declarations, not test evidence.

For audio, assign genuinely available owned sounds and the three distinct independent Ambience, Effects, and Radio sound classes. Avoid duplicate ambience/radio players and a second master-volume multiplier. Preserve initialization-before-play and owned-source cleanup-before-mix-release. Missing optional sounds may remain explicitly silent for a systems test; do not claim final audio polish.

The full transcript and explicit acknowledgement remain mission authority. Audio completion, mute, missing recordings, and replay cannot grant `messageHeard`. Test completion with sound muted and playback absent, plus early Continue and cancellation.

Enable display via the existing UI's `bEnableDisplaySettings` only after exclusive display ownership is integrated. Its supported path is a Windows non-editor standalone game with one local player, standard `UGameUserSettings`, and no XR system. Do not bypass these checks to pretend PIE validates display switching.

Preserve the 15-second real-time trial, current viewport/presentation and fresh-input requirements for Keep, session-only versus explicit save choices, rollback to the actual pretrial mode, and five-second restoration-observation failure handling. Trial requests must not stage/save engine preferences through a competing `ApplySettings` path. A `SaveSettings` request is not verified disk persistence; confirm actual files and relaunch behavior separately.

Review native screens with keyboard/mouse and controller-only input at 1920×1080 and 1280×720, and 100%/150% text. Observe focus, scrolling, transcript visibility, modal suppression, held controls, and recovery. Do not fabricate screenshots or controller observations.

## 10. Fifth work phase: validate persistence and the packaged loop

Preserve coherent campaign generations across inventory, receipts, world objects, journal, quest, and player/checkpoint state. Keep failed Continue distinct from New. Never silently reset the campaign, erase prior saves, or clear a poisoned coordinator to conceal a restore failure.

Coastal preferences retain `CoastalLocalOptions_v1_A` and `CoastalLocalOptions_v1_B`, user index 0. Current reads accept schemas 1/2/3; explicit writes use schema 3. M1.10 introduces no campaign or Coastal preference migration. Engine display settings are separate. Do not revert to historical schema-1/2 write instructions or introduce a gratuitous migration.

Use isolated disposable profiles with backups for corruption, unsupported-format, read/write, ambiguous-generation, external-change, and rollback tests. Use one writer per profile. Preserve valuable saves and engine settings. Exact read-back/CRC is not evidence of crash-atomic storage or cross-process locking.

Reproduce the source checks using tools available in the current environment:

```bash
bash tools/run_core_tests.sh
CXX=clang++ bash tools/run_core_tests.sh
bash tools/run_sanitizer_tests.sh
python3 -m unittest discover -s tests -p 'test_*.py'
python3 tools/verify_package.py
```

A supplied Windows alternative for the standalone C++ tests is `tools/run_core_tests.ps1`, intended for Developer PowerShell with `cl.exe` available. Record compiler/tool availability and actual outcomes; never label an unavailable compiler or skipped sanitizer stage as passed.

Run the 38 supplied Unreal automation tests through the real host, then execute the retained acceptance ledgers and add regression cases for defects fixed. Native helper tests do not replace visual, device, provider, or platform-IO testing.

Cook/package the actual Windows test room and launch it outside the editor. Complete:

**New → collect battery and fuse → storage round-trip → repair radio → show full transcript → explicit acknowledgement → safe return → verified campaign save → quit → relaunch → Continue.**

Check collection in either order, full destinations, repeated interactions, one part stored at save time, partial progress before repair, post-repair reload, missing provider, and failed restore/rollback. Confirm inventory/world/receipt/quest coherence and independent control/audio/display persistence. Inspect the actual package output and its launch log; merely generating a package command is not a build.

## 11. Blocker and environment policy

Inspect accessible project files and relevant configured tools before asking for information already present. Do not ask the user to reconfirm the genre, engine family, no-co-op decision, or named route.

If a dependency or capability is genuinely unavailable, record the exact missing engine/tool/package/path/API access and the specific work it blocks. Continue safe, useful, independent work such as native compile fixes, source verification, host preparation, or precise integration scripts where possible. Do not substitute a fake vendor or claim the missing check passed.

If the environment cannot launch Windows Unreal, perform only the work it can actually execute and leave native/editor/Windows gates explicitly blocked or not run. Do not repeatedly add unrelated helpers to create the appearance of progress. A real dependency blocker is a legitimate stopping point after nonblocked work is exhausted.

Manual instructions are a last-mile fallback for actions unavailable to your tools, not a substitute for edits/builds you can execute. Ask a targeted question only when an indispensable ambiguity cannot be resolved locally or an action requires permission. Do not stall the whole assignment for optional art/audio or decisions already documented.

## 12. Required deliverables and reporting

Maintain a concise integration plan and evidence-backed status report in the working project, using existing conventions where present. Suggested new paths are `docs/M1_NATIVE_INTEGRATION_PLAN.md`, `docs/M1_NATIVE_INTEGRATION_STATUS.md`, and a separate local evidence directory. These names are requested outputs, not existing artifacts.

Keep original supplied reports immutable. Record current runs separately with actual commands, tool versions, timestamps, exit codes, logs, asset references, and acceptance outcomes. Tests should distinguish pass, fail, blocked, and not run. Leave unobserved manual scenarios unpassed. Record manual observations as such; do not present them as automated execution.

At each substantial stopping point report what changed, what was executed, actual results, remaining blockers, and the next concrete action. At handoff give the real `.uproject` path, plugin integration location, build/run commands, generated asset/map paths, actual package location if produced, and known limitations. No invented paths or completion percentages.

Report these dimensions separately: **source implemented; source checked; Unreal compiled; vendor integration tested; editor gameplay tested; Windows package tested.** A successful build is not a playthrough; a successful playthrough is not every failure scenario.

Stop this first assignment at an evidence-backed M1 milestone, or at explicit environmental blockers with the maximum safe completed work. Do not start M2 environmental assembly until the actual packaged M1 gate passes. Later work remains M2's route, remaining M3 art/audio/input/graphics/performance polish, and M4 clean Windows delivery.

**Begin now: inspect the actual workspace and current instructions, list the real M1 blockers, and implement the first nonblocked integration task.**

## Source basis

Project requirements and current status above derive from the unmodified M1.10 archive, particularly `AGENTS.md`, `README.md`, `VALIDATION_REPORT.md`, `docs/WHAT_REMAINS.md`, `docs/COASTAL_EXPLORATION_FOUNDATION.md`, `docs/M1_ADAPTER_CONTRACT.md`, and the current wiring chain. The external M1.10 delivery check supplies the archive hash. Existing source and registers must be inspected before acting on implementation names or paths.

The handoff preparation checked official OpenAI guidance on project instructions and local workflows: [AGENTS.md discovery](https://developers.openai.com/codex/agent-configuration/agents-md) and [Codex CLI local repository work](https://developers.openai.com/codex/cli). Those references explain Codex context/tool use; they do not establish Unreal compatibility or any native test result.
