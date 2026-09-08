# coastal-exploration---an-unreal-ai-created-project
coastal exploration - an unreal ai created project

## Local native integration

Development is paused at the user's request. This repository is a source snapshot of the coastal game and its integration tools; it does not include the external Unreal host project, purchased Fab assets, campaign saves, or a playable download.

The isolated Unreal 5.8.1 host compiles the actual AdvancedShooterSystem source and the project combat wrapper. The last native test run passed 46 tests before the combat increment. Destination quests, campaign switching, day/night, and a bounded procedural grass patch have passed their recorded live checks. Combat live acceptance, the corrected swimming seam, and Morbid shelter verification remain pending; the shooter tracer and cooldown review findings remain unresolved. See the [current checkpoint](AGENTS.md) and [M3 implementation](docs/M3_IMPLEMENTATION.md).

All 66 Python tests passed for this publication snapshot. The historical milestone descriptions below describe earlier deliveries and are not claims that the current integration is complete.

The workspace now lives in `F:/coastline`. [M2 implementation](docs/M2_IMPLEMENTATION.md) records the persistent First Signal coast, assembled with the owned Nordic cabin. Editor and Windows targets compile, and the Windows package is built at `F:/coastline/LocalPackageM2/Windows`. Use `F:/coastline/Play M2.cmd` when ready. Gameplay acceptance is deferred at the user's request; see the [scope audit](docs/M2_SCOPE_AUDIT.md).

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
