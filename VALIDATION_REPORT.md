## Current local validation — September 9, 2026

The interaction/UI audio increment compiles in the independent Unreal 5.8.2 host.
All 54 native tests pass (53 clean, one existing AGIS warning), and its real PIE
campaign passes 69 gameplay assertions. Four actual mixer captures verify nonzero
baseline, Effects/Master mute and a 0.4988 RMS ratio at Effects 50%. These are not
listening or physical-device acceptance. All 72 pre-existing saves, both host maps
and project descriptors remain unchanged. See [the audio checkpoint](docs/M3_INTERACTION_AUDIO.md).

The source checks pass all 70 Python tests. The historical validation sections
below refer to prior source deliveries and do not override this current local run.
Local execution evidence remains outside the public repository.

# M1.10 Validation — Display Settings & Timed Keep/Revert

**Section date:** September 6, 2026, America/New_York  
**Plugin:** `0.1.10-m1-display-settings-source`  
**Source basis:** Supplied M1.9 ZIP, SHA-256 `f4e23c36a9d6a446b6a2f5e6fccff3ae037e0bd483bff2d41dfca5b4c55d45d4`; all 305 baseline manifest hashes verified before edits.  
**Delivery:** Complete source update. No Unreal build, native display capture or Windows executable.

## Result and boundary

All eighteen standalone C++ suites passed under GCC and Clang and under Clang AddressSanitizer plus UndefinedBehaviorSanitizer. All retained Python tooling tests passed. These execute the exact shared engine-independent helper headers and local tools, **not** the native display adapter, UHT/UBT, Slate/UMG, audio, purchased packages or actual Windows settings IO.

**M1 has not passed its playable acceptance gate.** M1.10 adds a bounded source display feature, not an assembled coast, finished graphics menu or a tested playable build.

## Executed checks

| Check | Actual result | Evidence boundary |
|---|---|---|
| GCC 14.2, C++17, strict warnings | **7,674 assertions; 0 failures** | Eighteen standalone suites. |
| Clang 17, C++17, strict warnings | **7,674 assertions; 0 failures** | Same suites with a second compiler, not additional gameplay scenarios. |
| Clang AddressSanitizer + UndefinedBehaviorSanitizer | **Eighteen suites passed; no reported sanitizer errors** | Exercised shared-helper paths only. |
| Python 3.13.5 unittest | **52 cases passed** | Retained local tooling/filesystem fixtures; no new Python feature suite claimed. |
| Source/data/document consistency | **1,592 checks; 0 failures** | Source contracts, metadata, reflected-header include ordering, Python syntax and local links; not native compilation. |
| Source preservation | **91 existing plugin files and all 31 prior data files unchanged byte-for-byte** | Exact content equality, not old Unreal binary-save compatibility. |

The C++ total is **7,389 retained + 285 new display assertions**. Counts are assertions, not playthroughs. Compilers and sanitizers do not multiply distinct gameplay coverage.

### Assertion accounting

| Suite | Assertions |
|---|---:|
| First Signal mission | 407 |
| Campaign/transaction | 227 |
| Save envelope | 583 |
| UI flow/presentation | 650 |
| Inventory view/catalogue | 80 |
| Startup lifecycle | 94 |
| Test-room manifest | 193 |
| Interaction routing/offer | 211 |
| Safe-return/checkpoint | 160 |
| Player options/look | 277 |
| Options codec/selection | 1,442 |
| Options storage/session | 164 |
| Sprint/input/speed ownership | 1,924 |
| M1.6 preference upgrade | 236 |
| Audio gain/routing/lifecycle | 519 |
| Audio preference upgrade | 111 |
| Audio playback/presentation | 111 |
| **New display trial/catalogue/presentation** | **285** |
| **Total** | **7,674** |

## What new coverage establishes

The standalone suite exercises candidate bounds/deduplication/order, reported-list membership, snapshot preservation, invalid or missing context, one trial at a time, fresh tickets, same-frame suppression, observed target matching, later-frame paint consent, exact deadline rejection on both poll and command paths, manual/automatic restore, no duplicate requests, pending versus failed restoration, stale epochs/tickets, lost foreground/owner, invalid/regressing clocks and frame numbers, repeated lifetime calls and consecutive session-only trial snapshots.

It also checks a small independent deadline/viewport/paint predicate matrix and the existing UI stack's new parent/child kinds. Returning to the target after a mismatch requires another presentation revision. Small original viewports can be snapshotted for return without being offered as newly selectable modes.

These tests are **not** a fake operating system, Unreal viewport or graphics driver. They establish decisions about command emission and eligibility, not actual resolution changes, native window focus, successful GPU/display negotiation, physical visibility or file persistence. No native `.cpp` file was compiled against fabricated engine headers to create a false UHT/UBT result.

The four new native tests cover unbound refusal and shared catalogue/deadline/modal rules. They perform no real window change or developer-profile write. There are **38 supplied Unreal automation tests across the plugin**, and a separate **38-case display manual/integration ledger**. Both remain unrun in this environment.

