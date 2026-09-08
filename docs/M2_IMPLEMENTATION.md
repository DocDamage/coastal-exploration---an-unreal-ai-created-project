# M2 — First Signal coastal opening

Implementation is complete in `F:/coastline`, with the Windows development package built. The user deferred gameplay and physical-device acceptance; those checks must not be reported as passed.

## Scope and ownership

`data/m2_opening.json` defines the persistent opening map, four connected areas, route and logical identities. `tools/unreal/m2_geometry.py` authors the original terrain and radio geometry. `tools/unreal/build_first_signal.py` assembles the map in the licensed local Unreal host, using the inspected Nordic Fishing Hut interior, furnishings, rocks and trees. The source recipe refuses to overwrite an existing map.

The seven existing native world identities remain bound to real AGIS receipts and Hyper focus. The opening uses map identity `level.first_signal` and default campaign name `first_signal_01`; the systems room remains separate. No campaign schema migration or alternative inventory is introduced. Supply containers grant one required part through the existing transaction boundary and remain visible after collection. A refused insertion leaves the source available.

The cabin contains the radio, maintenance note and storage. Two distinct dock containers hold the battery and fuse without prerequisite locks. The optional overlook holds the existing North Reach discovery. Existing mission and journal logic allows either part to be collected before inspecting the radio.

## Authored layout

The winding main route measures approximately 196 metres one way. The optional branch measures 66 metres one way; including return travel, the authored centerlines total 525 metres. These are geometric design measurements, not a timed player session. Terrain has broad destination clearings and a 3.6 metre target path width. Water and outer edges use existing native recovery volumes; no swimming or travel system is added.

The original radio mesh is intentional project art. Nordic Fishing Hut assets remain in `F:/coastline/LocalVendor/NordicHut`, linked into the local host. They are excluded from the source repository. The mannequin remains temporary character art; final presentation, ambient polish and performance work belong to M3.

## Implementation evidence and deferred acceptance

The actual owned cabin prefab inspection completed successfully (`local-evidence/m2-inspect-assets-fdrive.execution.json`, exit 0); its component placements and material references are in `m2-assets.json`. This proves asset loading and supplies construction data, not cabin gameplay compatibility.

The native `TP_ThirdPersonEditor Win64 Development` build succeeded under Unreal 5.7.4 after the workspace move (`local-evidence/m2-editor-build.execution.json`, exit 0). This compiles the M2 presentation and save-map changes; it is not gameplay acceptance.

The persistent map was generated and saved in the local host: `Content/Coastal/Maps/L_FirstSignal.umap`. The third assembly run recorded 1,322 actors, including 989 furnished cabin components (`local-evidence/m2-assembly.json`, `m2-assemble-3.log`, explicit `COASTAL_M2_ASSEMBLED` marker). The first attempt encountered a demo-level-only dynamic material instance; the independent scene retains that mesh's authored default material instead. Composition captures of the second attempt exposed Python's native-make rotation argument order; the third uses explicit pitch/yaw/roll keywords throughout. Earlier maps and captures remain in local evidence and do not establish the final scene's appearance.

The refinement pass is saved and all five composition captures completed (`local-evidence/m2-refine-capture-final.log`, explicit `COASTAL_M2_SURFACES_SAVED` and `COASTAL_M2_EDITOR_CAPTURE_COMPLETE` markers). It grounds the trees, blends owned grass/dirt/rock textures using an original linear terrain mask, adds timber surfaces and an interior reading light, places distant cliffs in the sea, and extends the water surface with a waterline recovery volume. The captures were visually inspected during authoring; they are not a gameplay run.

The local host's game/editor startup maps now select `L_FirstSignal`, and its cook list also preserves the M1 systems room. The Windows game target compiled, cooked, staged and archived successfully (`local-evidence/m2-windows-package-ssd.execution.json`, exit 0; log ends `BUILD SUCCESSFUL`, September 7, 2026, 06:53:51 UTC). The delivery is `F:/coastline/LocalPackageM2/Windows/TP_ThirdPerson.exe`, with `F:/coastline/Play M2.cmd` as a convenience launcher. The generated UFS manifest includes the cooked First Signal map; the archive contains its executable and Pak/IoStore content. The package was not launched. The temporary SSD compiler cache has been restored to F: and removed from C:, as recorded in `local-evidence/m2-ssd-cache-state.json`. Actor counts and editor captures do not establish playability. New-game completion, out-of-order collection and partial-save resumption remain explicitly deferred acceptance checks.

The [scope audit](M2_SCOPE_AUDIT.md) maps each M2 requirement to its implementation evidence and explicitly separates deferred gameplay acceptance. Cooking logged inherited gameplay-tag warnings, transient development-label FText warnings and one static-mesh Nanite build warning. These remain recorded in the build log; no packaged rendering or performance result is inferred from successful cooking.

Saved actor properties were read back in the editor without entering gameplay (`local-evidence/m2-authored-inventory.json`). All seven logical roles are present; the two distinct pickup actors each contain exactly one correct item and retain their containers. The postcard has the intended journal ID. Each interactive artwork mesh has convex collision geometry, and the existing custom Hyper collision profile is retained. This is asset metadata inspection, not interaction or save/load acceptance.

To reproduce the authored result in a new local host, run the geometry generator, the base editor assembly recipe, then `refine_first_signal.py`. The geometry generator also produces the linear mask texture data; vendor textures remain locally licensed dependencies. All Unreal recipes require the full editor and refuse unsaved map work.
