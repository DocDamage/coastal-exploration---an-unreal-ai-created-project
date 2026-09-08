"""Repair one cabinet decoration's interaction trace response, outside PIE."""
from pathlib import Path
import json
import shutil
import unreal as u

ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT.parent / "LocalHost/CoastalExploration"
OUT = ROOT.parent / "local-evidence"
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
if Path(u.Paths.get_project_file_path()).resolve().parent != HOST.resolve():
    raise RuntimeError("Expected the CoastalExploration local host")
if level.is_in_play_in_editor() or list(u.EditorLoadingAndSavingUtils.get_dirty_map_packages()):
    raise RuntimeError("Stop PIE and preserve unsaved map work first")
if editor.get_editor_world().get_path_name() != "/Game/Coastal/Maps/L_FirstSignal.L_FirstSignal":
    raise RuntimeError("Open First Signal first")
matches = [a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
           if a.get_name() == "StaticMeshActor_780" and a.get_actor_label() == "Cabin_SM_Books_175"]
if len(matches) != 1:
    raise RuntimeError("Expected the exact diagnosed cabinet book instance")
component = matches[0].get_component_by_class(u.StaticMeshComponent)
if component.static_mesh.get_path_name() != "/Game/Fishermans_Cabin/Meshes/Books/SM_Books_09.SM_Books_09":
    raise RuntimeError("Cabinet decoration mesh changed")
backup = OUT / "L_FirstSignal-before-storage-visibility.umap"
if not backup.exists():
    shutil.copy2(HOST / "Content/Coastal/Maps/L_FirstSignal.umap", backup)
component.set_collision_response_to_channel(u.CollisionChannel.ECC_VISIBILITY,
                                             u.CollisionResponseType.ECR_IGNORE)
if not level.save_current_level():
    raise RuntimeError("Could not save the repaired map")
report = {"actor": matches[0].get_name(), "label": matches[0].get_actor_label(),
          "visibility": str(component.get_collision_response_to_channel(u.CollisionChannel.ECC_VISIBILITY)),
          "collision_enabled": str(component.get_collision_enabled()), "backup": str(backup)}
(OUT / "m3-storage-visibility-repair.json").write_text(json.dumps(report, indent=2))
print(json.dumps(report))
