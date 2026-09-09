"""Read installed Mutable sample metadata without saving or compiling its graphs.

Run in the separate MutableSample project using the Python commandlet. This
script intentionally rejects both Coastal host projects and writes only private
inventory evidence. Dependency presence is not retarget/cook acceptance.
"""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'MutableSample'
actual = Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve()
if actual != (SOURCE / 'MutableSample.uproject').resolve():
    raise RuntimeError('This read-only inventory requires the separate MutableSample project')
registry = u.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
graph = '/Game/Character/CO_Character'
# Child clothing/hair objects can point toward the root rather than the root
# referencing them. Seed every character object so migration retains variants.
graphs = sorted({str(data.package_name) for data in registry.get_assets_by_path('/Game/Character', True)
                 if str(data.asset_class_path.asset_name) == 'CustomizableObject'} | {graph})
options = u.AssetRegistryDependencyOptions(
    include_soft_package_references=True, include_hard_package_references=True,
    include_searchable_names=False, include_soft_management_references=False,
    include_hard_management_references=False)
pending, visited, edges, external = list(graphs), set(), {}, set()
while pending:
    package = pending.pop()
    if package in visited:
        continue
    visited.add(package)
    deps = sorted(str(x) for x in registry.get_dependencies(package, options))
    edges[package] = deps
    for dep in deps:
        if dep.startswith('/Game/'):
            pending.append(dep)
        else:
            external.add(dep)
assets = []
for package in sorted(visited):
    for data in registry.get_assets_by_package_name(package):
        assets.append(dict(package=package, asset=str(data.asset_name),
                           asset_class=str(data.asset_class_path.package_name) + '.' + str(data.asset_class_path.asset_name)))
meshes = []
for package in ('/Game/Character/Body/SK_BaseBody',
                '/Game/Character/Body/SK_BaseBody_Head',
                '/Game/Character/Body/SK_ReferenceMesh_Body'):
    mesh = u.load_asset(package)
    if isinstance(mesh, u.SkeletalMesh):
        skeleton = mesh.get_editor_property('skeleton')
        meshes.append(dict(mesh=package, skeleton=skeleton.get_path_name() if skeleton else None))
report = dict(project=str(actual), engine=u.SystemLibrary.get_engine_version(),
              graph=graph, character_graphs=graphs, assets=assets, dependency_edges=edges,
              external_dependencies=sorted(external), meshes=meshes,
              graph_compiled=False, skeleton_compatibility_tested=False,
              source_packages_saved=False)
output = ROOT / 'local-evidence/m3-mutable-sample-inventory.json'
output.write_text(json.dumps(report, indent=2), encoding='utf-8')
print('MUTABLE_INVENTORY', len(assets), 'assets;', len(external), 'external packages;', output)
