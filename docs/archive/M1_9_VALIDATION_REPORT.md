# M1.9 Validation — Ambience & Transcript Playback

**Section date:** September 6, 2026, America/New_York  
**Plugin:** `0.1.9-m1-audio-playback-source`  
**Source basis:** Supplied M1.8 archive, SHA-256 `804239da210916336f96974b408c4ef30a5ec614d74e6bf7c5a420a13c0d1865`. All 275 baseline manifest entries were verified before edits.  
**Delivery:** Full source update. No Unreal-compiled plugin, actual audio output test or Windows executable.

## Result and boundary

All seventeen standalone C++ suites passed with GCC and Clang and with Clang AddressSanitizer plus UndefinedBehaviorSanitizer. All retained Python tooling cases passed. These checks exercise the shared engine-independent rules and local tools, not the native audio component, reflection, sound routing, UI, purchased packages or Windows platform IO.

**M1 has not passed its playable acceptance gate.** M1.9 supplies actual playback source for assigned sounds, but no recording or other audio asset. It is not a new map, finished coastal presentation or a production playable build.

## Executed checks

| Check | Actual result | Scope |
|---|---|---|
| GCC 14.2, C++17, strict warnings | **7,389 assertions; 0 failures** | Seventeen shared engine-independent suites. |
| Clang 17, C++17, strict warnings | **7,389 assertions; 0 failures** | The same suites under a second compiler. |
| Clang AddressSanitizer + UndefinedBehaviorSanitizer | **Seventeen suites passed; no reported sanitizer errors** | Exercised shared helper paths only. |
| Python 3.13.5 unittest | **52 cases passed** | Retained tooling/filesystem fixtures; no new Python feature suite claimed. |
| Source/data/document consistency | **1,409 checks; 0 failures** | Source constraints, IDs, source/spec wiring, Python syntax and local links; not UHT/UBT. |
| Source preservation | **89 existing plugin files and all 27 prior data files unchanged byte-for-byte** | Content equality, not old binary-save compatibility. |

The C++ total is **7,278 retained assertions plus 111 new playback assertions**. Counts are assertions, not playthroughs. Repeating them under a second compiler or sanitizers does not multiply distinct gameplay coverage.

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
| **New audio playback/presentation** | **111** |
| **Total** | **7,389** |

## New coverage

The shared playback suite checks opt-in lifetime, missing/invalid source metadata, silent title behavior, delayed first bed, one Play attempt, nested-menu pause/resume, paint-before-radio, repeated refresh after natural finish or failed Play submission, cancellation before and after paint, immediate suspension, explicit fresh-ticket replay, session replacement, stale epochs/tickets, inactive campaigns, optional channels and idempotent teardown.

Exact command-mask assertions check both expected actions and the absence of accidental extra starts/stops. Volume is deliberately not an input to the playback state machine: mute is handled by the retained mix rather than by starting/stopping or acknowledging a transcript. No test fixture is described as audible output or a real Unreal device.

The four new native automation tests cover unbound rejection, shared presentation rules, source rules and unbound routing-release reentry. They bring the full supplied native inventory to **34 tests**. The new host/manual acceptance ledger separately contains **34 scenarios**. Both inventories remain unrun here.

## Evidence and reproducibility

Completed output is retained in [GCC](../../evidence/archive/m1_9/gcc_core.txt), [Clang](../../evidence/archive/m1_9/clang_core.txt), [sanitizers](../../evidence/archive/m1_9/sanitizers.txt), [Python](../../evidence/archive/m1_9/python_tests.txt) and [source checks](../../evidence/archive/m1_9/structural_checks.txt). Each completed stage has a matching `*_execution.json` with commands, timestamps, exit code, stdout and stderr. [Machine-readable results](../../evidence/archive/m1_9/results.json) summarize actual scope and outcomes. [Environment observations](../../evidence/archive/m1_9/environment.txt) list the inspected tool names and paths, not a scan of the user's computer.

