> Historical M1.3 source-only record, retained from the supplied package. See the current root README and validation for M1.4.

# M1.3 Validation — Checked Startup & Integration Diagnostics

**Section date:** September 5, 2026, America/New_York  
**Evidence recorded:** September 6, 2026 UTC (September 5 in America/New_York)  
**Plugin version:** `0.1.3-m1-startup-source`  
**Delivery:** Full updated source; no Unreal build or Windows executable.

## Result

Standalone checks passed for the new startup-state and exact M1 manifest rules alongside all retained mission, campaign, envelope and UI helpers. The native bootstrap/provider audit, shared collision check and editor script have **not** been executed in Unreal. **M1 has not passed its playable acceptance gate.**

## Executed checks

| Check | Actual result | Boundary |
|---|---|---|
| GCC 14.2, C++17, strict warnings | **2,234 assertions; 0 failures** | Seven engine-independent suites. |
| Clang 17, C++17, strict warnings | **2,234 assertions; 0 failures** | Same seven suites, not another set of distinct player scenarios. |
| Clang AddressSanitizer + UndefinedBehaviorSanitizer | **Seven suites passed; no reported sanitizer errors** | Exercised standalone helper paths only. |
| Python 3.13 unittest | **31 cases passed** | Report/file safeguards plus retained host-preflight and save-envelope tooling, using temporary fixtures. |
| Source/data/document consistency | **519 checks; 0 failures** | Layout, IDs, manifests, declarations, source constraints, local links and Python syntax. |

| C++ suite | Assertions |
|---|---:|
| First Signal mission rules | 407 |
| Campaign/transaction rules | 227 |
| Save envelope | 583 |
| UI flow/presentation | 650 |
| Inventory view/catalogue | 80 |
| Startup lifecycle | 94 |
| M1 room manifest | 193 |
| **Total** | **2,234** |

There are **287 new assertions** and 1,947 retained. Manifest subsets, state transitions and corrupted-byte checks count as assertions, not playthroughs. Python comprises nineteen retained tests and twelve new report tests. Running the same logic under another compiler or sanitizer does not multiply the number of gameplay scenarios.

Actual logs: [GCC](../../evidence/archive/m1_3/gcc_core.txt), [Clang](../../evidence/archive/m1_3/clang_core.txt), [Python](../../evidence/archive/m1_3/python_tests.txt), [sanitizers](../../evidence/archive/m1_3/sanitizers.txt), [structural checks](../../evidence/archive/m1_3/structural_checks.txt). The [machine-readable record](../../evidence/archive/m1_3/results.json) records actual command outcomes. [Environment observations](../../evidence/archive/m1_3/environment.txt) list the paths checked, not an exhaustive claim about every possible engine installation. [Plugin delta](../../evidence/archive/m1_3/plugin_source_changes.json) compares content hashes against the supplied M1.2 ZIP; no repository revision is invented.

## Not executed

| Required check | Status |
|---|---|
| Unreal Header Tool / UnrealBuildTool / Windows host compiler | **NOT RUN** |
| Eleven supplied editor automation tests, including four new startup tests | **NOT RUN** |
| Real startup preflight/binding/partial-failure behavior | **NOT RUN** |
| Real registered AGIS provisional-session export/views and read diagnostics | **NOT RUN** |
| Actual AGIS mutation atomicity, idempotency, rollback and critical-item conservation | **NOT RUN** |
| Hyper interface, focus, prompt and pressed-once interaction wiring | **NOT RUN** |
| Native UMG rendering, collision traces, input-context cleanup and ownership | **NOT RUN** |
| Read-only editor audit and original map generator in Unreal | **NOT RUN** |
| Controller/keyboard full campaign loop, save/relaunch/Continue, disk-failure recovery | **NOT RUN** |
| Windows PowerShell runner, cooking/packaging, executable launch and performance | **NOT RUN** |

The host `.uproject`, local purchased package APIs and an Unreal toolchain were not found in the inspected supplied runtime locations. No engine build or map result has been fabricated. No test fixture is described as an installed vendor or live engine.

## Boundaries and remaining risks

The bootstrap is source for a preferred initialization path, not automatic AGIS/Hyper integration. It depends on the real provider becoming ready before the explicit call. The strict manifest accepts the authored M1 room only; it is not a save migration or general world validator. A manifest pass does not prove collision, controller behavior or a complete mission route.

Provider checks inspect declared metadata, validated display views and simple read behavior. They cannot prove opaque vendor payload consistency or that a provider reports its own state truthfully. Initial views and requirement checks do not validate later BuildNewInventory, mutation or restore behavior. Real fault tests are mandatory.

Partial-binding failures deliberately poison future campaign operations and require relaunch. Diagnostics use structured Blueprint data, the log and development on-screen text; no new shipping-quality startup error screen is claimed. Input cleanup and capsule checks are native source changes and remain unverified in Unreal.

The campaign schema and existing IDs remain unchanged. That is a source-level compatibility statement, not proof an old binary save loaded successfully. Alternating save read-back and CRC remain corruption checks, not proven crash-safe disk atomicity.

The [startup wiring guide](../../docs/M1_3_STARTUP_WIRING.md) and [26-case ledger](../../data/m1_3_startup_acceptance.json) define the next local work alongside retained M1/M1.2 checks. Only the actual packaged test-room campaign loop passes M1. No coastline, finished asset integration, fishing, boats or combat was silently included.
