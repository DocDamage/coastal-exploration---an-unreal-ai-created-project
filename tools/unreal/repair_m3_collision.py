"""Repair terrain winding and diagnosed decorative collision obstructions.

Run in the open First Signal editor through MCP, outside PIE. Original map and
terrain remain in local evidence / the original asset; vendor assets are untouched.
"""
import json
from pathlib import Path
import shutil
import unreal as u

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT.parent / "local-evidence"
HOST = ROOT.parent / "LocalHost/CoastalExploration"
MAP = "/Game/Coastal/Maps/L_FirstSignal"
FIXED = "/Game/Coastal/M3/Geometry/SM_M3CoastalTerrain_CollisionFixed"
DECORATIONS = {"StaticMeshActor_20": "Cabin_BP_Plastic_Trays",
               "StaticMeshActor_80": "Cabin_BP_Plastic_Trays2",
               "StaticMeshActor_401": "Cabin_BP_Wooden_Box_Low2",
               "StaticMeshActor_530": "Cabin_SM_Cobwebs_01",
               "StaticMeshActor_741": "Cabin_BP_Wooden_Box_Low"}
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
if level.is_in_play_in_editor() or list(u.EditorLoadingAndSavingUtils.get_dirty_map_packages()):
    raise RuntimeError("Preserve unsaved work and stop PIE before collision repair")
if editor.get_editor_world().get_path_name() != MAP + ".L_FirstSignal":
    raise RuntimeError("Open First Signal before collision repair")
all_actors = actors.get_all_level_actors()
ground = [a for a in all_actors if a.get_actor_label() == "Coastal headland and walking trail"]
decorations = [a for a in all_actors if a.get_name() in DECORATIONS]
if len(ground) != 1 or {a.get_name(): a.get_actor_label() for a in decorations} != DECORATIONS:
    raise RuntimeError("Expected terrain and exact diagnosed decorative actors")
backup = OUT / "L_FirstSignal-before-m3-collision-repair.umap"
if not backup.exists():
    shutil.copy2(HOST / "Content/Coastal/Maps/L_FirstSignal.umap", backup)
source_dir = OUT / "m3-collision-source"
source_dir.mkdir(exist_ok=True)
original = OUT / "m3-environment-source/SM_M3CoastalTerrain.obj"
fixed_obj = source_dir / "SM_M3CoastalTerrain_CollisionFixed.obj"
lines = []
for line in original.read_text().splitlines():
    if line.startswith("f "):
        indices = line.split()[1:]
        line = "f " + " ".join([indices[0], *reversed(indices[1:])])
    lines.append(line)
fixed_obj.write_text("\n".join(lines))
shutil.copy2(original.with_suffix(".mtl"), source_dir / original.with_suffix(".mtl").name)
component = ground[0].get_component_by_class(u.StaticMeshComponent)
old_mesh = component.get_editor_property("static_mesh")
if not u.EditorAssetLibrary.does_asset_exist(FIXED):
    task = u.AssetImportTask()
    task.set_editor_property("filename", str(fixed_obj))
    task.set_editor_property("destination_path", "/Game/Coastal/M3/Geometry")
    task.set_editor_property("destination_name", "SM_M3CoastalTerrain_CollisionFixed")
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
fixed_mesh = u.load_asset(FIXED)
if not fixed_mesh:
    raise RuntimeError("Corrected terrain import failed")
fixed_mesh.get_editor_property("body_setup").set_editor_property(
    "collision_trace_flag", u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
fixed_mesh.set_editor_property("static_materials", old_mesh.get_editor_property("static_materials"))
if not u.EditorAssetLibrary.save_loaded_asset(fixed_mesh):
    raise RuntimeError("Corrected terrain save failed")
component.set_static_mesh(fixed_mesh)
for actor in decorations:
    actor.get_component_by_class(u.StaticMeshComponent).set_collision_enabled(u.CollisionEnabled.NO_COLLISION)

report = {"terrain": FIXED, "disabled_decorations": sorted(DECORATIONS), "floor_traces": []}
for x, y, z in [(0, 0, 300), (12500, 1800, 250), (7650, 7850, 1150)]:
    hit = u.SystemLibrary.line_trace_single(editor.get_editor_world(), u.Vector(x, y, z+200),
        u.Vector(x, y, z-200), u.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], u.DrawDebugTrace.NONE)
    data = hit.to_tuple() if hit else None
    if not data or not data[0] or data[1] or data[7].z < 0.7:
        raise RuntimeError("Corrected checkpoint floor trace failed at {}, {}".format(x, y))
    report["floor_traces"].append({"x": x, "y": y, "floor_z": data[5].z,
                                    "normal_z": data[7].z, "actor": data[9].get_actor_label()})
if not level.save_current_level():
    raise RuntimeError("Collision-repaired map save failed")
(OUT / "m3-collision-repair.json").write_text(json.dumps(report, indent=2))
u.log("COASTAL_M3_COLLISION_REPAIRED " + json.dumps(report))
