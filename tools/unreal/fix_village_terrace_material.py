"""Use world-sized stone texture projection on the tall village retaining wall."""
import json
from pathlib import Path
import unreal as u

level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
if level.is_in_play_in_editor() or editor.get_editor_world().get_name() != 'L_FirstSignal':
    raise RuntimeError('Expected First Signal outside PIE')
path = '/Game/Coastal/M3/Materials/M_VillageRetainingStone'
material = u.load_asset(path)
lib = u.MaterialEditingLibrary
if material is None:
    texture = u.load_asset('/Game/ItalianMedievalTown/Textures/Stone/T_Stones_01a_BaseColor')
    function = u.load_asset('/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture')
    if texture is None or function is None:
        raise RuntimeError('Missing installed stone texture or engine projection function')
    material = u.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_VillageRetainingStone', '/Game/Coastal/M3/Materials', u.Material, u.MaterialFactoryNew())
    sample = lib.create_material_expression(material, u.MaterialExpressionTextureObject)
    sample.set_editor_property('texture', texture)
    aligned = lib.create_material_expression(material, u.MaterialExpressionMaterialFunctionCall)
    aligned.set_editor_property('material_function', function)
    size = lib.create_material_expression(material, u.MaterialExpressionConstant3Vector)
    size.set_editor_property('constant', u.LinearColor(400, 400, 400))
    roughness = lib.create_material_expression(material, u.MaterialExpressionConstant)
    roughness.set_editor_property('r', .88)
    links = [lib.connect_material_expressions(sample, '', aligned, 'TextureObject'),
             lib.connect_material_expressions(size, '', aligned, 'TextureSize'),
             lib.connect_material_property(aligned, 'XYZ Texture', u.MaterialProperty.MP_BASE_COLOR),
             lib.connect_material_property(roughness, '', u.MaterialProperty.MP_ROUGHNESS)]
    if not all(links):
        raise RuntimeError('Stone projection graph connection failed: ' + str(links))
    lib.recompile_material(material)
    if not u.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Retaining stone material save failed')
matches = [a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
           if a.actor_has_tag('Coastal.Expansion.medieval_italian_village.Masonry terrace')]
if len(matches) != 1:
    raise RuntimeError('Expected one village retaining terrace')
matches[0].static_mesh_component.set_material(0, material)
if not level.save_current_level():
    raise RuntimeError('Retaining-wall material assignment save failed')
Path('F:/coastline/local-evidence/m3-omitted-village-material.json').write_text(
    json.dumps({'material': path, 'texture_size_cm': 400,
                'scope': 'World-space tri-planar base color; roughness 0.88; visual verification separate'}, indent=2))
print('Saved world-space retaining-wall material')
