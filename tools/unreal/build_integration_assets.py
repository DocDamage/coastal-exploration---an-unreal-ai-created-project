import unreal
import re
if re.search(r"(?:^|\s)-(?:nullrhi|run=pythonscript)(?:\s|$)", unreal.SystemLibrary.get_command_line(), re.IGNORECASE):
    raise RuntimeError("Use the full editor with a real rendering backend before generating assets.")
if unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError("Stop PIE before generating assets.")
if list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()) or list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()):
    raise RuntimeError("Preserve unsaved editor work before generating assets.")
if not unreal.CoastalVendorInspection.build_integration_assets():
    raise RuntimeError("Integration asset creation failed; inspect log and preserve partial assets.")
unreal.SystemLibrary.quit_editor()
