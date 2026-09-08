# Validation report — September 5, 2026

## Delivered status

This is a source-and-design starter. It is not a playable Unreal project, packaged game, or verified integration of the user's purchased assets. No repository was changed.

## Checks actually executed

### Standalone mission rules

Command:

```sh
bash tools/run_core_tests.sh
```

The runner compiles the exact `Public/Core/FirstSignalRules.h` used by the Unreal wrapper through a standalone C++17 test program, with `-Wall -Wextra -Werror -pedantic`, then executes it.

Compiler: GNU g++ 14.2.0 on Linux.

Result: **407 checks; 0 failures.**

Coverage includes initial objective selection, equipment-first progression, failure nonmutation, successful repair, repeated repair, committed-transaction recovery at the rules boundary, transmission ordering, repeat completion, and validation across all 64 possible combinations of the six boolean facts. It does not exercise real inventory removal, a real transaction ledger, disk persistence, or Unreal reflection.

These are 407 assertions/checks in a focused rules test program, not 407 end-to-end game scenarios.

### Structural consistency

Command:

```sh
python3 tools/verify_package.py
```

The tool checks JSON parsing and IDs, required-item consistency between JSON and source, quantities, transaction identifiers, protected quest items, area-connection reachability, unresolved asset path honesty, plugin descriptor structure, generated-header placement, original source file size, and local document links.

Result: **153 structural checks; 0 failures.** This is static structural validation, not an Unreal compiler substitute.

## Included but not executed

| Check | Status | Reason |
|---|---|---|
| Unreal Header Tool / UnrealBuildTool compilation | NOT RUN | Unreal tooling is not installed in this environment. |
| `Coastal.FirstSignal.DefaultAdapterFailsClosed` | NOT RUN | Requires the local Unreal host project. |
| `Coastal.FirstSignal.SnapshotValidation` | NOT RUN | Requires the local Unreal host project. |
| Windows PowerShell/MSVC test runner | NOT RUN | Current execution environment is Linux. |
| AGIS adapter, transactions, and persistence | NOT IMPLEMENTED / NOT RUN | Actual purchased package/API is not available here. |
| Hyper Interaction bridge | NOT IMPLEMENTED / NOT RUN | Actual purchased package/API is not available here. |
| Combined campaign save/load | NOT IMPLEMENTED / NOT RUN | Only in-memory quest snapshot helpers are included. |
| Coastal map, final character, and UI | NOT BUILT | Source asset packages and editor are not present. |
| Gamepad menu behavior | NOT RUN | No assembled UI or playable build. |
| Windows packaging and 1080p performance | NOT RUN | No assembled Unreal project/build. |

## Source review limitations

The wrapper uses standard Unreal plugin and component patterns. A guard-value include was checked against Epic's API reference and set to `Templates/UnrealTemplate.h`. This is a source-level correction, not evidence that all engine-specific code compiles. Run the actual compiler and automation tests before claiming compatibility.

The default inventory adapter intentionally returns `NotConfigured`. A fake successful adapter was not shipped. The package contains no third-party asset files, `.uasset` maps/Blueprints, `.uproject`, generated character art, or final icons.

## Required next validation

Follow `docs/UNREAL_SETUP.md` and execute `docs/ACCEPTANCE_TESTS.md` against the real local engine and packages. Keep rule-level checks, Unreal compilation, vendor integration, save reliability, and packaged-game evidence separate.
