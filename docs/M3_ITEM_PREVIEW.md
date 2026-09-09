# M3 item preview and rotation

## Current scope

The `Pickupable&RotatableItems.zip` archive was inspected in private staging:
257 files, 136,017,318 expanded bytes, with a UE5.6 project declaration. Its
first-person Enhanced Input persistent actor-move demonstration is not safe to
transplant into the existing coastal host. The archive remains private and its
Blueprint gameplay is not treated as the implementation.

The adaptation uses the existing inventory detail flow and two original
provisional meshes that match the existing battery and fuse icons:

| Item | Preview mesh | Cook path |
|---|---|---|
| `item.radio_battery` | `SM_RadioBatteryPreview` | `/Game/Coastal/M3/ItemPreview` |
| `item.marine_fuse` | `SM_MarineFusePreview` | `/Game/Coastal/M3/ItemPreview` |

The authoring script creates two models and 12 unlit materials in the private
UE5.8.2 host. No paid mesh from the archive is used. The cook rule explicitly
includes `/Game/Coastal/M3/ItemPreview`.

## User flow and controls

Select a carried item in the existing inventory and choose **Inspect**. The
existing Item Details panel presents the current name, description, quantity,
size, and a transient render-target preview when a mapped mesh exists.

While the preview is open:

- `A`/`D` rotate left/right by 15 degrees.
- `W`/`S` tilt up/down by 15 degrees.
- The gamepad right stick rotates continuously while its value exceeds the
  dead zone.
- The panel buttons provide Rotate left, Rotate right, Tilt up, and Tilt down.
- Back, pause, recovery, campaign reset, and player replacement close the
  preview and release its transient capture resources.

The preview does not add a free-floating world rotation action. World pickups
continue through the existing Hyper interaction path.

## Ownership and safety rules

Before opening, the UI rereads the existing AGIS container view and records the
selected item’s fresh instance GUID, item ID, container, and view revision.
Every rotation validates that same GUID and revision again. A stale item or
revision closes or refuses the preview, so inspection cannot mutate inventory
or act on a stale row index.

`UCoastalItemPreviewComponent` remains the owner API on the local
PlayerController. A separate transient collision-free actor now owns the
preview rendering primitives for their lifetime: a movable static mesh, a
384x384 render target, and a scene capture at a hidden preview location. The
preview mesh has no collision, physics, overlap, or gameplay camera effect. Its
yaw wraps and pitch clamps to -70 through +70 degrees. The source mesh and
world pickup transform are never changed. The transient actor is destroyed on
close; it is not a gameplay actor or a second inventory authority.

The first two disposable PIE runs failed overall because the capture was flat
white, even though eight pickup and authority assertions completed. The
captured reports are `../local-evidence/m3-item-preview-flat-capture-failed.json`
and `../local-evidence/m3-item-preview-register-reset-failed.json`. The root
cause was the hidden PlayerController and `USceneCaptureComponent::OnRegister`
resetting direct show-flag edits. A controlled probe now captures the real
battery while the PlayerController remains hidden, but
`../local-evidence/m3-item-preview-registered-flags.png` is probe evidence only.
The persisted show-flag/fixed-exposure source rebuild passed, with log and
execution evidence under `../local-evidence/m3-preview-persisted-flags-build.*`
and source SHA matching the active host. The latest `0909c` run reached 14
assertions and captured 22 distinct pixel colors before the harness hit a
Python API mismatch (`get_static_mesh`, corrected to `static_mesh`). Its image
was an overexposed fallback, so visual acceptance did not pass. The source now
refreshes the capture continuously while Item Details remains open, including
while paused, with fixed bias 0 and bloom 0. The final `j` result below
supersedes this earlier partial run; the native refresh build passed.

The existing Item Details panel owns input while open. The original UI and
inventory authorities remain in place; no second inventory, pickup, save, or
preference schema is introduced. Campaign epoch changes invalidate the preview
and restore normal gameplay input.

Runs `g` and `h` showed that a legitimate fuse metal end-on view can dominate
sampled pixels, so the harness now rotates the battery 45 degrees and fuse 90
degrees before palette sampling. Fresh `i` is running. Actual battery imagery
and all battery controls, inventory, and cleanup checks pass through the latest
runs, but no overall preview gameplay or visual acceptance is claimed. Default
fuse orientation art polish remains open.

## Final live preview result

The final `j` run passed 53 assertions in `coastal_test_m3_preview_0909j` in
16.375 seconds. Battery and fuse captures both showed colored 3D imagery;
button, Slate keyboard, and gamepad rotation passed; stale GUID/revision
refusal, unchanged AGIS inventory, stable camera, cleanup, recovery, and
save/reload passed; and pre-existing saves were preserved. This supersedes the
earlier partial `d` through `i` attempts. Default fuse orientation art polish
remains open; physical-device, performance, and Windows-package acceptance are
separate.

## Source and verification boundary

The core pose rules are in `Core/ItemPreviewRules.h`. The transient owner is
`UCoastalItemPreviewComponent`; inventory/detail binding is in
`CoastalUIItemPreview.cpp`, and keyboard/gamepad routing is in
`CoastalPanelInput.cpp`. The private authoring/import and live scripts are
`tools/unreal/import_m3_item_previews.py` and
`tools/unreal/check_m3_item_preview.py`.

The checker uses a fresh disposable campaign and is designed to verify real
AGIS battery/fuse pickups, duplicate-pickup refusal, preview opening, button,
keyboard, and gamepad rotation, stale GUID/revision rejection, unchanged
camera/player/world transforms, capture cleanup, save/reload epoch cleanup,
unchanged inventory, and pre-existing save preservation.

The combined merchant and item-preview native build passed, and 59 native tests
pass (58 clean plus one known AGIS warning). The report is
`../local-evidence/m3-ue58-native-report-20260909T081913Z`. Scripted PIE failed
twice on the white capture, with the eight pickup/authority assertions noted
above. The persisted-flags fix has built successfully, and the latest `0909c`
run is partial evidence only because the harness failed after capture and the
image used an overexposed fallback. The acceptance path now rejects white
captures and waits for the actual teal palette. Physical device behavior,
visual/art acceptance, performance, and Windows package acceptance remain
open. The earlier build evidence is
`../local-evidence/m3-merchant-preview-build.execution.json`; its initial helper
failure remains under `../local-evidence/m3-merchant-preview-build-nan-failed.*`.
