# M1.6 — Player Options, Camera Look and Readability

**Project:** Coastal Exploration / First Signal  
**Section date:** September 5, 2026, America/New_York  
**Version:** `0.1.6-m1-player-options-source`  
**Status:** Original source implementation with standalone verification; no native Unreal build or Windows executable.

## 1. Scope and source basis

The supplied foundation specifies adjustable camera sensitivity, invert-Y, readable text and settings; prior increments explicitly left production settings unfinished. The supplied M1.5 package still reports its real engine/vendor/Windows gate as not run. This increment deliberately advances a bounded part of the foundation's M3 presentation/input work for that existing test room. It does not redefine M1 as passed or claim M2 coast assembly.

The delivered source is based on the actual supplied M1.5 ZIP. AGIS remains the sole inventory authority, Hyper remains target/prompt owner, the native UI remains modal/input-mode owner, and the host remains movement and input-mapping owner. No owned asset packages, host project or engine installation was supplied. No vendor symbols or integration success are invented.

## 2. Implemented behavior

| Setting | Default | Allowed values | Runtime consumer |
|---|---:|---|---|
| Mouse look | 100% | 25–300%, step 25 | Optional character look consumer; actual mapped mouse delta. |
| Gamepad look | 100% | 25–300%, step 25 | Same consumer; actual mapped, dead-zoned normalized stick axes. |
| Camera FOV | 85° | 70–110°, step 5 | One active perspective camera component on the fixed character. |
| Text size | 100% | 100–150%, step 10 | Native Coastal HUD, modal text, buttons, notices, help and save-name field. |
| Invert look Y | Off | Off/On | Pitch multiplier only; yaw unaffected. |

These are original starting settings, not calibrated hardware measurements or accessibility certification. Text size does not modify Hyper prompts or purchased widgets. No audio sliders are shown without actual sound-class routing. Resolution/quality, remapping, controller glyphs, sprint hold/toggle, narration, screen-reader support and localization are not added here.

The new settings screen is a child of Session or Pause. It inherits the existing top-ticket, frame, epoch, parent-focus and recovery guards. The five settings are selected with Previous/Next; Decrease/Increase or Toggle adjusts the selected field. Keyboard, mouse and the existing D-pad/button navigation can operate these commands in source; actual controller usability still needs the local test.

Changes are drafts. Text previews on the options panel itself, while FOV/sensitivity/inversion wait for explicit apply. Back discards only unapplied edits. Defaults changes the available settings in the draft; it does not erase any file. Applying to the session changes live options without writing. Applying and saving changes live values only after exact preference read-back succeeds. A failed write leaves the former live values intact, retains the draft for review/session-only apply, and reports that persistence was not verified.

## 3. Source ownership

| Source | Responsibility |
|---|---|
| `Core/PlayerOptionsRules.h` | Bounded integer settings, adjustments, font sizes, mouse/stick math and input rearming. |
| `Core/OptionsPersistence.h` | Fixed preference encoding/decoding and safe slot selection. |
| `Core/OptionsSession.h` | Exact synchronous preference lifecycle; explicit apply, observed-file checks, alternating verified writes and failure lock. |
| `CoastalLocalOptions` | Thin UObject/platform IO wrapper, owned by the single native UI component. |
| `CoastalUIOptions.cpp` | Draft/presentation/commands, available-consumer checks and explicit application. |
| `CoastalLookInputComponent` | Optional character camera/look consumer; no mapping context or target discovery. |
| Existing panel/HUD source | Text scaling, whole-panel scroll layout, focus scrolling and bounded HUD width. |
| Existing `Core/UIFlowRules.h` | Appended Settings panel kind and visible body/scroller intersection helper. |

The bootstrap call and order remain unchanged. Inside `InitializeUI`, after menu input attaches, the UI creates/loads its local options object before creating widgets. It discovers zero or one optional look component on the bound character. That component is initialized here, not by an additional host/bootstrap call.

