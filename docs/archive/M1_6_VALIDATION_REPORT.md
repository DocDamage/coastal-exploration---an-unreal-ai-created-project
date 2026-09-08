> Historical M1.6 report. Relative links retargeted to retained artifacts; current M1.7 results are in the root validation report.

# M1.6 Validation — Player Options & Readability

**Section date:** September 5, 2026, America/New_York  
**Evidence recorded:** 2026-09-06T02:52:56.396645+00:00 (September 5 in America/New_York)  
**Plugin version:** `0.1.6-m1-player-options-source`  
**Delivery:** Complete source update from the supplied M1.5 ZIP; no Unreal-compiled or Windows playable build.

## Result

All twelve standalone C++ suites passed with GCC and Clang and under Clang AddressSanitizer plus UndefinedBehaviorSanitizer. The retained Python tooling tests also passed. These results exercise the exact engine-independent helpers used by the native source, plus local tools—not Unreal camera/input/rendering, the real platform save API, or vendor gameplay. **M1 has not passed its playable acceptance gate.**

This increment brings a bounded part of the foundation's planned M3 options/readability work forward into the systems test room. It does not include M2's coast, finished accessibility/settings, purchased content or a playable executable.

## Executed checks

| Check | Actual result | Boundary |
|---|---|---|
| GCC 14.2, C++17, strict warnings | **4,148 assertions; 0 failures** | Twelve shared engine-independent suites. |
| Clang 17, C++17, strict warnings | **4,148 assertions; 0 failures** | The same assertions, not another set of player scenarios. |
| Clang AddressSanitizer + UndefinedBehaviorSanitizer | **Twelve suites passed; no reported sanitizer errors** | Exercised helper paths only; not native engine/vendor execution. |
| Python 3.13.5 unittest | **52 cases passed** | Retained save/preflight/report/safety-layout tooling in temporary fixtures. |
| Source/data/document consistency | **894 checks; 0 failures** | Source layout, declarations, specifications, local document links and Python syntax; not UHT/UBT. |

### Assertion accounting

| C++ suite | Assertions | Relationship to M1.5 |
|---|---:|---|
| First Signal mission | 407 | Retained; rerun. |
| Campaign/transaction | 227 | Retained; rerun. |
| Save envelope | 583 | Retained; rerun. |
| UI flow/presentation | 650 | Retained; rerun. |
| Inventory view/catalogue | 80 | Retained; rerun. |
| Startup lifecycle | 94 | Retained; rerun. |
| Test-room manifest | 193 | Retained; rerun. |
| Interaction routing/offer | 211 | Retained; rerun. |
| Safe-return/checkpoint | 160 | Retained; rerun. |
| Player options/look and visible-body geometry | 277 | New. |
| Options codec/slot selection | 1,102 | New. |
| Options storage/session protocol | 164 | New. |
| **Total** | **4,148** | **2,605 retained + 1,543 new.** |

Many codec assertions enumerate individual corrupted bits. These counts are assertions, not thousands of gameplay scenarios. Repeating them with another compiler or sanitizers broadens verification without multiplying distinct cases. The 52 Python cases are retained; no new Python feature suite is claimed.

Actual logs are retained in [GCC](../../evidence/archive/m1_6/gcc_core.txt), [Clang](../../evidence/archive/m1_6/clang_core.txt), [sanitizers](../../evidence/archive/m1_6/sanitizers.txt), [Python](../../evidence/archive/m1_6/python_tests.txt), and [source consistency](../../evidence/archive/m1_6/structural_checks.txt). The [machine-readable record](../../evidence/archive/m1_6/results.json) stores commands, environments, timestamps and return codes. [Environment observations](../../evidence/archive/m1_6/environment.txt) record inspected paths, not a scan of the user's computer.

## What the new tests establish

The new helpers exercise bounded/default settings, adjustments, independent mouse/stick mathematics, inversion, nonfinite input rejection, delta-time policy, same-device same-frame rejection, neutral stick rearming after modal/session/recovery transitions and parent/child panel behavior. The visible-body rectangle test is a geometry helper, not evidence that Slate actually painted readable text.

Preference tests exercise the actual shared `OptionsSession` protocol with in-memory read/write callbacks: no automatic write, draft/session-only behavior, alternating generations, exact read-back, incompatible/corrupt/unreadable files, ambiguous equal generations, external changes, partial/unverified writes and synchronous callback reentry. They do not substitute callbacks for `UGameplayStatics` and claim the platform API ran. Real platform failure handling remains an integration gate.

