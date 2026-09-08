"""Correct the project-owned material samplers without modifying vendor textures."""
import json
from pathlib import Path
import unreal as u

OUT = Path(__file__).resolve().parents[3] / 'local-evidence'
lib = u.MaterialEditingLibrary
source = '/Game/Fishermans_Cabin/Textures/Tiling_Textures/Rock/T_Rocks_N'
destination = '/Game/Coastal/M3/Textures/T_RockNormal'
if not u.EditorAssetLibrary.does_asset_exist(destination):
    if not u.EditorAssetLibrary.duplicate_asset(source, destination):
        raise RuntimeError('Cannot duplicate owned texture')
normal = u.load_asset(destination)
normal.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_NORMALMAP)
normal.set_editor_property('srgb', False)
u.EditorAssetLibrary.save_loaded_asset(normal)
result = {}
for name in ['M_HeadlandGround', 'M_RailBallast']:
    mat = u.load_asset('/Game/Coastal/M3/Materials/' + name)
    if not mat:
        raise RuntimeError('Surface material missing')
    todo = [lib.get_material_property_input_node(mat, u.MaterialProperty.MP_NORMAL)]
    visited, replaced = set(), 0
    while todo:
        expr = todo.pop()
        if not expr or expr.get_path_name() in visited:
            continue
        visited.add(expr.get_path_name())
        if isinstance(expr, u.MaterialExpressionTextureSample):
            tex = expr.get_editor_property('texture')
            if tex and tex.get_path_name().split('.')[0] in (source, destination):
                expr.set_editor_property('texture', normal)
                expr.set_editor_property('sampler_type', u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
                replaced += 1
        todo.extend(lib.get_inputs_for_material_expression(mat, expr))
    if replaced != 1:
        raise RuntimeError('Expected one rock normal expression in ' + name)
    lib.recompile_material(mat)
    u.EditorAssetLibrary.save_loaded_asset(mat)
    result[name] = replaced
(OUT/'m3-surface-normal-repair.json').write_text(json.dumps(result, indent=2))
u.log('COASTAL_M3_SURFACE_NORMALS_REPAIRED')
