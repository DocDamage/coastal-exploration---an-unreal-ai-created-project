# Installed vendor contract: bounded M1 integration

These notes describe inspected installed assets and original wrapper code. Raw vendor Blueprint graph dumps, purchased assets and generated binaries are private local dependencies.

## Provenance and mounts

- AGIS supplied project: `AdvancedGridInventorySyst`, descriptor association 5.8. Its inspected core Blueprints compile/load and run in the 5.7.4 host. Inventory__Main asset SHA-256: `f49886d5416f793ccc396b418ec086cf37959555ddbc44f4d319d3ca83a8249c`.
- Hyper supplied archive: `InteractionSystem_5.7.zip`; SHA-256 `5a7f27fe0249371e451d77fd40b0263399d39045d3bb1bc3e67e419135c00d12`. Paths and archive entries were validated before extraction. AC_CH_Interact_Base asset SHA-256: `3ac827cb46f8e334105b6714ead807216d4319dd1dc77b71431a3acb822e2b49`.
- Copy owned content into the external host at exact mounts `Content/INVENTORY` and `Content/Hyper`. Preserve original downloads. Enable CommonUI; use `GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient` under `[/Script/Engine.Engine]`.
- Merge the required vendor gameplay tag declarations, including separate `Config/Tags` files, without overlaying unrelated project configuration. The inspected AGIS demo includes missing weapon attachment tags; warnings remain recorded.
- Add trace channel `ECC_GameTraceChannel1`, name `Interaction`, trace type, default Ignore. Original world-wrapper ProxyMesh blocks this channel. Hyper uses TraceTypeQuery3 for selection.

## AGIS API and authority

Live owners are two actual instances of `/Game/INVENTORY/Core/InventoryComponents/Inventory__Main.Inventory__Main_C`. Auto Init Component is disabled before registration; demo replication and tick are disabled. No GameInstance_AGIS initialization or SaveWorld/LoadWorld is called.

The wrapper resolves exact reflected signatures and fails on missing/ambiguous fields. UserDefinedStruct hashed names are matched by unique reflected prefix. Inspected vendor calls:

| Purpose | Actual call |
|---|---|
| Definitions | FL_AGIS: Get Item Defaults By ID |
| Grid creation | FL_AGIS: Create Slots For Container |
| Add container | Inventory__Main: Add Containerr (installed spelling) |
| Refresh caches | Refresh Containers |
| Check capacity | Find Space For Item Stack |
| Place into staged grid | Set Item on Container By Address |

The vendor Add Item graph can partially add. The adapter instead reconstructs a proposed transaction in temporary real AGIS components, reads the vendor's actual stored grids and validates item amounts, UID/address, rotation and occupied cells. Only after all failing work has succeeded does it copy Containers, Containers_Dirty, Cached ItemMap and Cached ContainerMap into the two stable live components, then commit campaign/receipts/revision. This synchronous native copy has no Blueprint callbacks and is covered by the operation guard.

There is no parallel authoritative item array. Temporary records represent an export or a proposed transaction. Every inventory view/export/mutation reads real AGIS container arrays. The battery, fuse and existing catalogue postcard definitions are added through a transient parent UDataTable of the installed CDT_Items, using its real row structure. The adapter removes that parent on shutdown and does not save vendor packages.

The bounded codec supports battery UID 1, fuse UID 2 and distinct nonstackable postcard instances UID 3 through 74. All postcards share `item.old_postcard`; their reserved world IDs are `world.test.postcard.001` through `.072`, separate from the existing `world.test.postcard` journal discovery. The fixed grids remain 6x4 backpack and 8x6 cabin. A separate strict capacity map contains these pickups. Packaged full-backpack and full-cabin refusal, world-state conservation, save/relaunch and recovery through UI transfers pass. The default seven-object room stays unchanged in scope.