## Evidence and reproducibility

Actual completed logs: [GCC](evidence/gcc_core.txt), [Clang](evidence/clang_core.txt), [sanitizers](evidence/sanitizers.txt), [Python](evidence/python_tests.txt) and [source consistency](evidence/structural_checks.txt). Matching `*_execution.json` files record commands, environment overrides, start/end times, exit codes and captured output. [Machine-readable results](evidence/results.json) summarize scope. [Environment observations](evidence/environment.txt) record inspected executables/directories, not a scan of the user's PC.

The [initial full GCC run](evidence/initial_gcc_core.txt) also passed. The [initial structural run](evidence/initial_structural_checks.txt) found nine not-yet-created current evidence links during package assembly; that output is retained, and the completed package is checked separately. No timed-out or failed execution is relabelled as a pass.

Run the five source-check commands in [README](README.md). The retained Windows PowerShell runner is supplied but unrun here. No audio file, native screenshot, generated map, mock vendor adapter or actual-host test result is fabricated.

## Preservation and package provenance

The [plugin delta](evidence/plugin_source_changes.json) contains **7 added, 10 modified, 0 removed** paths against the actual M1.9 archive, with before/after hashes. Modifications are limited to descriptor metadata, the UI flow/panel and native menu lifecycle/dispatch/presentation. [Preservation detail](evidence/source_preservation.json) enumerates the unchanged files and hashes.

Campaign serialization/coordinator, item/world IDs, mission/story, inventory adapter, interaction, recovery, look/sprint, audio options/playback and the Coastal preference codec remain unchanged. Existing schemas 1/2/3 still read and preference writes remain schema 3. No new legacy encoder fixture or migration was needed. Source equality is not proof that an old Unreal binary save loaded.

M1.9's original evidence and manifest are retained under `evidence/archive/m1_9/`. Its [historical validation](docs/archive/M1_9_VALIDATION_REPORT.md) only has local links retargeted to retained locations. Current source results are not attributed backward or promoted into historical native execution. No Git commit, installed engine version or vendor revision is invented.

The final [SHA256SUMS manifest](SHA256SUMS.txt) hashes every delivered file except itself. The separate delivery-check JSON validates the final ZIP CRC, exact manifest membership, all file hashes and rerun extracted-source checks. Extraction verification is not a Windows build.

## Not executed

| Required verification | Status |
|---|---|
| UHT / UBT / actual Windows host compiler | **NOT RUN** |
| All 38 supplied Unreal automation tests | **NOT RUN** |
| Actual display-mode enumeration, `r.setres` sinks, window mode/size requests and normalization | **NOT RUN** |
| Native countdown, new-mode paint, scrolling/focus and cancellation at 720p/1080p and 100–150% text | **NOT RUN** |
| Controller-only and keyboard/mouse native display behavior, held input, Alt-Tab and mode-switch focus changes | **NOT RUN** |
| Real standard engine settings save/read-only failure/relaunch behavior | **NOT RUN** |
| Mixed-DPI/secondary monitors, device removal, driver failures, shutdown and delayed restore commands | **NOT RUN** |
| Actual AGIS pickup/transfer/repair/export/restore/rollback and item conservation | **NOT RUN** |
| Actual Hyper target/prompt/input integration | **NOT RUN** |
| Native camera/sprint/recovery, sound routing/playback and muted transcript behavior | **NOT RUN** |
| Native campaign/preference IO, damaged-slot recovery and fault injection | **NOT RUN** |
| Editor map generation, Windows PowerShell runner, cook/package and executable launch | **NOT RUN** |
| Frame-time, memory, native IO hitches and complete accessibility review | **NOT RUN** |

No actual host `.uproject` or installed paid package APIs are supplied. No Unreal executable/build tool was found in the recorded PATH names and inspected immediate directories. These are limited observations of this runtime, not claims about the user's machine.

## Remaining limitations and local gate

The native display path is opt-in, Windows/non-editor, single-player, standard-settings and non-XR only. Engine-reported modes are candidates, not monitor certifications. Restoration covers prior viewport size/window mode, not window position, monitor, refresh rate or HDR. A frame-polled owner cannot detect every transient/same-valued writer or perform recovery while not executing. A late or failed native request requires actual-host testing, not a stronger wording of the helper result.

No trial explicitly saves engine preference fields. Session-only Keep leaves them untouched. Only explicit validated Keep-plus-save invokes the engine writer; that void API is reported as a request, not verified IO. It serializes other existing engine user settings too. Host-specific auto-save behavior remains an integration responsibility. No campaign file is removed or reset to recover a display configuration.

Follow [M1.10 wiring](docs/M1_10_DISPLAY_WIRING.md), execute the [38 display cases](data/m1_10_display_acceptance.json) and all retained ledgers, then package the real Windows test room. Record exact versions, actual window/file behavior, screenshots and outcomes separately. Only the coherent launched campaign loop passes M1. See [What remains](docs/WHAT_REMAINS.md) for M2–M4.
