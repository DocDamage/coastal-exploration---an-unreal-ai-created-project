# M1.4 Validation — Interaction Routing and Pressed-Once Protection

**Section date:** September 5, 2026, America/New_York  
**Evidence recorded:** 2026-09-06T01:42:49.786432+00:00 (September 5 in America/New_York)  
**Plugin version:** `0.1.4-m1-interaction-source`  
**Delivery:** Complete updated source package; no Unreal-compiled build or Windows executable.

## Result

All eight standalone C++ suites passed under GCC and Clang, and under Clang AddressSanitizer plus UndefinedBehaviorSanitizer. The retained Python tooling tests passed. New native relay, reflection, input, prompt projection and bridge changes have **not** been compiled or run in Unreal. **M1 has not passed its playable acceptance gate.**

## Executed checks

| Check | Actual result | What it establishes |
|---|---|---|
| GCC 14.2, C++17, strict warnings | **2,445 assertions; 0 failures** | Eight engine-independent helper suites compile and run. |
| Clang 17, C++17, strict warnings | **2,445 assertions; 0 failures** | Same eight suites on a second compiler; not additional playthroughs. |
| Clang AddressSanitizer + UndefinedBehaviorSanitizer | **Eight suites passed; no reported sanitizer errors** | Exercised standalone helper paths only. |
| Python 3.13 unittest | **31 tests passed** | Retained save-envelope, host-preflight and integration-report tooling tests using fixtures. |
| Source/data/document consistency | **616 checks; 0 failures** | Source layout, IDs, declarations, ownership safeguards, fail-closed defaults, local links and Python syntax. |

| C++ suite | Assertions | Relationship to M1.3 |
|---|---:|---|
| First Signal mission | 407 | Retained; rerun. |
| Campaign/transaction | 227 | Retained; rerun. |
| Save envelope | 583 | Retained; rerun. |
| UI flow/presentation | 650 | Retained; rerun. |
| Inventory view/catalogue | 80 | Retained; rerun. |
| Startup lifecycle | 94 | Retained; rerun. |
| Test-room manifest | 193 | Retained; rerun. |
| Interaction routing/offer rules | 211 | New. |
| **Total** | **2,445** | **2,234 retained + 211 new.** |

Assertions include generated permission combinations and boundary checks; they are not thousands of gameplay scenarios. Repeating the same suite under another compiler or sanitizers improves verification coverage rather than multiplying distinct tests. All 31 Python cases are retained, not newly added in this increment.

Actual evidence: [GCC](../../evidence/archive/m1_4/gcc_core.txt), [Clang](../../evidence/archive/m1_4/clang_core.txt), [sanitizers](../../evidence/archive/m1_4/sanitizers.txt), [Python](../../evidence/archive/m1_4/python_tests.txt), [structural checks](../../evidence/archive/m1_4/structural_checks.txt), and the [machine-readable run record](../../evidence/archive/m1_4/results.json). [Environment observations](../../evidence/archive/m1_4/environment.txt) record actual tool versions and the paths inspected.

## What the new checks cover

The exact helper source used by the native relay/bridge now tests one-action-per-frame and whole-call reentry rejection, held input, release after menus/loads, permission revisions, focus expiry, old focus-loss callbacks, all seven-flag world-permission combinations, radio/action offer selection and small combined input regressions. A duplicate door-dispatch regression checks that the abstract action cannot toggle twice in the same frame. It does not claim an actual Unreal door was moved.

The [plugin source delta](../../evidence/archive/m1_4/plugin_source_changes.json) records **7 added and 7 modified plugin files**, no removed plugin files, and the SHA-256 of the supplied M1.3 baseline archive. This is a content-hash comparison, not an invented repository revision. Prior M1.3 evidence is retained under `evidence/archive/m1_3/`, with earlier historical evidence also preserved. [SHA256SUMS.txt](../../evidence/archive/m1_4/SHA256SUMS.txt) covers current package files and excludes itself.

## Not executed

| Required verification | Status |
|---|---|
| Unreal Header Tool and UnrealBuildTool / real Windows host compiler | **NOT RUN** |
| Fifteen supplied Unreal automation tests, including four new interaction tests | **NOT RUN** |
| Real native E / gamepad input event ordering, key aggregation and UI-only transitions | **NOT RUN** |
| Actual Hyper selected-target forwarding and prompt presentation | **NOT RUN** |
| Real AGIS collection, transfer, repair, serialization and rollback | **NOT RUN** |
| Native bridge/relay collision, pawn lifecycle, bootstrap and cleanup behavior | **NOT RUN** |
| Editor map generation/audit and rendered UMG/controller flow | **NOT RUN** |
| Complete campaign loop and save/relaunch/Continue with the new relay | **NOT RUN** |
| Actual damaged-save/write-failure recovery and platform enumeration | **NOT RUN** |
| Windows PowerShell runner, cooking/packaging and external executable launch | **NOT RUN** |
| Runtime frame time, low-frame-rate input behavior and memory measurements | **NOT RUN** |

No host `.uproject` or actual vendor package API was present in the supplied mounted files, and no Unreal toolchain was found in the listed inspected paths or PATH. This is not a claim to have exhaustively searched every possible installation. No fake engine, replacement inventory, generated map, rendered prompt screenshot or executable was used as evidence.

## Integration and acceptance boundaries

`VendorEvents` is the relay's default and creates no new mapping. Its release/hold behavior requires the host to forward the real aggregate action-down state, including released samples after transitions. Optional `NativeEnhancedInput` creates E / face-left bindings, but actual held-key behavior across UI mode, mapping rebuild and controller changes remains a local test. Exactly one input route must own Interact.

Hyper still owns focus and visual prompts. The relay needs a current selected-target sample every frame; a focus-change-only event is not sufficient. Its sample-frame-plus-two-frame lease is a bounded ordering tolerance, not proof of correct vendor selection. Prompt data is advisory and does not certify inventory capacity or promise a successful transaction.

The shared bridge guard stops duplicate/reentrant actions in the same engine frame. It cannot infer physical release for a legacy caller invoking the bridge directly every frame. Such a host must adopt the relay or retain its own independently tested pressed-once route. The source must not be described as automatic Hyper integration.

The adapter remains deliberately unconfigured until the real AGIS implementation is supplied. Campaign/save schemas and IDs are retained, but binary save compatibility has not been tested. Existing save CRC/read-back validation is not a proof of crash-safe disk atomicity.

Follow [M1.4 wiring](../../docs/M1_4_INTERACTION_WIRING.md) and all **31 new interaction acceptance scenarios**, plus retained startup/UI/campaign acceptance. Only a real Windows test-room package completing pickup → storage → repair → transcript → verified save → exit/relaunch → Continue without loss or duplication passes M1. No M2 coastal route, final purchased art, fishing, boats or combat is claimed.
