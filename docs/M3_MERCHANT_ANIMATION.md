# M3 merchant animation

## Current scope

The merchant increment is a presentation-only integration in the independent
UE5.8.2 host. It adds one map-owned merchant presenter near the village
register, using the readable UE5 FBX alternative from
`MerchentVendorAnimFBXFiles.zip`:

| Asset | Source member | Host asset |
|---|---|---|
| Manny skeletal mesh | `MerchentVendorAnimFBXFiles/UE5/SKM_Manny.FBX` | `/Game/Coastal/M3/Merchant/UE5/SKM_Manny` |
| Idle bartering clip | `MerchentVendorAnimFBXFiles/UE5/Animation/AS_IdleBartering.fbx` | `/Game/Coastal/M3/Merchant/UE5/Animation/AS_IdleBartering` |
| Offer pitch clip | `MerchentVendorAnimFBXFiles/UE5/Animation/AS_PitchBarter_Rt.fbx` | `/Game/Coastal/M3/Merchant/UE5/Animation/AS_PitchBarter_Rt` |

The archive contains 54 FBX entries: 26 UE4 animations, 26 UE5 animations,
and one skeletal mesh in each engine folder. The selected UE5 mesh and both
clips share `/Game/Coastal/M3/Merchant/UE5/SKM_Manny_Skeleton`. The imported
idle clip is about 7.2 seconds and the pitch clip about 6.3 seconds. The
native `MerchantVendorAnim-V1.1.zip` remains encrypted and is not imported.

The readable archive has no license or README entry. Keep purchase provenance
outside Git and resolve distribution credits before release.

## Runtime behavior

`ACoastalMerchantPresenter` is map-owned and presentation-only. The village
register and journal remain authoritative. It binds to the local player,
existing interaction bridge, save coordinator, and UI session owner. A valid
campaign wakes the presenter in idle; an invalid binding, recovery, player
replacement, campaign epoch change, or non-standalone net mode hides or resets
it.

The presenter responds only to the existing
`journal.coastal_records.village` request. It waits for that journal modal to
have appeared and then closed, queues one non-looping pitch, and returns to the
looping idle clip when the pitch ends. An unrelated destination record leaves
the merchant idle. Pause, an unexpected modal, recovery, return-to-player,
epoch change, or the bounded queue timeout cancels the pending presentation.
No shop, trade, crafting, inventory, save, or preference authority is added.

The authored proposal is near `[-2400, 15900]`; the village register is at
`[-1455, 15300, 2305]`, with arrival near `[-1600, 15300, 2200]`. Final
placement uses the downward Visibility trace recorded during authoring.

## Source ownership

The state machine is in `Core/MerchantPresentationRules.h`; the Unreal actor is
`ACoastalMerchantPresenter` in `CoastalExpansion58`. The state sequence is
`Dormant -> Idle -> AwaitingJournalClose -> Pitching -> Idle`. The focused
native rule test is `tests/merchant_presentation_tests.cpp`; Unreal automation
coverage is in `CoastalMerchantAutomationTests.cpp`.

Stage the three selected FBXs with
`tools/stage_m3_merchant_animation.py`. Import them with
`tools/unreal/import_m3_merchant_animation.py` only into the identity-checked
UE5.8.2 host. Both scripts refuse changed private staging, wrong project
identity, PIE, dirty packages, or an existing destination directory.

## Verification boundary

Private host import, shared-skeleton binding, idle duration, and pitch duration
are recorded in `data/m3_merchant_animation.json`. The combined merchant and
item-preview native build passed; native tests are still running and scripted
PIE, physical-device behavior, listening/art review, and Windows package
acceptance remain `not_run`. Build evidence is
`../local-evidence/m3-merchant-preview-build.execution.json`; the initial
helper failure is retained under `../local-evidence/m3-merchant-preview-build-nan-failed.*`.
The live checker is `tools/unreal/check_m3_merchant.py`; it uses a fresh
`coastal_test_` campaign and checks unrelated journal idle behavior, the village
journal gate, pitch completion, idle restoration, pause cancellation, reload,
epoch cleanup, and preservation of pre-existing saves.
