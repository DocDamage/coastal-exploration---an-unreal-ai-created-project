# M1.10 — Display Settings and Timed Keep/Revert

**Project:** Coastal Exploration / First Signal  
**Section date:** September 6, 2026, America/New_York  
**Plugin:** `0.1.10-m1-display-settings-source`  
**Basis:** Actual supplied M1.9 source archive; SHA-256 `f4e23c36a9d6a446b6a2f5e6fccff3ae037e0bd483bff2d41dfca5b4c55d45d4`. Its 305 manifest hashes were verified before modification.  
**Delivery:** Complete source update, not a compiled Unreal plugin or Windows executable.

## 1. Why this section

The foundation's M3 presentation/stability work and the latest supplied reports leave display/graphics settings unfinished. This increment implements the bounded **resolution/window-mode** portion in the existing native menu. It is an original sequencing decision within that unfinished work, not a previously promised M1.10 milestone and not evidence of M1 completion.

Before implementation, the remaining-work review identified four stages: actual M1 engine/vendor/test-room acceptance; M2's coastal opening; M3's remaining presentation, input and performance work; and M4's tested Windows delivery. That assessment remains in [What remains](WHAT_REMAINS.md). No completion percentage is invented from standalone assertion totals.

## 2. Delivered player behavior

Session and Pause now expose **Display settings**, separate from the ten-field Player options screen. The player chooses an available window mode and resolution in a draft. Next mode preserves a matching size where possible; otherwise it selects an available size in that category. Previous/Next resolution cycles that category. Refresh rereads the actual engine catalogue and discards the draft. Back discards without changing the window.

**Test draft for 15 seconds** opens a child confirmation dialog, captures the actual current viewport size/mode, and submits one native resolution request. Revert is the initial focus. The countdown uses a monotonic platform clock, not game delta, and continues while the normal native menu pauses gameplay. At the first running tick or command observation at/after the deadline, the model requests restoration instead of allowing Keep.

The dialog has Revert, Keep for this session only, and Keep and request engine settings save. Keep stays unavailable until the requested native viewport size and mode have been observed after the opening frame, the dialog has a fresh body-presentation opportunity in that mode, and a later valid command arrives before the deadline. A settings object's requested values are not treated as evidence that the window changed.

A fresh later-frame Escape/B or Revert can cancel before body paint; it still requires the correct top ticket and frame debounce. This narrow exception can only undo a temporary display trial. It does not let an unpainted transcript acknowledge itself or bypass other campaign commands. Repeated/held native input retains the existing UI protections.

## 3. Native catalogue and supported host

| Area | Admission policy |
|---|---|
| Windowed | Engine-reported convenient windowed sizes; the actual current windowed size may also be retained when within candidate bounds. |
| Fullscreen | Engine-reported supported fullscreen sizes only when the live viewport reports exclusive fullscreen support. |
| Borderless | Engine-reported desktop size; resolution is not independently varied in this category. |
| New candidate bounds | Width 1280–7680, height 720–4320; valid known mode; duplicates removed. These are original source safeguards, not hardware claims. |
| Catalogue bound | More than 512 unique admitted candidates refuses the catalogue instead of silently truncating it. |
| Rollback snapshot | Positive dimensions up to 32768; may be smaller than new-candidate bounds, so an existing smaller viewport is not lost. |

The native owner requires explicit `bEnableDisplaySettings=true` on the existing UI, one local player in a Windows standalone `Game` world, a fixed live viewport/controller, and the standard `UGameUserSettings` class. It refuses editor/PIE, a custom settings subclass and a present XR system rather than guessing their side effects. This is a bounded initial integration, not a universal graphics framework. An unavailable optional display owner does not poison the campaign or disable unrelated settings.

