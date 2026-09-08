"""Import project-owned icon sources; run inside Unreal outside PIE."""
from pathlib import Path
import json
import unreal as u

ROOT = Path(__file__).resolve().parents[2]
DEST = "/Game/Coastal/UI/Icons"
HOST = ROOT.parent / "LocalHost/CoastalExploration"
if Path(u.Paths.get_project_file_path()).resolve().parent != HOST.resolve():
    raise RuntimeError("Run in the CoastalExploration local host")
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError("Stop PIE before importing icons")
report = []
for name in ("T_RadioBattery", "T_MarineFuse"):
    source = ROOT / "art/ui/icons" / (name + ".png")
    if not source.is_file():
        raise RuntimeError("Generate icon sources first: " + str(source))
    task = u.AssetImportTask()
    for key, value in dict(filename=str(source), destination_path=DEST,
                           destination_name=name, automated=True,
                           replace_existing=True, save=False).items():
        task.set_editor_property(key, value)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = u.load_asset(DEST + "/" + name)
    if not isinstance(texture, u.Texture2D):
        raise RuntimeError("Icon import failed: " + name)
    texture.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("lod_group", u.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", u.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)
    if not u.EditorAssetLibrary.save_loaded_asset(texture):
        raise RuntimeError("Icon save failed: " + name)
    report.append({"asset": texture.get_path_name(), "source": str(source)})
# Native LoadObject paths are not discovered by the cooker. Preserve all other
# host settings and add this exact project-owned directory once in its section.
config = HOST / "Config/DefaultGame.ini"
text = config.read_text(encoding="utf-8-sig")
section = "[/Script/UnrealEd.ProjectPackagingSettings]"
rule = '+DirectoriesToAlwaysCook=(Path="/Game/Coastal/UI/Icons")'
if rule not in text:
    if section not in text:
        text += "\n" + section + "\n" + rule + "\n"
    else:
        text = text.replace(section, section + "\n" + rule, 1)
    config.write_text(text, encoding="utf-8")
(ROOT.parent / "local-evidence/m3-item-icon-import.json").write_text(json.dumps(report, indent=2))
print(json.dumps(report))
