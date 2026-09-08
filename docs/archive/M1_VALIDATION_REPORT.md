# M1 Validation Report

**Section date:** September 5, 2026  
**Evidence recorded:** September 6, 2026 UTC (September 5 in America/New_York)  
**Delivery status:** Expanded source checked locally; not an Unreal-compiled or packaged game.

## Executed checks

| Check | Actual result | What it establishes |
|---|---|---|
| GCC 14.2 C++17, strict warnings | 1,217 assertions; 0 failures | Engine-independent mission, campaign/gate/fingerprint and envelope helpers compile and run. |
| Clang 17 C++17, strict warnings | Same 1,217 assertions; 0 failures | The same helper source compiles and runs under a second compiler. |
| Clang AddressSanitizer + UndefinedBehaviorSanitizer | All three C++ suites passed; no reported sanitizer errors | No detected memory/undefined-behavior defects in the exercised standalone paths. |
| Python 3.13 unittest | 10 test cases passed | Save-envelope inspection and exclusive-new-file corruption-fixture safeguards. |
| Package/source consistency | Passed; exact count in `evidence/structural_checks.txt` | Required IDs, source layout, generated-header ordering, data/copy consistency, fail-closed defaults and local document links. |
| Python syntax checks | Passed through package validator | Syntax only, including the Unreal editor generator. No `unreal` module was imported/executed here. |

The C++ total consists of 407 retained mission assertions, 227 campaign/transaction assertions and 583 envelope assertions. Running the same assertions under two compilers does not create 2,434 distinct gameplay tests. Generated bit-corruption checks and state-combination checks are assertions, not separate user scenarios.

## Evidence files

Actual stdout/exit results are retained in `evidence/gcc_core.txt`, `evidence/clang_core.txt`, `evidence/python_tests.txt`, `evidence/sanitizers.txt` and `evidence/structural_checks.txt`. The machine-readable record is `evidence/results.json`. Checksums for packaged files are in `SHA256SUMS.txt`; that manifest excludes itself.

## Not executed

Unreal Header Tool, UnrealBuildTool, all four included Unreal editor automation tests, the editor map-generation script, actual AGIS transactions, Hyper interface wiring, in-engine save/load/rollback, gamepad UI, Windows PowerShell test runner, Windows cooking/packaging, standalone game launch and frame-time measurements are **NOT RUN**.

No Unreal executable/build tool was located in the checked runtime locations. The user's actual host `.uproject` and purchased asset package contents are not present in this source delivery environment. The plugin source has therefore not been validated for engine compilation or an installed vendor API.

## Important unresolved integration work

The adapter still deliberately returns `NotConfigured`. Its required implementation is specified, not faked. Inventory insertion/removal/transfer atomicity and restore rollback depend on a tested implementation against the real AGIS package. Default adapter tests only prove that missing implementation does not report success.

The editor script places native development objects. For actual Hyper focus, replace them with a project-owned interface-enabled subclass while preserving authored fields, or supply the actual subclass path to the recipe before generating a new map.

Widgets are not included. Start/Save/Continue, storage, transcript and error presentation need local Blueprint wiring. Save selection uses explicit test-set names; there is no polished title/save catalogue. UI blockers are not a complete focus stack. Character placement checks are for the M1 dry room, not the finished water recovery system.

The save envelope is a corruption-detection mechanism, not a security boundary. The coordinator uses synchronous IO for the small test room. Read-back verification and retaining another slot do not prove crash-safe storage atomicity. Actual platform write-failure and corrupted-save recovery tests remain mandatory.

## Milestone decision

**M1 has not passed its acceptance gate.** Source exists for the game-owned systems, and standalone verification passed. Next execute the local build, vendor integration and packaged test-room sequence in `docs/M1_BLUEPRINT_WIRING.md`. Do not count a script as a generated map or a successful helper test as a playable build.
