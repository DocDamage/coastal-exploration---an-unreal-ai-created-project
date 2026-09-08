"""Run INSIDE Unreal Editor after compiling CoastalFoundation. Not normal Python.
Creates a NEW proxy map only; does not install assets, configure AGIS or generate UI.
Refuses existing maps and unsaved editor work. On failure, leaves the new map for inspection.
"""
from pathlib import Path
import json
import re
import runpy
import unreal

ROOT = Path(__file__).resolve().parents[2]

def build(include_safety: bool = False) -> None:
    command_line = unreal.SystemLibrary.get_command_line()
    if re.search(r"(?:^|\s)-(?:nullrhi|run=pythonscript)(?:\s|$)", command_line, re.IGNORECASE):
        raise RuntimeError("Room generation requires the full editor with a real rendering backend. "
                           "Use -ExecutePythonScript and, for unattended work, -RenderOffscreen; "
                           "UE 5.7 LevelEditor/thumbnail operations can crash in commandlet or NullRHI mode.")
    spec = json.loads((ROOT / "data/test_room.json").read_text(encoding="utf-8"))
    safety = []
    safety_class = None
    if include_safety:
        validate = runpy.run_path(str(ROOT / "tools/recovery_layout.py"))["validate_layout"]
        safety = validate(json.loads((ROOT / "data/m1_5_safety_layout.json").read_text(encoding="utf-8")))
        safety_class = unreal.load_class(None, "/Script/CoastalFoundation.CoastalSafetyVolume")
        if not safety_class:
            raise RuntimeError("Compile M1.5 safety source before generating the recovery variant.")
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if level.is_in_play_in_editor():
        raise RuntimeError("Stop Play In Editor before generating the test room.")
    dirty = list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
    dirty += list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    if dirty:
        raise RuntimeError("Save or discard unsaved editor changes yourself before running this script.")
    path = spec["map_path"]
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError(f"Refusing to overwrite existing map: {path}")
    director_class = unreal.load_class(None, "/Script/CoastalFoundation.CoastalMissionDirector")
    object_class = unreal.load_class(None, spec.get("world_object_class", "/Script/CoastalFoundation.CoastalWorldObject"))
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    if not director_class or not object_class or not cube:
        raise RuntimeError("Compile/enable CoastalFoundation and confirm Engine BasicShapes content first.")
    ids = [row["world_id"] for row in spec["objects"]]
    if not all(ids) or len(ids) != len(set(ids)):
        raise ValueError("Empty or duplicate authored world IDs.")
    if not level.new_level(path, False):
        raise RuntimeError(f"Could not create new map: {path}")

    def spawn(cls, name, xyz, rotation=None):
        actor = actors.spawn_actor_from_class(cls, unreal.Vector(*xyz), rotation or unreal.Rotator())
        if not actor:
            raise RuntimeError(f"Could not spawn {name}; inspect the partially created test map.")
        actor.set_actor_label(name)
        return actor

    def geometry(name, xyz, dimensions):
        actor = spawn(unreal.StaticMeshActor, name, xyz)
        mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
        mesh.set_static_mesh(cube)
        mesh.set_collision_profile_name("BlockAll")
        actor.set_actor_scale3d(unreal.Vector(*(d / 100.0 for d in dimensions)))
        return actor

    geometry("DEV_Floor", [0, 0, -20], spec["floor_size_cm"])
    geometry("DEV_BackWall", [0, 1200, 160], [3600, 30, 320])
    geometry("DEV_FrontWall", [0, -1200, 160], [3600, 30, 320])
    geometry("DEV_LeftWall", [-1800, 0, 160], [30, 2400, 320])
    geometry("DEV_RightWall", [1800, 0, 160], [30, 2400, 320])
    # Optional obstruction for the character-origin visibility test; no critical route is gated.
    geometry("DEV_OcclusionTestWall", [350, -800, 130], [30, 300, 260])
    spawn(unreal.PlayerStart, "Coastal_PlayerStart", spec["player_start_cm"])
    spawn(unreal.DirectionalLight, "DEV_Sun", [0, 0, 700], unreal.Rotator(-45, -30, 0))
    spawn(unreal.SkyLight, "DEV_SkyLight", [0, 0, 500])
    spawn(director_class, "Coastal_MissionDirector", [0, 0, 200])
    for row in spec["objects"]:
        actor = spawn(object_class, row["world_id"].replace(".", "_"), row["position_cm"])
        actor.set_editor_property("world_id", unreal.Name(row["world_id"]))
        actor.set_editor_property("kind", getattr(unreal.CoastalObjectKind, row["kind"]))
        actor.set_editor_property("display_label", row["label"])
        actor.set_editor_property("proxy_size_cm", unreal.Vector(*row["size_cm"]))
        if "item_id" in row:
            item = unreal.CoastalItemRequirement()
            item.set_editor_property("item_id", unreal.Name(row["item_id"]))
            item.set_editor_property("quantity", 1)
            actor.set_editor_property("pickup_item", item)
        if "journal_entry" in row:
            actor.set_editor_property("journal_entry", unreal.Name(row["journal_entry"]))
        actor.refresh_development_proxy()
    for row in safety:
        actor = spawn(safety_class, row["label"], row["center_cm"])
        actor.set_editor_property("kind", getattr(unreal.CoastalSafetyKind, row["kind"]))
        actor.get_editor_property("bounds").set_box_extent(unreal.Vector(*row["half_extent_cm"]), False)
        # The marker at the actor origin is the authored capsule-center destination.
        # These thin, non-colliding squares are explicit DEV PROXIES, not water artwork.
        x, y, _ = row["center_cm"]
        marker = geometry(row["label"] + "_FloorMarker", [x, y, 1],
                          [row["half_extent_cm"][0] * 2, row["half_extent_cm"][1] * 2, 2])
        marker.get_component_by_class(unreal.StaticMeshComponent).set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if not level.save_current_level():
        raise RuntimeError("Map created but final save failed. Inspect and save it manually.")
    unreal.log(f"Created {path}. Set your Third Person GameMode, wire AGIS/Hyper and M1 widgets next.")
    if safety:
        unreal.log("Safety DEV PROXIES added. Add the character recovery component and verify ReturnPoint heights against the actual capsule.")
    unreal.log("This is a development proxy map, not the coastal opening or a verified playable build.")

if __name__ == "__main__":
    build()
