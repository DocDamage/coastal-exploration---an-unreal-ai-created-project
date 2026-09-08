# Coastal Exploration — M1 Systems Test Room & Campaign Persistence

**Third-person. Single-player. Unreal. Windows first.**

This is an **expanded source delivery**, built on the supplied M0 starter. It is not a packaged game, an engine-compiled plugin, or a completed M1 milestone. No `.uproject`, paid assets, `.umap`, Blueprint widgets, or Windows executable is included.

## What changed

The plugin now includes campaign snapshots; guarded save/load/new-game coordination; two-slot save IO with integrity and read-back checks; rollback/fatal-lock handling; stable-ID development world actors; guarded pickup, door, storage, radio and journal actions; and adapter contracts for the actual AGIS installation. A Python editor recipe creates the development test-room geometry after local Unreal compilation. It does not wire AGIS, Hyper, or widgets.

AGIS remains the only inventory authority. Every default adapter method fails explicitly. No substitute runtime inventory was added to hide the missing integration.

## Start here

Read [M1 implementation and handoff](../M1_IMPLEMENTATION.md), then [Blueprint wiring](../M1_BLUEPRINT_WIRING.md) and [the extended adapter contract](../M1_ADAPTER_CONTRACT.md). The original [foundation](../COASTAL_EXPLORATION_FOUNDATION.md) is retained as the M0 design baseline. Read the current [validation report](M1_VALIDATION_REPORT.md) before interpreting test results.

## Contents

| Path | Purpose |
|---|---|
| `Plugins/CoastalFoundation/` | Expanded original Unreal runtime-plugin source. |
| `tools/unreal/build_systems_test.py` | New-map-only editor generator; requires compiled plugin and editor Python. |
| `data/test_room.json` | Actual input consumed by the editor generator, in centimetres. |
| `data/journal.json` | Original story-copy record, mirrored by `CoastalStoryLibrary`. |
| `data/` | Retained foundation records and M1 dependency/evidence ledgers. Other design JSON is not automatically loaded by the runtime. |
| `tests/` | Standalone C++ rules/integrity tests and Python save-inspector tests. |
| `tools/inspect_save_envelope.py` | Read-only envelope inspection; optional exclusive-new-file corruption fixture. |
| `docs/` | Foundation, M1 source design, wiring, integration contracts, acceptance tests. |
| `evidence/` | Actual local check outputs and machine-readable result summary. |

## Local source checks

```sh
bash tools/run_core_tests.sh
CXX=clang++ bash tools/run_core_tests.sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 tools/verify_package.py
```

Windows, from Visual Studio Developer PowerShell:

```powershell
.\tools\run_core_tests.ps1
py -m unittest discover -s tests -p "test_*.py" -v
py .\tools\verify_package.py
```

The Windows commands are supplied but were not executed here. Standalone checks do not compile Unreal reflection, exercise AGIS, or prove packaged gameplay.

## Local Unreal entry point

Create or open the standard Third Person C++ host project. Back it up, close the editor, replace the old `Plugins/CoastalFoundation` folder with this delivery's complete plugin folder, regenerate project files and build Development Editor / Win64. Preserve your own adapter subclass and assets separately before replacing any existing source.

After compilation, enable editor Python and Editor Scripting Utilities, save current work, and run `tools/unreal/build_systems_test.py` through Unreal's Python execution interface. Existing destination maps are refused. Follow the wiring guide before attempting the objective.

The exact engine version is still a local compatibility decision. The foundation's UE 5.7 candidate has not been certified against your installed packages. No repository was created or modified, and no vendor content is redistributed.
