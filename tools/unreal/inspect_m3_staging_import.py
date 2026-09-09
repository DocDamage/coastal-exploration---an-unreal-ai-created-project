"""Validate newly migrated package registration and collect animation metadata.

Run via identity-checked editor MCP outside PIE. Does not load vendor maps,
create gameplay owners, save packages, or alter the active map.
"""
import collections
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
expected = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
actual = Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve()
if actual != expected.resolve() or u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('Requires independent host outside PIE')
copy = json.loads((ROOT / 'local-evidence/m3-staging-additions-copy.json').read_text())
if not copy.get('complete'):
    raise RuntimeError('Finish hash-verified copy before inspecting imported packages')
registry = u.AssetRegistryHelpers.get_asset_registry()
roots = sorted({'/Game/' + row['path'].split('/')[0] for row in copy['copied']})
registry.scan_paths_synchronous(roots, True)
rows, missing, missing_dependencies, counts = [], [], set(), collections.Counter()
dependency_options = u.AssetRegistryDependencyOptions(
    include_soft_package_references=True, include_hard_package_references=True,
    include_searchable_names=False, include_soft_management_references=False,
    include_hard_management_references=False)
for row in copy['copied']:
    relative = Path(row['path'])
    if relative.suffix not in ('.uasset', '.umap'):
        continue
    package = '/Game/' + relative.with_suffix('').as_posix()
    data = registry.get_assets_by_package_name(package)
    # World Partition external object packages need not expose a top-level
    # browsable asset. Their physical presence is covered by copy verification.
    if not data and not relative.parts[0].startswith('__External'):
        missing.append(package)
    for asset in data:
        kind = str(asset.asset_class_path.asset_name)
        counts[kind] += 1
        entry = dict(package=package, asset_class=kind)
        if kind == 'AnimSequence':
            entry['skeleton_tag'] = str(u.AssetRegistryHelpers.get_tag_value(asset, 'Skeleton'))
        rows.append(entry)
    for dep in registry.get_dependencies(package, dependency_options):
        name = str(dep)
        if name.startswith('/Game/'):
            path = expected.parent / 'Content' / name.removeprefix('/Game/')
            if not path.with_suffix('.uasset').exists() and not path.with_suffix('.umap').exists():
                missing_dependencies.add(name)
report = dict(project=str(actual), registered_assets=rows, class_counts=dict(counts),
              missing_packages=missing, missing_game_dependencies=sorted(missing_dependencies),
              passed=not missing and not missing_dependencies,
              gameplay_integrated=False, animations_retargeted=False)
out = ROOT / 'local-evidence/m3-staging-import-registry.json'
out.write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps({k:v for k,v in report.items() if k != 'registered_assets'}))