There is no monitor picker, refresh-rate control, window-position restoration, HDR setting, render-scale slider, dynamic-resolution control, VSync/frame-cap UI, hardware benchmark or quality preset. Enumeration is not a certificate that a mode works on every monitor. Primary/secondary monitor and mixed-DPI behavior must be observed locally. A borderless request normalized to different dimensions cannot be kept by this model. The engine may move/normalize a window as part of a mode change; this source neither explicitly selects a monitor nor promises to restore its previous position.

## 4. Source ownership

| File under the plugin module | Responsibility |
|---|---|
| `Public/Core/DisplaySettingsRules.h` | Shared catalogue bounds, exact trial/restore states, ticket/context rules, real-time deadline and later-frame Keep policy. |
| `Public/CoastalDisplaySettings.h` | Internal UI-owned native adapter; not a second player-preference manager or attachable gameplay component. |
| `Private/CoastalDisplaySettings.cpp` | Fixed host/viewport admission, real native catalogue, actual viewport observation, descriptive status. |
| `Private/CoastalDisplayTrial.cpp` | Deferred native request, polling, explicit keep/save request, cancellation and teardown. |
| `Private/CoastalUIDisplay.cpp` | Existing stack integration, draft commands, early watchdog tick, post-match presentation reset. |
| `Private/CoastalUIDisplayPresentation.cpp` | Display/confirmation text and command availability, compact countdown/status. |
| Existing native UI/panel files | Two appended panel kinds, menu entry, safe cancel exception, close/recovery/session/teardown hooks. |
| `Private/Tests/M1DisplayAutomationTests.cpp` | Four additional supplied, unrun engine tests; none changes a real display. |

The original bootstrap signature and order remain. The UI creates its internal display object after its existing widgets/audio initialization. No new host bootstrap call, input action asset or actor component is required. No engine-module dependency is added. Existing `Engine`, `UMG`, `Slate`, `SlateCore`, `InputCore` and `EnhancedInput` dependencies remain.

## 5. Request, observe, confirm

`FSystemResolution::RequestResolutionChange` submits a deferred engine resolution request through `r.setres`. Epic describes the change as occurring later when console variable sinks execute. This is why the source separately observes `FViewport::GetSizeXY()` and `GetWindowMode()` and never treats the request's void return as success. [S1, S2]

`DisplayTrial` has Idle, Testing, Reverting and Failed phases. Testing is bound to a fresh ConfirmDisplay ticket/epoch. A native mode match establishes a presentation revision; the UI resets the old paint gate and scrolls to the body. A later body paint and later command are required. Losing the match invalidates settlement, so returning to the target requires a new revision. The compact confirmation uses display-only status instead of burying the timer beneath historical campaign notices.

The actual pretrial viewport is the rollback target, not a possibly stale last-confirmed disk setting. For example: keep B for the session after starting in A, test C, then Revert returns to B. A previously supported small viewport can also be restored even though it is not offered as a new candidate.

Revert submits one restoration request and blocks further trials until a later-frame observation sees the original size/mode. The request itself and an immediate same-frame observation are not restoration success. If restoration is not observed within five running real-time seconds, further trials are locked for this owner lifetime. There is no infinite retry or automatic fallback to a guessed resolution.

This is still viewport observation, not an operating-system or monitor acknowledgement. Real asynchronous mode switching, sinks, DPI, focus, driver behavior and rendering must be tested in the actual engine/Windows host.

## 6. Cancellation and ownership loss

Explicit Revert/Back, timeout, observed foreground loss, lost top ticket, changed epoch, recovery/return, invalid/regressing clocks/frames and captured-binding loss stop confirmation. The watchdog executes before the UI's save-coordinator-busy early return. ClearPanels, save/session notices, safe return and UI teardown also cancel. These hooks submit display cleanup only; they do not call campaign mutation, change inventory, release another owner's gameplay lock or acknowledge the radio.

Before trial/Keep, the adapter captures and compares the standard engine settings object's resolution/window-mode fields. A distinct competing edit cancels and preserves those stored fields; after restoration the source locks further trials. A replaced viewport/controller/settings object is not silently rebound. A best-effort restore is only submitted while the captured binding is still valid; a replacement viewport is never taken over to hide the error.

