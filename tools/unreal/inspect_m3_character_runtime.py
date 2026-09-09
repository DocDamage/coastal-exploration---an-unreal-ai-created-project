"""Inspect staged Mutable graph and existing player animation sources in the trial."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
project = Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve()
if project != (ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Expected independent UE5.8 host')
registry = u.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(['/Game/Character'], True)
graphs = registry.get_assets_by_path('/Game/Character', True)
for data in graphs:
    if str(data.asset_class_path.asset_name) == 'CustomizableObject':
        data.get_asset()
graph = u.load_asset('/Game/Character/CO_Character')
if not graph:
    raise RuntimeError('Staged Mutable graph did not load')
params = u.CompileParams()
params.set_editor_property('async_', True)
params.set_editor_property('skip_if_not_out_of_date', False)
graph.compile(params)
output = ROOT / 'local-evidence/m3-character-graph-inspection.json'
output.write_text(json.dumps(dict(status='compiling', project=str(project))))


def capture():
    rows = []
    for i in range(graph.get_parameter_count()):
        name = graph.get_parameter_name(i)
        kind = graph.get_parameter_type_by_name(name)
        row = dict(name=name, type=str(kind), multidimensional=graph.is_parameter_multidimensional(name))
        if 'ENUM' in str(kind) or 'INT' in str(kind):
            row['values'] = [graph.get_enum_parameter_value(name, j) for j in range(graph.get_enum_parameter_num_values(name))]
        elif 'FLOAT' in str(kind):
            row['default'] = graph.get_float_parameter_default_value(name)
        elif 'BOOL' in str(kind):
            row['default'] = graph.get_bool_parameter_default_value(name)
        rows.append(row)
    meshes = []
    for path in ['/Game/Character/Body/SK_BaseBody', '/Game/Character/Body/SK_BaseBody_Head',
                 '/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple']:
        mesh = u.load_asset(path)
        if mesh:
            skeleton = mesh.get_editor_property('skeleton')
            meshes.append(dict(path=path, skeleton=skeleton.get_path_name()))
    record = dict(status='compiled', compiled=graph.is_compiled(), project=str(project),
                  components=[str(graph.get_component_name(i)) for i in range(graph.get_component_count())],
                  parameters=rows, meshes=meshes)
    output.write_text(json.dumps(record, indent=2))
    print('CHARACTER_GRAPH', len(rows), 'parameters', record['components'])


def tick(delta):
    if graph.is_compiled():
        u.unregister_slate_post_tick_callback(handle)
        try:
            capture()
        except Exception as exc:
            output.write_text(json.dumps(dict(status='inspection_error', error=str(exc))))
            raise


handle = u.register_slate_post_tick_callback(tick)
