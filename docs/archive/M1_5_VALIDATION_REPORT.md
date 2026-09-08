# M1.5 Validation — Safe Return & Dry Checkpoints

**Section date:** September 5, 2026, America/New_York  
**Evidence recorded:** September 6, 2026 UTC (September 5 in America/New_York)  
**Plugin version:** `0.1.5-m1-safe-return-source`  
**Delivery:** Complete source update based on the supplied M1.4 package; no Unreal-compiled game or Windows executable.

## Result

All nine standalone C++ suites passed with GCC and Clang, and with Clang AddressSanitizer plus UndefinedBehaviorSanitizer. All Python tooling tests passed. These results exercise the shared engine-independent rules and local tools. The native recovery component, safety actors, input/fade/collision behavior and Unreal reflection declarations have **not** been compiled or exercised in Unreal. **M1 has not passed its playable acceptance gate.**

This increment brings the foundation's planned safe-return work forward into the systems test room. It does not establish M1 completion or include M2's production coast.

## Executed checks

| Check | Actual result | Scope |
|---|---|---|
| GCC 14.2, C++17, strict warnings | **2,605 assertions; 0 failures** | Nine engine-independent suites. |
| Clang 17, C++17, strict warnings | **2,605 assertions; 0 failures** | The same nine suites, not additional gameplay scenarios. |
| Clang AddressSanitizer + UndefinedBehaviorSanitizer | **All nine suites passed; no reported sanitizer errors** | Exercised helper paths only; no native engine/vendor execution. |
| Python 3.13.5 unittest | **52 test cases passed** | Recipe validation and retained save/preflight/report tools in filesystem fixtures. |
| Source/data/document consistency | **731 checks; 0 failures** | IDs, source contracts, recipe/timing consistency, Python syntax and local document links. |

### Assertion accounting

| C++ suite | Assertions | Relationship to M1.4 |
|---|---:|---|
| First Signal mission | 407 | Retained; rerun. |
| Campaign/transaction | 227 | Retained; rerun. |
| Save envelope | 583 | Retained; rerun. |
| UI flow/presentation | 650 | Retained; rerun. |
| Inventory view/catalogue | 80 | Retained; rerun. |
| Startup lifecycle | 94 | Retained; rerun. |
| Test-room manifest | 193 | Retained; rerun. |
| Interaction routing/offer | 211 | Retained; rerun. |
| Safe-return/checkpoint | 160 | New. |
| **Total** | **2,605** | **2,445 retained + 160 new.** |

Python comprises 31 retained cases and 21 new safety-recipe cases. Boundary combinations and individual assertions are not player playthroughs. Repeating the same tests under another compiler or sanitizers broadens verification without multiplying distinct scenarios.

Actual output is retained in [GCC results](../../evidence/archive/m1_5/gcc_core.txt), [Clang results](../../evidence/archive/m1_5/clang_core.txt), [sanitizers](../../evidence/archive/m1_5/sanitizers.txt), [Python results](../../evidence/archive/m1_5/python_tests.txt) and [structural checks](../../evidence/archive/m1_5/structural_checks.txt). The [machine-readable record](../../evidence/archive/m1_5/results.json) contains commands, return codes and counts. [Environment observations](../../evidence/archive/m1_5/environment.txt) record the inspected tools/paths, not an exhaustive search of the user's computer.

## What the new checks establish

The exact shared recovery helpers used by native source are tested for ordered return phases, exclusive save/mutation/recovery reservations, checkpoint dwell, finite/valid timing, bounded primary/fallback candidate attempts, grounded-settle timeout, failure permanence, session invalidation, held-interact rearming, and upright capsule/box intersection boundaries. The recipe tests reject malformed records, invalid kinds, duplicated identifiers, nonfinite geometry, wrong timing, and unsupported fields.

These are not simulated AGIS transactions, real Unreal traces, engine teleports, observed camera fades or gamepad playthroughs. Native integration can still reveal compilation or runtime defects not exposed by standalone helpers.

