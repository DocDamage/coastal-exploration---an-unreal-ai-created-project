# M2 implementation scope and evidence

The scope comes from `COASTAL_EXPLORATION_FOUNDATION.md`, section 14 (M2), and `data/m2_opening.json`. The user explicitly deferred testing. This records implementation and authored-asset evidence; it does not pass the foundation's gameplay gate.

| Requirement | Current evidence | Result |
|---|---|---|
| Cabin Cove, Shoreline Trail, Old Dock, optional Rail Overlook | Saved `LocalHost/CoastalExploration/Content/Coastal/Maps/L_FirstSignal.umap`; five final `local-evidence/m2-*.png` composition captures; `m2-assembly.json` | Assembled and visually inspected |
| Compact connected route | Original terrain and route in `data/m2_opening.json`, `tools/unreal/m2_geometry.py`; imported geometry bounds in `m2-imported-geometry.json` | Authored route approximately 525 m round trip including optional branch; traversal timing not measured |
| Replace interaction proxies with art | `m2-authored-inventory.json`: seven assigned mesh references; original radio, owned door/storage/books/crates; `CoastalWorldObject.cpp` hides development labels for assigned art | Implemented; simple radio and environment presentation remain listed for M3 |
| Two guaranteed required parts without prerequisite locks | Saved inventory metadata: distinct `world.test.battery` and `world.test.fuse`, quantity one each, correct item IDs, persistent containers; existing pickup transaction boundary retained | Authored and wired; acquisition acceptance deferred |
| Optional discovery and journal text | Saved postcard journal ID and maintenance-note actor; existing native journal/mission text and discovery logic retained | Integrated; UI reading acceptance deferred |
| Mission component with real actors | One native mission director and seven existing logical IDs in assembly recipe and saved actor inventory; existing AGIS/Hyper host game mode; compiled editor and Windows target | Implemented; no second inventory or alternative mission authority |
| New-player objective without developer commands | Real actors and existing mission interactions are wired into startup `L_FirstSignal` | Gameplay gate deferred by user; not claimed passed |
| Either part first; partial-save resume | Existing order-independent mission/transaction/save logic retained; bootstrap uses `level.first_signal`, UI defaults to `first_signal_01` | Implementation retained; M2 out-of-order and partial-save acceptance deferred |
| M1 content and saves preserved | Separate M1 map and `LocalPackageM1Candidate`; M2 has a distinct map identity; no save deletion/migration | Preserved; M1 generations cannot be loaded into the M2 map |
| Workspace on F: | `workspace-move.json` records 15,809 SHA256 matches and preserved Git status; `m2-ssd-cache-state.json` records restored F: cache and removed temporary C: cache | Complete |

All evidence paths above are relative to `F:/coastline`, except repository source paths, which are relative to `F:/coastline/CoastalExploration`. Native build/package results and delivery paths are recorded in `M2_IMPLEMENTATION.md` and `F:/coastline/M2_HANDOFF.md`.

No M2 gameplay, device, automation-test, performance or packaged-launch pass is claimed. M3 retains character art, water/environment polish, camera/controller acceptance, audio detail and performance work. No combat, swimming, boats or future-area travel was added.
