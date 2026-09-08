"""Bind the owned scan's JPEG atlas explicitly after FBX drops its texture link."""
import json
from pathlib import Path
import unreal as u

if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('Run outside PIE')
root = '/Game/Coastal/Expansion/Baelo'
tools = u.AssetToolsHelpers.get_asset_tools()
texture = u.load_asset(root+'/T_Baelo_Albedo')
if not texture:
    task = u.AssetImportTask()
    atlas = Path('F:/coastline/LocalVendor/Expansion/Baelo/Image_0.jpg')
    if not atlas.exists():
        atlas = atlas.with_name('Image_0-from-blender.jpg')
    task.filename = str(atlas)
    task.destination_path = root
    task.destination_name = 'T_Baelo_Albedo'
    task.automated = True
    task.save = True
    tools.import_asset_tasks([task])
    texture = u.load_asset(root+'/T_Baelo_Albedo')
if not isinstance(texture, u.Texture2D):
    raise RuntimeError('Baelo texture import failed')
texture.set_editor_property('srgb', True)
mat = u.load_asset(root+'/M_Baelo_Surface')
if not mat:
    mat = tools.create_asset('M_Baelo_Surface', root, u.Material, u.MaterialFactoryNew())
    sample = u.MaterialEditingLibrary.create_material_expression(mat, u.MaterialExpressionTextureSample, -400, 0)
    sample.set_editor_property('texture', texture)
    u.MaterialEditingLibrary.connect_material_property(sample, 'RGB', u.MaterialProperty.MP_BASE_COLOR)
    rough = u.MaterialEditingLibrary.create_material_expression(mat, u.MaterialExpressionConstant, -400, 250)
    rough.set_editor_property('r', .95)
    u.MaterialEditingLibrary.connect_material_property(rough, '', u.MaterialProperty.MP_ROUGHNESS)
    u.MaterialEditingLibrary.recompile_material(mat)
mesh = u.load_asset(root+'/SM_Baelo')
mesh.set_material(0, mat)
for asset in [texture, mat, mesh]:
    if not u.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError('Save failed: '+asset.get_path_name())
Path('F:/coastline/local-evidence/m3-baelo-material-repair.json').write_text(json.dumps({
    'texture': texture.get_path_name(), 'material': mat.get_path_name(),
    'mesh': mesh.get_path_name(), 'reason': 'Imported FBX material had no texture parameters.'}, indent=2))
print('Baelo atlas bound and saved.')