No component means text-only settings with disabled camera/look controls. A duplicate or incompatible consumer yields an options diagnostic instead of poisoning an otherwise healthy campaign. Setting `bUseHostLookEvents` is an explicit host declaration, not proof that action callbacks are wired. The property defaults false; its value is captured at initialization. Changing it during play does not hot-swap input ownership.

## 4. Camera and input contract

Camera support is deliberately narrow: the fixed possessed local ACharacter is also the view target, camera-component view discovery is enabled, and exactly one registered camera component is active and perspective. The component samples that player's small component list when checking readiness; it never scans the world for an interaction target. Custom camera managers, cine-camera lens overrides, camera blends and camera replacement are not certified by these checks.

FOV is applied at initialization and explicit option application through the camera component. It is not written every tick, so the source does not deliberately fight another camera system. Teardown restores the initial FOV only while the camera's current FOV still equals this owner's last applied value. This is a limited ownership safeguard, not a universal camera-effects compositor.

The host forwards mouse and stick separately. Mouse input is a mapped delta and gets a percentage multiplier; it is not multiplied by frame time. Stick input is normalized, already dead-zoned by the host, clamped to [-1, 1], then multiplied by 90 input units/second at 100% and by game delta time capped at 0.1 seconds. The cap intentionally limits a long-hitch contribution, so sub-10-FPS behavior is not identical to unbounded continuous integration. No second dead zone, acceleration curve, smoothing or forced cinematic motion is introduced.

These outputs go through the character's normal AddControllerYawInput/AddControllerPitchInput methods. Existing host/legacy axis scales can still multiply them; record or remove intentional upstream scaling rather than claiming these are universally calibrated degrees. Preserve the intended host pitch sign before applying the optional inversion once. Do not send a stick value that the host already converted to a time-scaled rate.

Each device may submit once per engine frame. Both devices may contribute in the same frame. World input must be permitted by the original bridge and controller look must not be ignored. Recovery, menus, new/load epochs and permission revisions invalidate the input gate. Mouse cannot apply on the enabling frame. Stick additionally needs an actual neutral sample on a later eligible frame before a later non-neutral sample can rotate. A real neutral logical axis has both absolute components at most 0.0001; do not invent a neutral sample simply because a UI closed or an action was canceled.

The host must continue supplying actual stick state/neutral after transitions. The source neither polls a guessed vendor action nor installs another mapping. Leaving an original direct look callback active alongside SubmitMouseLook/SubmitStickLook doubles input; the plugin cannot stop unrelated Blueprint rotation code.

## 5. Local preference persistence

Preference slots are `CoastalLocalOptions_v1_A` and `CoastalLocalOptions_v1_B`, user index 0. Their names do not match the campaign catalogue's `Coastal_<set>_A/B` convention. They do not contain campaign IDs, maps, transforms, inventory, mission facts, world records or transaction receipts. New/Continue does not reload the local preference object. Session-only values therefore survive campaign switches in that UI lifetime but not a relaunch.

A payload has an eight-byte options identifier, schema 1, unsigned 64-bit generation and five unsigned 32-bit fields. It is exactly 40 bytes, wrapped in the retained 20-byte CRC32 envelope: 60 encoded bytes. Integer percentages avoid serialized NaN/infinity values. The decoder checks the options identifier, version, exact size, generation, ranges, steps and Boolean encoding before returning a valid record. Oversized data is rejected by the decoder, although the platform raw-read API has already allocated its returned buffer. This is accidental-corruption handling, not a hostile-file/security boundary.

On initialization both slots are read; there is no automatic write. Two missing slots mean defaults plus permission for an explicit first save. With unambiguous valid generations the highest is selected. One corrupted slot plus one valid slot produces an explicit recovery notice. Equal valid generations, unsupported format, a reported read error, or existing files without a valid record do not authorize automatic replacement. Defaults and session-only changes remain usable while disk writes are blocked. A maximum-generation valid record can load but cannot increment/write.

Before a write, both observed files/statuses are reread and compared. A change blocks writing and asks for relaunch. Otherwise only the unselected slot is written; exact bytes and decoded values/generation are read back. Only confirmed success changes the live options and advances the local selection. Failed or ambiguous writes lock further preference writes until a new session re-reads disk state. The other slot is never deliberately modified by that operation. A write that reported failure may still have reached disk, so relaunch may legitimately load the proposed values.