Reproduce with the five source-check commands in [README](../../README.md). The retained PowerShell runner is supplied but unrun. No mock engine, synthetic AGIS provider, fabricated sound asset or native screenshot was used to turn shared-helper results into integration evidence.

[Plugin delta](../../evidence/archive/m1_9/plugin_source_changes.json) records **6 added, 6 modified and 0 removed plugin files** against the actual baseline archive. [Preservation detail](../../evidence/archive/m1_9/source_preservation.json) lists unchanged files and hashes. The modifications are limited to the descriptor, native UI integration, and audio-routing access/release hooks. Campaign/mission/inventory, recovery, look/sprint, preference fields/codec and prior data remain unchanged. No repository revision is invented.

Prior M1.8 evidence and its original manifest are retained under `evidence/archive/m1_8/`; [historical validation](../../docs/archive/M1_8_VALIDATION_REPORT.md) only has its local links retargeted to retained evidence. Current tests are not attributed backward to the earlier delivery. [SHA256SUMS](../../evidence/archive/m1_9/SHA256SUMS.txt) covers final package files except itself. The separate delivery-check JSON verifies the final ZIP, exact manifest membership, hashes and extracted-source runs.

## Not executed

| Required verification | Status |
|---|---|
| Unreal Header Tool, UnrealBuildTool and actual Windows host compiler | **NOT RUN** |
| All 34 supplied Unreal automation tests | **NOT RUN** |
| Actual UAudioComponent registration, playback, pause/resume, stop/destruction and audio-device commands | **NOT RUN** |
| Assigned sound routing, PlayWhenSilent behavior, concurrency/streaming, muted startup and audible output | **NOT RUN** |
| Native source/device/UI loss, pre-mix-release ordering, GC and repeated teardown | **NOT RUN** |
| Actual transcript paint visibility, paused UI voice, cancellation, early Continue and silent completion | **NOT RUN** |
| Native UMG/controller behavior at 720p/1080p and 100–150% text | **NOT RUN** |
| Real AGIS pickup, transfer, repair, export, restore, rollback and item conservation | **NOT RUN** |
| Real Hyper current-target/prompt/input integration | **NOT RUN** |
| Native campaign/preferences IO, damaged-slot recovery and platform write-failure injection | **NOT RUN** |
| Editor map generation, PowerShell runner, Windows cooking/packaging and executable launch | **NOT RUN** |
| Frame time, memory, voice counts/latency and complete accessibility review | **NOT RUN** |

No host `.uproject` or installed paid package APIs are supplied. No Unreal executable/build tool was found under the names and directories recorded in this runtime. These are environment observations, not claims about the user's PC or an exhaustive filesystem search.

## Remaining limits and local gate

Playback supports one fixed local-controller lifetime, one 2D ambience bed and one finite UI radio recording. It is not footsteps, ambience zones/crossfades, a spatial emitter, final audio content or a new world. Actual assets must be assigned. Valid metadata and an accepted opt-in are not proof of audible output or every branch of a complex cue. PlayWhenSilent is required by policy but must be verified with real sources; the metadata duration cap is not a runtime timer.

Native Play/Stop submit commands; they do not acknowledge a speaker result. The source refuses automatic retries on a dropped/finished/muted voice. It stops owned sources before removing their volume mix, but actual audio-thread timing and unrelated sources remain host responsibilities. Keep UI/playback owners ticking, use one writer, and release before deliberately disabling their lifetime. Lost devices/sources are not silently rebound.

No new preference migration occurs. Existing schemas 1/2/3 remain readable; explicit writes stay schema 3. All inherited save/CRC/read-back limits remain, including no proven crash-safe storage atomicity or cross-process locking. Fault-test only disposable backed-up profiles.

Follow [local wiring](../../docs/M1_9_PLAYBACK_WIRING.md), execute [all 34 playback acceptance cases](../../data/m1_9_playback_acceptance.json) and retained ledgers with the real host, then package Windows. Record actual engine/toolchain/vendor versions, sound/class paths, runtime/audio observations and screenshots separately. Only the real coherent test-room campaign loop passes M1. M2 remains the production coastal route after that gate.
