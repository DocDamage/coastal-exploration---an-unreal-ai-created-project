"""Import normalized owned survey art into the local host, without changing maps."""
import json
from pathlib import Path
import unreal as u

host = Path('F:/coastline/LocalHost/CoastalExploration')
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (host / 'CoastalExploration.uproject').resolve():
    raise RuntimeError('Wrong host')
tools = u.AssetToolsHelpers.get_asset_tools()
report = []
for name in ('Hallsands', 'Baelo'):
    path = '/Game/Coastal/Expansion/' + name
    asset_path = path + '/SM_' + name
    if not u.EditorAssetLibrary.does_asset_exist(asset_path):
        task = u.AssetImportTask()
        task.set_editor_property('filename', 'F:/coastline/LocalVendor/Expansion/' + name + '/SM_' + name + '.fbx')
        task.set_editor_property('destination_path', path)
        task.set_editor_property('destination_name', 'SM_' + name)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        options = u.FbxImportUI()
        options.set_editor_property('import_mesh', True)
        options.set_editor_property('import_as_skeletal', False)
        options.set_editor_property('import_materials', True)
        options.set_editor_property('import_textures', True)
        options.static_mesh_import_data.set_editor_property('combine_meshes', True)
        options.static_mesh_import_data.set_editor_property('auto_generate_collision', False)
        task.set_editor_property('options', options)
        tools.import_asset_tasks([task])
    mesh = u.load_asset(asset_path)
    if not isinstance(mesh, u.StaticMesh):
        raise RuntimeError('Missing imported mesh: ' + asset_path)
    mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag', u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    u.EditorAssetLibrary.save_loaded_asset(mesh)
    bounds = mesh.get_bounds()
    report.append({'asset': mesh.get_path_name(), 'origin': list(bounds.origin.to_tuple()),
        'extent': list(bounds.box_extent.to_tuple()),
        'materials': [str(s.material_interface.get_path_name()) if s.material_interface else None for s in mesh.static_materials]})
Path('F:/coastline/local-evidence/m3-expansion-scan-import.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
