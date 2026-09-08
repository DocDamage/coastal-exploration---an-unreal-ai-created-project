"""Create a separate capacity acceptance map in the external host project."""
import hashlib
import json
import re
from pathlib import Path
import unreal

if re.search(r"(?:^|\s)-(?:nullrhi|run=pythonscript)(?:\s|$)", unreal.SystemLibrary.get_command_line(), re.I):
    raise RuntimeError("Use the full editor with a real rendering backend.")
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if level.is_in_play_in_editor():
    raise RuntimeError("Stop PIE before generating assets.")
if list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()) or list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()):
    raise RuntimeError("Preserve unsaved editor work before generating assets.")
source = "/Game/Coastal/Maps/L_SystemsTest_Integrated"
target = "/Game/Coastal/Maps/L_SystemsTest_Capacity"
if unreal.EditorAssetLibrary.does_asset_exist(target):
    raise RuntimeError("Preserve the existing capacity map; this generator creates only a new map.")
source_file = Path(unreal.Paths.project_content_dir()) / "Coastal/Maps/L_SystemsTest_Integrated.umap"
before = hashlib.sha256(source_file.read_bytes()).hexdigest()
backup = Path(unreal.Paths.project_saved_dir()) / "CoastalAcceptance/integrated-map-before-capacity.umap"
backup.parent.mkdir(parents=True, exist_ok=True)
with backup.open("xb") as stream:
    stream.write(source_file.read_bytes())
if not level.load_level(source):
    raise RuntimeError("Cannot load the actual integrated room.")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not unreal.EditorLoadingAndSavingUtils.save_map(world, target):
    raise RuntimeError("Cannot create separate capacity map.")
if not level.load_level(target):
    raise RuntimeError("Cannot activate the new capacity map; source must remain untouched.")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
cls = unreal.load_class(None, "/Game/Coastal/Integration/BP_CoastalHyperWorldObject.BP_CoastalHyperWorldObject_C")
if not cls:
    raise RuntimeError("Actual Hyper world-object wrapper is required.")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
reference = next(a for a in actors.get_all_level_actors()
                 if isinstance(a, unreal.CoastalWorldObject) and str(a.world_id) == "world.test.battery")
interaction_response = reference.proxy_mesh.get_collision_response_to_channel(unreal.CollisionChannel.ECC_INTERACTION)
for index in range(72):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(-1450 + (index % 12) * 250, -100 + (index // 12) * 200, 40))
    if not actor:
        raise RuntimeError(f"Could not create postcard {index+1}; inspect the partial map.")
    actor.set_actor_label(f"DEV_Capacity_Postcard_{index+1:03d}")
    actor.set_editor_property("world_id", unreal.Name(f"world.test.postcard.{index+1:03d}"))
    actor.set_editor_property("kind", unreal.CoastalObjectKind.PICKUP)
    actor.set_editor_property("display_label", f"Capacity postcard {index+1:03d}")
    actor.set_editor_property("proxy_size_cm", unreal.Vector(20, 25, 10))
    item = unreal.CoastalItemRequirement()
    item.set_editor_property("item_id", unreal.Name("item.old_postcard"))
    item.set_editor_property("quantity", 1)
    actor.set_editor_property("pickup_item", item)
    actor.refresh_development_proxy()
    actor.proxy_mesh.set_collision_response_to_channel(unreal.CollisionChannel.ECC_INTERACTION, interaction_response)
if not level.save_current_level():
    raise RuntimeError("Cannot save capacity map.")
after = hashlib.sha256(source_file.read_bytes()).hexdigest()
if before != after:
    raise RuntimeError("Source map changed unexpectedly.")
result = unreal.CoastalIntegrationLibrary.audit_test_room(world, capacity_fixture=True)
if not result.passed:
    raise RuntimeError("Capacity room failed its strict native manifest audit.")
unreal.log(f"COASTAL_CAPACITY_ROOM_AUDIT {result}")
report = {"source": source, "target": target, "source_sha256_before": before, "source_sha256_after": after, "postcard_pickups": 72}
output = Path(unreal.Paths.project_saved_dir()) / "CoastalAcceptance/capacity-room-created.json"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, indent=2))
unreal.log("COASTAL_CAPACITY_ROOM_CREATED")
unreal.SystemLibrary.quit_editor()