Two-part snapshots remain byte-compatible with `agis-blueprint-20260906.codec1`. The first postcard receipt selects `agis-blueprint-20260906.codec2`, retaining the same four-byte UID/container/slot/rotation records with a bounded 72-item maximum. A codec-2 snapshot must contain a postcard receipt. Every collected postcard must still exist exactly once, and radio repair consumes only UIDs 1 and 2. No read automatically rewrites a disk save. Provider ID remains `coastal.agis.m1`. Instance GUIDs derive from the entire campaign GUID and instance UID. Both codecs reject arbitrary item data, owned subcontainers, unknown rows, duplicate/lost instances, overlapping placements and trailing bytes. Transfers require matching pickup receipts. Future or foreign versions return Incompatible; malformed known formats fail validation. This remains a bounded adapter, not a general-purpose AGIS serializer.

The real AGIS capacity automation fills the backpack with 24 distinct postcards and the cabin with 48 through normal collect/transfer methods. It verifies native NoSpace results, unchanged payload/receipts, successful retry after freeing enough cells, postcard-preserving repair, and codec-1/codec-2 restore. Tests yield between transaction batches to retain the normal Blueprint runaway-loop guard. Reflection/signature failures are reported as Failed, never NoSpace.

Hyper's inspected `Trace From Active Camera` adds requested spring-arm length to its trace-start offset. Near a wall, the camera can retract while requested length stays unchanged, placing the start beyond a close pickup. The host now compensates the exposed front-offset input using the actual camera-to-boom-pivot projection before calling the same vendor trace. No vendor graph is edited and no second target-selection trace is installed. The capacity pickup beside the wall, all 24 pickups, and the normal packaged campaign pass with this correction. Development-only waiting diagnostics perform read-only comparison traces; they never supply a target or authorize interaction.

## Hyper and host lifecycle

BP_CoastalHyperWorldObject derives from the native stable-ID proxy and implements the installed BPI_CanInteract and BPI_Interact. Its Can Interact result is Coastal offer visibility, keeping disabled offers discoverable. Native interaction still checks the bridge's range, occlusion, campaign and provider conditions before dispatch. Hyper remains the sole focus trace and prompt renderer.

BP_CoastalHyperFocus derives from the actual AC_CH_Interact_Base on the character. Its BeginPlay suppresses inherited demo mapping/timers. UCoastalHostSession explicitly calls Can Interact Trace and reads Able to interact with this object every permitted world-input frame. It cancels vendor focus while a modal blocks the world and forwards the actual target to the existing relay. The installed WBP_CanInteractButton receives current native offer text. Its default fixed A glyph is hidden; the prompt labels the relay's E / gamepad X bindings.

BP_CoastalPlayerController derives from the actual Third Person controller and adds one host-session component. Possession and next-tick readiness lead to one StartTestRoom call on the existing bootstrap. The existing coordinator, UI, recovery and mission owners remain in charge. Pawn replacement requires relaunch rather than constructing a second session.

## Rebuilding original wrappers

Original, editable editor helper source is in `tools/unreal/host_editor/CoastalHostEditor`. Copy that module into the external host Source directory, add an Editor module entry to the descriptor and its name to the Editor target's ExtraModuleNames. It depends on both original Coastal plugins and the normal Unreal editor modules. Enable PythonScriptPlugin and EditorScriptingUtilities for editor targets.

Build the Editor target, then run `tools/unreal/build_integration_assets.py` inside the full editor to create the three original wrapper Blueprints. Run `tools/unreal/build_integrated_room.py` to create the original child GameMode and audited room. Both must run with a real rendering backend and no unsaved editor work. Existing assets/maps are preserved; generation refuses overwrites. On partial failure, inspect the assets and log instead of deleting evidence and blindly retrying.

The helper's optional DumpBlueprint writes reflected vendor graph details only to the caller's explicitly selected local output. Never stage or publish those dumps. The helper source itself is original code; no vendor graph bodies are embedded.