The [source delta](../../evidence/archive/m1_5/plugin_source_changes.json) records 9 added and 13 modified plugin paths, with before/after content hashes against the actual supplied M1.4 ZIP; no repository revision is invented. Prior M1.4 logs and manifest are preserved under `evidence/archive/m1_4/`. The [historical M1.4 validation](../../docs/archive/M1_4_VALIDATION_REPORT.md) remains explicitly historical. [SHA256SUMS.txt](../../evidence/archive/m1_5/SHA256SUMS_before_m1_6.txt) hashes the final package files and excludes itself.

## Not executed

| Required verification | Status |
|---|---|
| Unreal Header Tool, UnrealBuildTool and the actual Windows host compiler | **NOT RUN** |
| Nineteen supplied Unreal automation tests: fifteen retained plus four new `Coastal.M1Recovery` tests | **NOT RUN** |
| Actual optional safety recipe in Unreal; no `.umap` generated here | **NOT RUN** |
| Real capsule traces, door obstruction, adjusted teleport destinations and ground settling | **NOT RUN** |
| Camera fade ownership, input-stack blocking/cleanup, UMG status and pause transitions | **NOT RUN** |
| Real AGIS pickup/transfer/repair, inventory retention, export, restore and rollback | **NOT RUN** |
| Real Hyper selected-target/prompt wiring and controller/keyboard return behavior | **NOT RUN** |
| Checkpoint/fall/invalid-return behavior across New, Continue and relaunch | **NOT RUN** |
| Actual disk write failures, damaged-save recovery and retained generation behavior | **NOT RUN** |
| Windows PowerShell runner, cooking, packaging and standalone executable launch | **NOT RUN** |
| Frame-time, low-frame-rate, streaming and memory measurements | **NOT RUN** |

No actual host `.uproject` or purchased package API is supplied. No Unreal executable/build tool was found in PATH or the recorded inspected directories. The included nineteen native tests are source, not executed evidence. No replacement inventory, fake engine, generated-map screenshot or Windows binary is presented as proof of integration.

## Remaining integration risks

The safety registry assumes the fixed M1 character and a fixed set of authored upright safety volumes. It does not support streamed/spawned safety membership, arbitrary gravity, ragdoll recovery, swimming or physical water. Two bounded return candidates deliberately stop rather than repeatedly search/teleport. A runtime pawn already destroyed by engine KillZ cannot be recovered; the authored fall plane must trigger earlier, and fast-fall/hitch behavior requires real local testing.

Input priority 90 blocks lower-priority gameplay while native menu input remains at 100. Host gameplay running at higher priority or directly from unrelated Blueprint/tick logic must not bypass the game-owned safety/interaction gates. Camera fading is optional and respects a preexisting active fade; unrelated hosts must not claim it during a return. Native input, delegate and teardown behavior are not certified by text-based structural checks.

Inventory and quest preservation are implemented by not mutating them during return and holding the original campaign operation gate, not by copying AGIS contents. The real provider must obey its original atomic/idempotent transaction boundary. The default adapter remains `NotConfigured`. The save schema and IDs are unchanged, but stricter destination validation can reject old unsafe positions; no successful old binary-save load or migration is claimed. Read-back verification/CRC still do not prove crash-safe disk atomicity.

## Local completion gate

Follow [M1.5 recovery wiring](../../docs/M1_5_RECOVERY_WIRING.md), compile the actual host/plugin, initialize the real provider, and run the nineteen native tests. Then execute the [32-case recovery acceptance ledger](../../data/m1_5_recovery_acceptance.json), shipped entirely as `not_run`, alongside the retained M1 campaign/UI/startup/interaction scenarios. Record actual engine, toolchain, AGIS/Hyper versions and runtime evidence separately.

The required Windows test-room loop remains New → pickup → storage round-trip → repair → transcript → safe return with unchanged inventory/progress → verified save → exit/relaunch → Continue, including blocked-return and disk-failure cases. Only the actual coherent packaged test-room result passes M1. M2 production coast assembly and finished presentation remain later work.
