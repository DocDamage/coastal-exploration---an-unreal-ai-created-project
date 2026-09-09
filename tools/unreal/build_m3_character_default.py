"""Create and generate a clothed human Mutable default without changing sample sources."""
import json
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Expected independent host')
u.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/Game/Character'], True)
path = '/Game/Coastal/Character/COI_CoastalDefault'
if u.EditorAssetLibrary.does_asset_exist(path):
    instance = u.load_asset(path)
else:
    instance = u.EditorAssetLibrary.duplicate_asset('/Game/Character/COI_Character', path)
if not instance:
    raise RuntimeError('Unable to create owned instance')
graph = instance.get_customizable_object()
if not graph.is_compiled():
    raise RuntimeError('Compile staged graph before default authoring')
choices = {'Body Type 1':'Regular', 'Body Type 2':'Regular', 'Pants':'Jeans', 'Shirts':'TShirt',
           'Shoes':'Boots', 'Head Accessories':'None', 'Jacket':'Jacket_A', 'Eye Color':'BrownHazel',
           'Cyborg Eye L':'Normal', 'Cyborg Eye R':'Normal', 'Left Arm':'None', 'Right Arm':'None',
           'Make Up':'None', 'Patterns TShirt':'None', 'TShirt Colors':'Blue'}
for name, value in choices.items():
    if value not in [graph.get_enum_parameter_value(name, i) for i in range(graph.get_enum_parameter_num_values(name))]:
        raise RuntimeError('Unrecognized default selection: ' + name)
    instance.set_enum_parameter_selected_option(name, value)
instance.set_float_parameter_selected_option('Skin Tone', 0.5)
instance.set_float_parameter_selected_option('Body Blend', 1.0)
u.EditorAssetLibrary.save_loaded_asset(instance)
test_instance = instance.clone()
test_instance.update_skeletal_mesh_async(True, True)
output = ROOT / 'local-evidence/m3-character-default-generation.json'
output.write_text(json.dumps(dict(status='generating', choices=choices)))
started = time.monotonic()


def tick(delta):
    body = test_instance.get_skeletal_mesh_component_skeletal_mesh('Body')
    head = test_instance.get_skeletal_mesh_component_skeletal_mesh('Head')
    if body and head:
        u.unregister_slate_post_tick_callback(handle)
        output.write_text(json.dumps(dict(status='generated', seconds=time.monotonic()-started,
            components={name:dict(mesh=mesh.get_path_name(), skeleton=mesh.skeleton.get_path_name()) for name,mesh in [('Body',body),('Head',head)]}, choices=choices), indent=2))
    elif time.monotonic() - started > 300:
        u.unregister_slate_post_tick_callback(handle)
        output.write_text(json.dumps(dict(status='timeout', seconds=time.monotonic()-started)))


handle = u.register_slate_post_tick_callback(tick)