The shared `OptionsSession` class contains that actual logic. Standalone tests exercise it through explicit in-memory read/write fault callbacks; those fixtures are not represented as real Unreal or Windows IO. There is no operating-system lock, atomic compare-and-swap, encryption or crash-safe storage guarantee. Another process can race after the pre-write check. One editor/game instance per profile is the supported test setup. Do not fault-test real player saves without backups.

## 6. Readability/layout changes

All modal content—including headings, the save-name field, body, notices, command buttons and help—is inside the panel's scroll box. Previously only the body scrolled, while a long list of commands consumed fixed vertical space. Native font sizes are scaled from their original bases, not compounded on every refresh. The editable field uses its actual widget style API; the implementation does not assume it has UTextBlock's SetFont method.

Focused commands can scroll into view using existing D-pad/keyboard navigation. New panels start at the top so long journal/transcript content is not immediately skipped by an automatic command scroll. Parent focus is restored on Back. Text preview refreshes preserve a valid command focus and the existing one-command-per-frame rule.

The presentation gate now checks positive body/scroller layout intersection before recording a display opportunity. This prevents an entirely offscreen body from satisfying the new layout's simple geometry check. Cached layout geometry can lag, and a paint/layout opportunity still does not prove the player read all text. The transcript's separate later explicit acknowledgement and token/session checks remain required. Real rendered tests must inspect this behavior after layout, scrolling and input changes.

The HUD uses content height instead of a fixed 300-unit panel, adjusts its width within the current viewport, scales its native labels, and hides its frame beneath modal UI. Long diagnostics are also available in the fully scrolling modal notice area. This is development presentation, not final art or a measured 720p/1080p accessibility pass.

## 7. Preservation and remaining gates

Campaign schema, receipts, item/world IDs, First Signal story, atomic inventory boundary, save coordinator and return/checkpoint source are retained. No changes to purchased packages or host input assets are included. This does not prove that an old binary campaign save loads successfully with the current engine.

Read the current [validation report](../VALIDATION_REPORT.md), [local wiring](M1_6_OPTIONS_WIRING.md), and [34-case acceptance ledger](../data/m1_6_options_acceptance.json) alongside all retained tests. The 22 supplied Unreal tests are not run here. The actual host still needs UHT/UBT, real AGIS/Hyper wiring, rendered/controller tests, platform IO fault tests and the full packaged campaign loop. Only then does M2 assemble Cabin Cove, Shoreline Trail, Old Dock and optional Rail Overlook. Options source is not that gate.

## References and boundaries

The supplied foundation and M1.5 documentation establish the requested game and remaining presentation work. All new settings policy, file format, source structure and tuning above are original project implementation decisions. Official Epic documentation was consulted to check API meaning, not to certify this source or change the locally selected engine:

- [Camera component API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UCameraComponent?lang=en-US): perspective camera state and SetFieldOfView.
- [UGameplayStatics](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameplayStatics?lang=en-US): raw save-slot IO.
- [Saving/loading](https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine?lang=en-US): synchronous versus asynchronous work.
- [UScrollBox](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/UScrollBox?lang=en-US): scrolling and focused-widget visibility.
- [UEditableTextBox](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/UEditableTextBox?lang=en-US) and [FEditableTextBoxStyle::SetFont](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/SlateCore/FEditableTextBoxStyle/SetFont?lang=en-US): editable-field style changes.
- [USizeBox](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/USizeBox?lang=en-US): content width handling.
- [FGeometry](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/SlateCore/FGeometry?lang=en-US): layout rectangles.
- [Add Controller Yaw Input](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Pawn/Input/AddControllerYawInput?lang=en-US) and [Is Look Input Ignored](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/Input/IsLookInputIgnored?lang=en-US): normal controller input and its existing scales/locks.

Pages displayed UE 5.8 when checked. This is not a pinned or certified engine version for the project.
