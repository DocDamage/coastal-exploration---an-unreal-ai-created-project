import unreal,runpy
import re
if re.search(r"(?:^|\s)-(?:nullrhi|run=pythonscript)(?:\s|$)", unreal.SystemLibrary.get_command_line(), re.IGNORECASE):
    raise RuntimeError("Use the full editor with a real rendering backend before generating assets.")
if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError("Stop PIE before generating assets.")
if list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()) or list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()):
    raise RuntimeError("Preserve unsaved editor work before generating assets.")
from pathlib import Path
root=Path(__file__).resolve().parents[2]
path="/Game/Coastal/Integration/BP_CoastalGameMode"
if unreal.EditorAssetLibrary.does_asset_exist(path):raise RuntimeError("Preserve existing GameMode")
factory=unreal.BlueprintFactory()
factory.set_editor_property("parent_class",unreal.load_class(None,"/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode.BP_ThirdPersonGameMode_C"))
bp=unreal.AssetToolsHelpers.get_asset_tools().create_asset("BP_CoastalGameMode","/Game/Coastal/Integration",unreal.Blueprint,factory)
unreal.get_default_object(bp.generated_class()).set_editor_property("player_controller_class",unreal.load_class(None,"/Game/Coastal/Integration/BP_CoastalPlayerController.BP_CoastalPlayerController_C"))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
if not unreal.EditorAssetLibrary.save_loaded_asset(bp):raise RuntimeError("GameMode save failed")
runpy.run_path(str(root/"tools/unreal/build_systems_test.py"))["build"](include_safety=True)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode",bp.generated_class())
if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():raise RuntimeError("Room save failed")
report=runpy.run_path(str(root/"tools/unreal/audit_systems_test.py"))["audit"](str(Path(unreal.Paths.project_saved_dir())/"CoastalAcceptance/integrated-room-audit.json"))
if not report["room_manifest_passed"]:raise RuntimeError("Integrated manifest failed")
unreal.log("COASTAL_INTEGRATED_ROOM_CREATED")
unreal.SystemLibrary.quit_editor()
