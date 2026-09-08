# M3 environment construction

The First Signal map now uses a project-owned copy of the installed Nordic water instance with calmer wave settings and roughness 0.18. The vendor material and mesh assets remain unchanged. Thirty-two small shoreline rock actors and 48 grass patches supplement the earlier broad M2 dressing. These decorative additions have no collision; grass shadows are disabled and grass culls beyond 85 metres. Performance has not been measured.

An original combined mesh adds individual board tops over the existing dock collision surface. Editor review exposed an inherited M2 issue: the seaward pier stood largely over dry terrain. A new project-owned terrain mesh shapes a shallow basin beneath it, starting seaward of the landing. The route's sampled heights remain unchanged, and 23 scenery actors were moved vertically to follow the revised terrain. The continuous dock collider and existing deep-water recovery volume remain in place. Actual walking, camera collision and water recovery remain untested.

The offline layout uses a fixed seed and explicit clearance rules, not a runtime procedural world. Placed decoration centers are at least 706.68 cm from the route/clearing-distance field used by the terrain recipe. These are authored-coordinate checks, not navigation or physical-player acceptance. The basin recipe also samples 101 positions per original path segment and refuses a change to their heights.

## Evidence and reproduction

All evidence below is under `F:/coastline/local-evidence`:

- `m3-environment-assets.json`: inspected owned water parameters and real mesh bounds.
- `m3-environment-source`: original OBJ/MTL geometry and deterministic placement JSON.
- `m3-environment-assembly.json`: saved 81 decoration actors, water assignment and unchanged persistent-object IDs/transforms at dressing time.
- `m3-basin.json`: saved basin terrain and route-height checks; scenery adjustments.
- `m3-environment-backup/L_FirstSignal.umap`: pre-dressing map backup. It refers to the retained M2 assets.
- `m3-environment-assemble-2.log` and `m3-environment-basin.log`: explicit save markers. Process exit alone is insufficient because Unreal can exit zero after a Python error.
- `m3-environment-{cabin,shore,dock}.png`: editor composition captures. Initial pre-basin captures are retained in `m3-environment-before-basin`.

Use `tools/unreal/m3_environment_layout.py` with ordinary Python to generate the original source geometry. `inspect_m3_environment.py` reads owned asset settings in the editor. On an undressed M2 map, run `dress_and_capture_first_signal.py` in the full editor; it dresses, shapes the basin and captures without entering gameplay. Recipes refuse unsaved maps and duplicate construction. `finish_and_capture_m3_basin.py` resumes the already-dressed, pre-basin stage. Do not rerun completed construction recipes on the current map.

The first dressing attempt stopped before saving the map: UE 5.7's scalar material-instance setter returns false even after writing. The installed implementation was inspected, and the recipe now verifies the parameter by read-back. The initial attempt's log is retained as failed construction evidence.

No C++ build, gameplay test or Windows package was run for this environment batch. The new map/assets live in `LocalHost`; `LocalPackageM2` and its launcher remain unchanged. M3's native UI/graphics/input changes still await an integrated build. Broad terrain dressing, final character art, item icons, audio detail and performance remain unfinished.

## Surface and rail continuation

The next art pass adds a project-owned ground material with tinted grass, dirt and rocky-shore layers using the existing route mask. Matching normal textures add surface detail. The materials use installed owned textures; no vendor assets are edited. A decorative ballast bed fills the gap under the rail sleepers. The original solid pier/landing meshes are hidden visually to remove overlap with the plank surface and terrain; their collision settings are untouched.

`tools/unreal/polish_and_capture_m3.py` constructs this pass once and captures four editor views. `data/m3_surface_spec.json` records the authored values. It refuses existing surface materials so that later edits cannot be silently overwritten. The pre-pass map backup is `local-evidence/m3-surface-backup/L_FirstSignal.umap`.

The first attempt caught a texture UV pin-name mismatch before saving. The next attempt saved the materials but captures exposed a shader error: the installed rock normal texture uses linear-color compression. The repair creates `/Game/Coastal/M3/Textures/T_RockNormal` with normal-map compression and updates the two project-owned material graphs. Fresh construction now makes this copy automatically. `repair_and_capture_m3_surfaces.py` handles the already-created materials. Initial logs/captures remain diagnostic evidence, not successful visual results.

Use `m3-surface-normal-repair.log` and `m3-surface-normal-repair.json` for the correction, `m3-surfaces.json` for the map edits, and `m3-surfaces-final-{cabin,shore,dock,overlook}.png` for the final composition views. Material/texture shader processing in the editor is asset construction, not a C++ build or packaged-game acceptance.

All four corrected views were reviewed: terrain layers render again, the dock plank surface remains visible, and ballast fills the sleeper bed. The repair log contains the old sampler errors during initial asset loading, followed by the repair and capture-complete markers; no further sampler errors appear after repair. The final lightweight check passed 3,380 structural checks with zero failures (`m3-batch5-source-check-final.log`). Native compilation, gameplay and packaged acceptance remain deferred.
