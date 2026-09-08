# M1.8 Validation — Routed Audio Options and Preference Upgrade

**Section date:** September 6, 2026, America/New_York  
**Evidence recorded:** 2026-09-06T14:31:25.604898+00:00  
**Plugin:** `0.1.8-m1-audio-options-source`  
**Delivery:** Full updated source from the supplied M1.7 ZIP; no native Unreal plugin build, audio content or Windows executable.

## Result and boundary

All sixteen standalone C++ suites passed under GCC and Clang and under Clang AddressSanitizer plus UndefinedBehaviorSanitizer. All retained Python tooling cases passed. These checks exercise engine-independent headers and local tools, **not** actual Unreal audio mixing, real sound routing, native preference IO, rendered controls or vendor gameplay. The final structural check result is recorded below and in the linked evidence.

**M1 has not passed its playable acceptance gate.** This increment implements routed volume-control source from the outstanding presentation requirements. It is not M2, an audio asset/playback delivery, an engine build, or a completed accessibility pass.

## Executed checks

| Check | Actual result | Scope |
|---|---|---|
| GCC 14.2, C++17, strict warnings | **7,278 assertions; 0 failures** | Sixteen engine-independent suites. |
| Clang 17, C++17, strict warnings | **7,278 assertions; 0 failures** | Same assertions with a second compiler; not additional playthroughs. |
| Clang AddressSanitizer + UndefinedBehaviorSanitizer | **All sixteen suites passed; no reported sanitizer errors** | Exercised shared helper paths only. |
| Python 3.13.5 unittest | **52 cases passed** | Retained save/preflight/report/layout tooling, using temporary fixtures. |
| Source/data/document consistency | **1,226 checks; 0 failures** | IDs, declarations, source constraints, native test inventory, unresolved asset claims, fixtures, Python syntax and local links; not UHT/UBT. |
| Original encoder compatibility | **Actual frozen M1.6 and M1.7 output read and explicitly upgraded in tests** | Shared serialized format compatibility, not native Windows slot IO or older game binaries. |

The initial GCC run reported two stale six-row selection-wrap expectations in the retained Sprint suite. Those expectations now use the current last row, preserving the wrap rule for ten fields. The complete suite was rerun successfully. Its [initial output](../../evidence/archive/m1_8/initial_gcc_core.txt) is retained rather than represented as a pass. An intermediate source/package check found missing-yet-to-be-assembled evidence links and one archived manifest-link target; the [initial output](../../evidence/archive/m1_8/initial_structural_checks.txt) is retained and final links are checked separately.

### Assertion accounting

| Suite | Assertions | Relationship to M1.7 |
|---|---:|---|
| First Signal mission | 407 | Retained. |
| Campaign/transaction | 227 | Retained. |
| Save envelope | 583 | Retained. |
| UI flow/presentation | 650 | Retained. |
| Inventory view/catalogue | 80 | Retained. |
| Startup lifecycle | 94 | Retained. |
| Test-room manifest | 193 | Retained. |
| Interaction routing/offer | 211 | Retained. |
| Safe-return/checkpoint | 160 | Retained. |
| Player options/look | 277 | Retained. |
| Options codec/selection | 1,442 | Previous 1,170 plus 272 assertions as 16 added bytes enter corrupt-bit/truncation loops; future-version sentinel is now 4. |
| Options storage/session | 164 | Retained. |
| Sprint/input/speed ownership | 1,924 | Retained; field count/wrapping expectations updated to ten rows. |
| M1.6 preference upgrade | 236 | Retained; new-write schema/size expectations updated, frozen old fixture unchanged. |
| Audio gain/routing/lifecycle | 519 | New. |
| Audio schema1/schema2/schema3 upgrade | 111 | New. |
| **Total** | **7,278** | **6,376 previous + 902 added assertion executions.** |

Counts are assertions, not thousands of player scenarios. Repeated refresh, invalid-field and corrupted-byte assertions are not playthroughs. Compilers/sanitizers do not multiply distinct coverage. All 52 Python cases are retained; no new Python feature-test suite is claimed.

## Evidence and reproducibility

Completed logs: [GCC](../../evidence/archive/m1_8/gcc_core.txt), [Clang](../../evidence/archive/m1_8/clang_core.txt), [sanitizers](../../evidence/archive/m1_8/sanitizers.txt), [Python](../../evidence/archive/m1_8/python_tests.txt), and [source consistency](../../evidence/archive/m1_8/structural_checks.txt). The [machine-readable result](../../evidence/archive/m1_8/results.json) records outcomes and scope; per-stage execution JSON records actual commands, return codes, timestamps and captured output. [Environment observations](../../evidence/archive/m1_8/environment.txt) identify the tools and paths checked in this runtime, not a scan of the user's computer.

Run `tools/run_core_tests.sh` with GCC or `CXX=clang++`, `tools/run_sanitizer_tests.sh`, Python unittest discovery and `tools/verify_package.py`. The Windows PowerShell runner is retained but unrun here. No Unreal stub compiler, mock audio device, replacement AGIS or fake native screenshot was used to convert standalone checks into integration evidence.

