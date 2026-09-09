# coastal-exploration---an-unreal-ai-created-project
coastal exploration - an unreal ai created project

## Local native integration

The full animation/activity expansion is in progress. Directional hit reactions
and layered pistol animations now pass native and scripted gameplay checks.
The supplied vehicle/fishing/ladder art is staged privately; its gameplay remains
on the active [activity checklist](docs/M3_ACTIVITY_EXPANSION.md).

The UE5.8.2 host now integrates a Mutable character creator with 16 settings,
per-campaign appearance saves, generated hair/clothing, and runtime retargeting
through the existing player systems. Two imported pickup/hit clips supplement
native locomotion, swimming and camp/shelter actions. Open Pause → Customize
character. Native build and 60 tests pass; exact appearance save/reload,
variant generation and scripted action playback are verified. See
[creator integration and validation](docs/M3_CHARACTER_CREATOR.md). Earlier
checkpoints below retain their original scope.

September 9 continuation adds seven destination music tracks and two distant
thunder clips, with score fades, indoor attenuation and pause/recovery handling.
The native build, 56 native tests, 49 gameplay assertions and seven mixer checks
pass. All 82 pre-existing saves and both host maps remain unchanged. Listening and
packaged acceptance remain open. See [soundscape](docs/M3_SOUNDSCAPE.md).

September 9: the first new-asset increment adds ten interaction/UI audio clips. The
new native build passes all 54 tests; 69 scripted gameplay assertions and actual
mixer mute/half-gain checks pass. All 72 pre-existing save files remain unchanged.
Listening/tonal review and the remaining asset batches are still open. See
[interaction audio](docs/M3_INTERACTION_AUDIO.md).

The merchant/item-preview source increment has a passing native build and 59
native tests (58 clean plus one known AGIS warning). Final preview PIE `j`
passes 53 assertions with colored battery/fuse imagery, controls, stale-view
refusal, cleanup, recovery, save/reload, and save preservation. Default fuse
orientation art polish remains open. Combat source compiles after a root
`TObjectPtr` fix, but is not bound to imported assets or runtime-tested.

Development has resumed on the relocated `G:/coastline` workspace with Unreal 5.8.2 and MCP connected. This repository is a source snapshot of the coastal game and its integration tools; it does not include the external Unreal host project, purchased Fab assets, campaign saves, or a playable download.

The isolated Unreal 5.8.2 host passes 56 native tests, 16 scripted combat cases,
22 water/checkpoint cases, 11 swimming route waypoints, 4 Morbid shelter cases and
7 German Shepherd companion cases. Both owned Atlantis Ruins and Modular SciFi
Station packs are assembled with tested routes and persistent investigation
records. Legacy seven-record progress survives the expansion to nine records.
Physical controls, visual polish, performance and Windows-package acceptance
remain. See the [current checkpoint](AGENTS.md), [companion](docs/M3_COMPANION.md)
and [M3 implementation](docs/M3_IMPLEMENTATION.md).

All 70 Python tests pass on the current machine (66 passed for the earlier publication snapshot). The historical milestone descriptions below describe earlier deliveries and are not claims that the current integration is complete.

The historical M2 paths in the following paragraph predate the relocation to `G:/coastline`. [M2 implementation](docs/M2_IMPLEMENTATION.md) records the persistent First Signal coast, assembled with the owned Nordic cabin. Editor and Windows targets compile, and the Windows package is built at `F:/coastline/LocalPackageM2/Windows`. Use `F:/coastline/Play M2.cmd` when ready. Gameplay acceptance is deferred at the user's request; see the [scope audit](docs/M2_SCOPE_AUDIT.md).

See [integration status](docs/M1_NATIVE_INTEGRATION_STATUS.md) and [ordered plan](docs/M1_NATIVE_INTEGRATION_PLAN.md) for current runs. The following M1.10 delivery description and `evidence/` reports are historical source evidence. `SHA256SUMS.txt` identifies the imported archive baseline, not the modified working tree.

---

# Coastal Exploration — M1.10 Display Settings & Keep/Revert

**Third-person · Single-player · Unreal · Windows first · First Signal**  
**Plugin:** `0.1.10-m1-display-settings-source`  
**Delivery:** Complete source update from the actual supplied M1.9 ZIP. Not an Unreal-compiled plugin or Windows executable.

Adds a native Display settings screen to Session/Pause: engine-reported resolution/window-mode drafts, an explicit 15-second runtime trial, observed viewport/presentation gating, session-only Keep, an explicit engine-settings save request, and timed/manual rollback to the actual pretrial mode. No trial automatically saves; a void engine save request is never labelled verified disk success.

The existing UI owns the internal display object. Set `bEnableDisplaySettings=true` on that UI only after integrating sole display ownership. The default is off. The bounded native path requires a Windows non-editor standalone game, one local player, standard engine settings and no XR system. PIE/custom settings subclasses are deliberately not silently modified.

No campaign or Coastal player-preference migration occurs: existing schemas 1/2/3 remain readable and player-preference writes stay schema 3. AGIS, Hyper, mission/story, inventory, recovery, look/sprint and audio remain their original authorities. All previous source features and data are retained. No host project, purchased asset, generated map, sound file or executable is supplied.

## Start here

| File | Purpose |
|---|---|
| [What remains](docs/WHAT_REMAINS.md) | M1 integration gate, M2 coast, remaining M3 presentation/stability and M4 delivery. |
| [Display wiring](docs/M1_10_DISPLAY_WIRING.md) | Install/merge safely, opt in, test actual Windows behavior. |
| [Implementation](docs/M1_10_IMPLEMENTATION.md) | Source ownership, mode admission, deadline/presentation, rollback and persistence limits. |
| [Validation](VALIDATION_REPORT.md) | Actual source evidence versus unrun native gates. |
| [38-case display ledger](data/m1_10_display_acceptance.json) | Actual-host scenarios; all shipped `not_run`. |
| [Display specification](data/m1_10_display_spec.json) | Original policies and bounds, not a runtime mode list. |
| [Retained M1.9 playback wiring](docs/M1_9_PLAYBACK_WIRING.md) | Existing optional ambience/radio setup. |
| [Foundation](docs/COASTAL_EXPLORATION_FOUNDATION.md) | Product direction and original milestone gates. |

## Reproduce source checks

```bash
bash tools/run_core_tests.sh
CXX=clang++ bash tools/run_core_tests.sh
bash tools/run_sanitizer_tests.sh
python3 -m unittest discover -s tests -p 'test_*.py'
python3 tools/verify_package.py
```

These compile/run shared engine-independent helpers and local tools, not UHT/UBT or the native display wrapper. The retained PowerShell runner is supplied but unrun here. Build the actual host and run all **38 supplied Unreal automation tests** plus the separate manual ledgers, then package and play the real test room. Earlier section documents/registers retain their historical counts; M1.10 is the current source increment.

**M1 has not passed its playable acceptance gate.** The next required production work is still real Unreal/AGIS/Hyper integration and the launched Windows campaign loop. Only afterward does M2 assemble Cabin Cove, Shoreline Trail, Old Dock and optional Rail Overlook. This display source does not deliver that coast or complete the remaining graphics-quality/accessibility/performance work.