This detects neither every same-valued/temporary writer nor all other settings fields or cross-process changes. Use one deliberate display owner, disable competing demo graphics menus/Alt-Enter persistence handlers where necessary, and test the actual host. The native engine itself can normalize requests. A transient focus loss during switching may conservatively cancel the trial; record that behavior rather than weakening safeguards blindly.

Release is idempotent and attempts cleanup for an active trial. If the UI stops ticking or the process freezes/crashes, no unexecuted code can run a timeout. Shutdown records that restoration was not observed when it cannot poll afterward. There is no external watchdog, crash-atomic video journal or promise that every driver failure can be repaired from the game thread.

## 7. Explicit persistence, separate from Coastal slots

A trial does not stage resolution/window-mode values in `UGameUserSettings`, call `ApplySettings`, or explicitly save. It requests the runtime change directly. Session-only Keep retires the trial without editing the engine settings object, so the next trial must again read the actual viewport.

Only the explicitly labelled engine-save command calls the standard resolution/window-mode setters, `ConfirmVideoMode()` and `SaveSettings()`, after the full Keep recheck. Epic documents `ApplySettings` as also saving and `SaveSettings` as a void operation; the source avoids applying all settings during a trial and cannot infer verified IO from SaveSettings. [S3]

The resulting notice says **save requested; disk persistence is not verified**. A failed write does not roll the kept runtime mode back or falsely report a verified campaign save. The standard engine writer serializes its existing user settings, not an isolated two-field custom file; existing non-display values may be serialized too. The standard resolution setter may update derived resolution-quality bookkeeping. No quality control or deliberate quality-application operation is added here. A custom settings subclass is excluded until intentionally integrated.

No load, defaults reset, file deletion, `ValidateSettings`, `RevertVideoMode` or save-file path guess is used by this adapter. Host-provided auto-save/config callbacks remain outside its guarantees and must be inspected locally.

Campaign schema and Coastal player preferences are untouched: schemas 1/2/3 still read; explicit player-preference writes remain schema 3 in their existing two slots. No schema 4 or new Coastal display slot is introduced. This is not proof an older Unreal binary campaign loads with a rebuilt engine/plugin.

## 8. Verification boundary and local gate

The new standalone suite exercises the exact shared catalogue/trial implementation, alongside every retained suite. It includes no fake Unreal viewport, vendor provider or fabricated display screenshot. Native declaration/API correctness still needs UHT/UBT and the actual host compiler. The four new engine tests test unbound rejection/shared rules without changing developer screens or profile files.

See [validation](../VALIDATION_REPORT.md), [wiring](M1_10_DISPLAY_WIRING.md) and the [38-case actual-host ledger](../data/m1_10_display_acceptance.json). The 38 supplied native automation tests across the complete plugin and the 38 new manual display scenarios are separate inventories, both unrun here. Standalone assertions are not playthrough counts.

Only the actual launched Windows test-room campaign loop, including faults and vendor integration, passes M1. M2 coast assembly remains after that gate.

## References and boundaries

Product milestones derive from the supplied foundation and M1.9 package. All new admission bounds, timeouts, control policies and implementation structure are original decisions. Official Epic API documentation checked September 6, 2026 supports entry-point meaning, not compilation or compatibility certification. Current pages may display UE 5.8; no project engine version is pinned by this work.

- [S1 — FSystemResolution](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FSystemResolution): deferred resolution request via console-variable sinks.
- [S2 — FViewport](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FViewport): actual viewport dimensions/window mode, foreground and fullscreen support queries.
- [S3 — UGameUserSettings](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameUserSettings): resolution/mode fields, confirmation, ApplySettings and void SaveSettings.
- [S4 — UKismetSystemLibrary](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UKismetSystemLibrary): supported fullscreen and convenient windowed enumeration.
