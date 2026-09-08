> Historical M1.2 evidence. These results are retained unchanged in scope; see [current validation](../../VALIDATION_REPORT.md).

# M1.2 Validation Report — Native Test-Room Interface

**Section date:** September 5, 2026  
**Evidence recorded:** September 6, 2026 UTC (September 5 in America/New_York)  
**Plugin version:** `0.1.2-m1-ui-source`  
**Delivery:** Updated source package; no Unreal-compiled or packaged game.

## Result

The new menu/input and inventory-view helper tests pass alongside the retained M1 mission, campaign and save-envelope suites. The native UI implementation, reflection declarations and vendor integrations have **not** been compiled or exercised in Unreal. **M1 has not passed its playable acceptance gate.**

## Executed checks

| Check | Actual result | Scope |
|---|---|---|
| GCC 14.2, C++17, strict warnings | **1,947 assertions; 0 failures** | Five engine-independent C++ suites. |
| Clang 17, C++17, strict warnings | **1,947 assertions; 0 failures** | The same five suites, not another 1,947 distinct scenarios. |
| Clang AddressSanitizer + UndefinedBehaviorSanitizer | **All five suites passed**; no reported sanitizer errors | Exercised helper paths only; not native Unreal or vendor code. |
| Python 3.13 unittest | **19 test cases passed** | Save-envelope tooling and read-only host preflight behavior in temporary fixtures. |
| Source/data/document consistency | **402 checks; 0 failures** | Layout, IDs, dependency declarations, fail-closed defaults, relative document links and Python syntax. |

Actual logs are retained in [GCC results](../../evidence/archive/m1_2/gcc_core.txt), [Clang results](../../evidence/archive/m1_2/clang_core.txt), [Python results](../../evidence/archive/m1_2/python_tests.txt), [sanitizer results](../../evidence/archive/m1_2/sanitizers.txt) and [structural checks](../../evidence/archive/m1_2/structural_checks.txt). The [machine-readable record](../../evidence/archive/m1_2/results.json) contains the recorded UTC timestamp and return codes. [Environment observations](../../evidence/archive/m1_2/environment.txt) record the inspected tool paths and directories, not a claim to have searched every possible installation location.

### Assertion accounting

| Suite | Assertions | Status |
|---|---:|---|
| First Signal mission rules | 407 | Retained; rerun. |
| Campaign and transaction rules | 227 | Retained; rerun. |
| Save envelope | 583 | Retained; rerun. |
| UI stack, tickets, focus, session and presentation rules | 650 | New. |
| Inventory-view validation and save-name parsing | 80 | New. |
| **Total** | **1,947** | **730 new assertions plus 1,217 retained.** |

Python comprises the retained ten save-inspector tests and nine new preflight tests. Generated state combinations and corrupt-byte assertions are checks, not separate gameplay playthroughs. Repeating tests with a second compiler or sanitizers increases verification coverage, not the number of distinct gameplay scenarios.

## What this source increment implements

Native UMG development screens now cover campaign start/selection/Continue, pause and guarded exit, journal, radio transcript, inventory/storage listing and item inspection. The controller-owned UI component manages modal input, focus, session invalidation and bridge blockers. An additional read-only AGIS view event supplies real item rows; the base implementation remains `NotConfigured` and returns no invented contents.

The updated source retains the original save protocol and all-or-none transaction boundary. Save-set enumeration discovers names only: it does not certify their payloads, and Continue still uses the existing validating loader. No campaign schema migration, replacement inventory, final art, coastline map or automatic vendor wiring was introduced.

The [plugin source delta](../../evidence/archive/m1_2/plugin_source_changes.json) records added/modified paths against the supplied M1 package by content hash, not by an invented repository revision. Original M1 evidence is preserved under `evidence/archive/m1/`; the earlier [validation report](../archive/M1_VALIDATION_REPORT.md) remains explicitly historical.

## Not executed

| Required verification | Status |
|---|---|
| Unreal Header Tool and UnrealBuildTool | **NOT RUN** |
| Actual Windows C++ compiler / Development Editor host build | **NOT RUN** |
| Seven supplied Unreal automation tests: four retained, three new | **NOT RUN** |
| Editor test-room generator and native widget rendering | **NOT RUN** |
| Real host preflight against the user's installation | **NOT RUN**; only fixture-based preflight tests ran. |
| Real AGIS transaction, view, restore and rollback implementation | **NOT RUN** |
| Hyper focus and interface bindings | **NOT RUN** |
| Keyboard/mouse and controller-only end-to-end menu behavior | **NOT RUN** |
| Actual pause ticks, held-input transitions and focus restoration | **NOT RUN** |
| Complete pickup, transfer, repair, transcript, save/relaunch/Continue | **NOT RUN** |
| Platform save enumeration, damaged-slot recovery and write failures | **NOT RUN** |
| Windows PowerShell runner, cooking, packaging and standalone launch | **NOT RUN** |
| Runtime frame time, memory and accessibility review | **NOT RUN** |

The runtime paths inspected did not contain an Unreal toolchain or the user's actual host `.uproject`. Purchased asset packages are not supplied. Passing standalone tests therefore does not establish that UHT accepts these declarations, that native widgets render correctly, that an installed Enhanced Input version behaves as intended, or that AGIS obeys the transaction/view contracts. These are concrete local integration gates, not details hidden by a completion percentage.

## Remaining risks and boundaries

Native APIs and widget layout require an actual host compile and screen/controller inspection. The list-based inventory is a development interface, not a finished visual grid. A painted transcript establishes a display opportunity, not proof the player read every line. Platform save-name enumeration can be unavailable; exact-name Continue is retained and failures remain explicit.

The shell owns input mode and pause during its menus; unrelated demo UI handlers must not compete with it. It balances its own movement/look locks and mapping context but is not a universal manager for arbitrary third-party menus. The fixed M1 character lifecycle is assumed; pawn replacement is rejected rather than silently rebound.

Real AGIS must provide coherent, revisioned views of its own items and implement all existing atomic/idempotent mutation and restore requirements. The UI does not make a provider's partial commit safe. The save envelope detects accidental corruption; read-back verification still does not prove crash-safe storage atomicity.

## Local completion gate

Follow [the M1.2 wiring guide](../M1_2_UI_WIRING.md), implement the real adapter/view contract and Hyper bridge, then initialize the native UI on the local PlayerController. Execute the [31-case UI acceptance ledger](../../data/m1_2_ui_acceptance.json), the retained M1 acceptance scenarios and all seven editor automation tests. Record actual engine, toolchain, vendor versions and evidence.

Only a successfully launched Windows test-room package that completes the full coherent campaign loop passes M1. The next world-building section remains M2: Cabin Cove, Shoreline Trail, Old Dock and Rail Overlook, after that gate—not a claimed feature of this source delivery.
