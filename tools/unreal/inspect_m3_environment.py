"""Read owned environment assets; no map, vendor asset or gameplay mutation."""
import json
from pathlib import Path
import unreal as u

out = Path(__file__).resolve().parents[3] / 'local-evidence/m3-environment-assets.json'
lib = u.MaterialEditingLibrary
water = u.load_asset('/Game/Fishermans_Cabin/Materials/Material_Instances/Tiling/MI_Water')
if not water:
    raise RuntimeError('Owned water material is missing')
report = {'water': {'path': water.get_path_name(), 'parent': water.get_editor_property('parent').get_path_name()}}
report['water']['scalars'] = {str(n): lib.get_material_instance_scalar_parameter_value(water, n) for n in lib.get_scalar_parameter_names(water)}
report['water']['textures'] = {}
for name in lib.get_texture_parameter_names(water):
    value = lib.get_material_instance_texture_parameter_value(water, name)
    report['water']['textures'][str(name)] = value.get_path_name() if value else None
report['meshes'] = []
for path in ['Rocks/SM_Rocks_01', 'Small_Rocks/SM_Small_Rocks_01', 'Foliage/Grass/SM_Grass_03',
             'Barrel/SM_Barrel_01', 'Ropes/SM_Wooden_Dock_Knot']:
    mesh = u.load_asset('/Game/Fishermans_Cabin/Meshes/' + path)
    if not mesh:
        raise RuntimeError('Missing owned mesh ' + path)
    bounds = mesh.get_bounding_box()
    report['meshes'].append({'path': mesh.get_path_name(),
        'min': [bounds.min.x, bounds.min.y, bounds.min.z], 'max': [bounds.max.x, bounds.max.y, bounds.max.z]})
out.write_text(json.dumps(report, indent=2))
u.log('COASTAL_M3_ASSETS_INSPECTED')
u.SystemLibrary.quit_editor()
