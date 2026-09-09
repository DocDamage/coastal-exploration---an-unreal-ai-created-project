"""Classify imported vendor references and load animation assets without playing them."""
import collections
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
expected = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != expected.resolve():
    raise RuntimeError('Independent UE5.8 host required')
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('Run package audit outside PIE')
report = json.loads((ROOT / 'local-evidence/m3-staging-import-registry.json').read_text())
registry = u.AssetRegistryHelpers.get_asset_registry()
missing = set(report['missing_game_dependencies'])
references, animations, failures = [], [], []
packages = {a['package'] for a in report['registered_assets']}
for kind in ('hard', 'soft'):
    options = u.AssetRegistryDependencyOptions(
        include_soft_package_references=kind == 'soft', include_hard_package_references=kind == 'hard',
        include_searchable_names=False, include_soft_management_references=False,
        include_hard_management_references=False)
    for package in sorted(packages):
        for dep in registry.get_dependencies(package, options):
            if str(dep) in missing:
                references.append(dict(package=package, missing=str(dep), kind=kind))
for row in report['registered_assets']:
    if row['asset_class'] != 'AnimSequence':
        continue
    asset = u.load_asset(row['package'])
    if not isinstance(asset, u.AnimSequence):
        failures.append(row['package'])
        continue
    skeleton = asset.get_editor_property('skeleton')
    duration = asset.get_play_length()
    entry = dict(package=row['package'], duration=duration,
                 skeleton=skeleton.get_path_name() if skeleton else None,
                 enable_root_motion=asset.get_editor_property('enable_root_motion'))
    animations.append(entry)
    if not skeleton or duration <= 0:
        failures.append(row['package'])
unregistered = []
for package in report['missing_packages']:
    asset = u.load_asset(package)
    unregistered.append(dict(package=package, loaded=asset.get_path_name() if asset else None))
audit_report = dict(references=references, animations=animations, animation_failures=failures,
              unregistered=unregistered,
              animation_assets_loadable=not failures,
              animation_counts=dict(collections.Counter(a['package'].split('/')[2] for a in animations)),
              retargeted=False, played=False)
(ROOT / 'local-evidence/m3-staging-animation-audit.json').write_text(json.dumps(audit_report, indent=2), encoding='utf-8')
print(json.dumps({k:v for k,v in audit_report.items() if k not in ('animations','references')}))