Source inspection also corrected the editable-text font update to use the widget-style API rather than an unsupported direct setter. Header inclusion/order, source constraints and string-level guards were checked, but only a real host compile can establish UHT/UBT/API compatibility. Native declarations can still contain defects not revealed by standalone compilation.

## Preservation and package provenance

[Plugin content delta](../../evidence/archive/m1_6/plugin_source_changes.json) records **9 added, 12 modified, 0 removed** plugin files against the actual supplied M1.5 archive, with before/after hashes. No Git commit, installed engine version or vendor revision is invented. Campaign coordinator/serialization, inventory adapter, item/world IDs, mission/story and return/checkpoint implementations are retained; menu presentation and the shared UI rules are intentionally changed. Unchanged source does not prove an old Unreal binary save successfully loads.

Prior M1.5 logs are retained in `evidence/archive/m1_5/`, with its prior manifest and [historical validation report](M1_5_VALIDATION_REPORT.md). Relative links in the archived report are retargeted to retained evidence; its historical results are not promoted to current native tests. [SHA256SUMS.txt](../../evidence/archive/m1_6/SHA256SUMS.txt) hashes delivered files and excludes itself. The separate delivery-check JSON verifies the final ZIP, manifest and extracted-source checks.

## Not executed

| Required verification | Status |
|---|---|
| Unreal Header Tool, UnrealBuildTool and actual Windows host compiler | **NOT RUN** |
| Twenty-two supplied Unreal automation tests, including three new `Coastal.M1Options` tests | **NOT RUN** |
| Native `UGameplayStatics` preference reads/writes, file enumeration/isolation and platform failure injection | **NOT RUN** |
| Actual active-camera detection, FOV behavior, legacy input scales and real host mouse/stick routing | **NOT RUN** |
| Native UMG/Slate rendering at 100–150% text, scrolling, focus, save-name editing and transcript visibility | **NOT RUN** |
| Controller-only and keyboard/mouse end-to-end UI/camera/recovery behavior | **NOT RUN** |
| Actual AGIS pickup, transfer, repair, restore, rollback and item conservation | **NOT RUN** |
| Actual Hyper current-target/prompt and held-input integration | **NOT RUN** |
| Full campaign save/relaunch/Continue loop with independently restored preferences | **NOT RUN** |
| Windows PowerShell runner, editor map recipe, cooking/packaging and executable launch | **NOT RUN** |
| Frame-time, memory, real low-frame-rate behavior and accessibility review | **NOT RUN** |

The supplied material does not contain the actual host `.uproject` or purchased package APIs. No Unreal executable/build tool was located in PATH or the listed inspected directories. No rendered UI capture, generated map, paid asset or executable is fabricated as evidence.

## Remaining limits and local gate

Camera/look support is opt-in for the fixed local character with one active perspective camera. The host must forward real separate mouse deltas and normalized stick state, including true neutral after menus; merely setting the opt-in flag is not integration. Custom camera managers, multiple cameras and independent look handlers can still require host changes. Text scaling affects native Coastal widgets only, not Hyper/vendor text. Audio settings, resolution/graphics, remapping, sprint hold/toggle and production accessibility remain unfinished.

Preferences use two independent small slots, not campaign state. CRC and read-back detect accidental corruption; they do not prove crash-safe disk atomicity, hostile-file safety or cross-process exclusion. Raw platform reads can allocate a full file before the bounded decoder examines it. Use a single running profile writer and disposable backed-up files for faults. Failed/ambiguous writes lock further preference writes in that UI lifetime, while explicit session-only apply remains available. Campaign saves are not deleted or reset.

Follow [M1.6 wiring](../M1_6_OPTIONS_WIRING.md), compile the actual host, and execute all 22 native tests and the [34-case options ledger](../../data/m1_6_options_acceptance.json), alongside retained M1 campaign/UI/startup/interaction/recovery acceptance. Inspect real 1920×1080 and 1280×720 layouts, actual controller input and platform IO faults. Record real engine/toolchain/vendor versions and evidence separately.

**Only the actual Windows test-room campaign loop passes M1. M2 coast assembly remains later work.**
