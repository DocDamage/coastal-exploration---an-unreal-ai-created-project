"""Inspect the installed September 8 packs without changing the current level.

Run with pack='swimming', 'morbid', 'prison' or 'village' in editor Python.
Asset loading is deliberately bounded; a whole vendor showcase is not required.
"""
import json
from pathlib import Path
import unreal as u

ROOTS = {'swimming': 'SwimmingAnimationPack', 'morbid': 'MorbidMotions_Pack',
         'prison': 'HAUNTED_PRISON', 'village': 'ItalianMedievalTown'}
SELECTED = {
    'prison': ['Meshes/Interior/SM_Floor', 'Meshes/Interior/SM_Wall_Door_Prison',
               'Meshes/Interior/SM_Wall_Int', 'Meshes/Interior/SM_PrisonGrid',
               'Meshes/Exterior/Moduls/SM_WallExt_A',
               'Meshes/Exterior/Moduls/SM_Roof', 'Meshes/Assets/SM_Bed'],
    'village': ['Meshes/Walls/SM_Wall_01a', 'Meshes/Roof/SM_Roof_01a',
                'Meshes/Floor/SM_Floor_01a', 'Meshes/Props/SM_Well_01a'],
}
key = globals().get('pack', 'swimming')
root = '/Game/' + ROOTS[key]
registry = u.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous([root], force_rescan=True)
assets = registry.get_assets_by_path(root, recursive=True)
counts = {}
samples = []
for row in assets:
    kind = str(row.asset_class_path.asset_name)
    counts[kind] = counts.get(kind, 0) + 1
    if kind in ('AnimSequence', 'SkeletalMesh', 'Skeleton'):
        samples.append({'path': str(row.package_name), 'class': kind})
meshes = []
for relative in SELECTED.get(key, []):
    path = root + '/' + relative
    sm = u.load_asset(path)
    if not isinstance(sm, u.StaticMesh):
        raise RuntimeError('Expected installed static mesh ' + path)
    bounds = sm.get_bounds()
    body = sm.get_editor_property('body_setup')
    meshes.append({'path': path, 'center': list(bounds.origin.to_tuple()),
                   'size': [v * 2 for v in bounds.box_extent.to_tuple()],
                   'materials': [str(m.material_interface.get_path_name())
                                 if m.material_interface else None
                                 for m in sm.get_editor_property('static_materials')],
                   'collision': str(body.get_editor_property('collision_trace_flag'))
                                if body else None})
report = {'root': root, 'counts': counts, 'animations_and_skeletons': samples,
          'selected_meshes': meshes}
out = Path('F:/coastline/local-evidence') / ('m3-omitted-' + key + '-assets.json')
out.write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps({'root': root, 'counts': counts, 'selected_meshes': meshes,
                  'report': str(out)}))