The new tests cover exact gain math (including real zero requests), independent channels, range/step failures, class identity/topology metadata, one active mix command lifetime, idempotent cleanup, unchanged-gain suppression, modal ticket rules, strict format lengths, old-value preservation, explicit upgrade in either slot, failed/ambiguous writes, external changes, damaged-newer fallback and retained category values through Master mute.

The old-format fixture was generated by compiling the actual supplied M1.7 encoder **before edits**. Its [provenance](../../tests/fixtures/m1_7_options_fixture.json) records source archive/encoder hashes, generation 29, original values and output bytes/hash. The matching header and original M1.6 fixture are exercised by the new tests. Preference IO callbacks are in-memory fault fixtures, not the Unreal platform API being simulated and called real execution.

## Source preservation and delivery

[Plugin delta](../../evidence/archive/m1_8/plugin_source_changes.json) records **4 added, 9 modified, 0 removed** paths against the actual supplied M1.7 archive. No repository revision is invented. [Protected-source comparisons](../../evidence/archive/m1_8/source_preservation.json) verify **46 enumerated files unchanged byte-for-byte**, covering mission, campaign/persistence, inventory, interaction, recovery, movement/look and startup source. UI/options, the preference codec and one current-schema native assertion are intentionally changed.

Campaign schema, logical world/item IDs, receipt semantics and story data have not been migrated. Exact source equality is not proof an old Unreal binary save loaded. M1.8 preferences accept schema1/2 and explicitly write schema3; this is a separate local preference-format change with a downgrade boundary.

Prior M1.7 logs and its manifest are retained under `evidence/archive/m1_7/`. The [historical M1.7 report](M1_7_VALIDATION_REPORT.md) only has relative links retargeted to the retained locations; its results remain historical. The final `SHA256SUMS.txt` hashes packaged files except itself. The separate delivery-check JSON verifies ZIP CRC, exact manifest membership, all hashes and extracted-source runs. No extraction-only check is labelled a Windows build.

## Not executed

| Required verification | Status |
|---|---|
| Unreal Header Tool / UnrealBuildTool / actual Windows host compiler | **NOT RUN** |
| Thirty supplied Unreal automation tests (26 retained, four `Coastal.M1AudioOptions`) | **NOT RUN** |
| Real Sound Class routing, native SoundMix push/override/pop and audible output | **NOT RUN** |
| Device changes, cleanup/GC, simultaneous/retained mixes, silent-loop behavior and muted startup | **NOT RUN** |
| Actual preference-slot IO, schema upgrade, write fault injection and old binary compatibility | **NOT RUN** |
| Native ten-field options rendering, scrolling, focus and 720p/1080p text review | **NOT RUN** |
| Keyboard/mouse and controller-only end-to-end input/audio/options tests | **NOT RUN** |
| Actual AGIS pickup/transfer/repair/restore/rollback and item conservation | **NOT RUN** |
| Actual Hyper selected-target/prompt/input integration | **NOT RUN** |
| Muted-radio mission completion, full campaign save/relaunch/Continue and recovery loop | **NOT RUN** |
| Editor map generation, Windows PowerShell runner, cooking/packaging and executable launch | **NOT RUN** |
| Performance, device latency, memory and complete accessibility review | **NOT RUN** |

No actual host `.uproject` or purchased package APIs are supplied. No Unreal executable/build tool was found under the recorded executable names and inspected directories. These are limitations of this delivery runtime, not assertions about the user's PC.

## Remaining limits and local gate

The optional consumer requires three distinct isolated dedicated classes and one controller/device lifetime. A correct class reference or enabled opt-in flag does not certify that any real sound uses that class. Native APIs return void; submitting a volume command is not audible-output confirmation. The component does not play audio, create sound assets, modify virtualization, detect all competing same-class mixes or configure a vendor graph. Master controls only the routed categories; cleanup removes that contribution and exposes the underlying host mix.

The 0.1-second interpolation is original tuning. True zero may interact with the real source's silent-voice/virtualization policy. Start host sounds only after options initialization and verify muted relaunch. Mute never changes mission/transcript source, but the real host must be checked for duplicate playback-completion callbacks. Neither a helper pass nor a valid routing flag proves this integration.

Preferences retain the existing slot names, read schemas1/2/3, and write schema3 only on explicit verified save. Older plugins reject upgraded slots; back up both before upgrade. An unverified write can reach disk. CRC/read-back is not a cross-process lock or crash-atomicity guarantee; raw platform reads allocate a file before bounded decoding. Test faults on disposable backups.

Follow [audio wiring](../M1_8_AUDIO_WIRING.md), run the 30 native tests and all [36 new acceptance cases](../../data/m1_8_audio_acceptance.json) plus retained ledgers with the real host/vendors. Record actual engine/toolchain/vendor versions, real sound/class paths, runtime observations and screenshots separately.

**Only the actual launched Windows test-room campaign loop passes M1. M2's production coast and final audio/art remain later work.**
