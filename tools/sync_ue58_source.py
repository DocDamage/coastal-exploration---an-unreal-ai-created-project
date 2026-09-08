"""Sync reviewed source into the independent trial without touching either host's assets."""
import json
import shutil
from pathlib import Path

repo = Path(__file__).resolve().parents[1]
trial = Path('F:/coastline/LocalHost58/CoastalExploration')
project = json.loads((trial / 'CoastalExploration.uproject').read_text())
if project.get('EngineAssociation') != '5.8':
    raise SystemExit('Expected the isolated UE5.8 project')
pairs = [(repo / 'Plugins' / name / 'Source', trial / 'Plugins' / name / 'Source')
         for name in ('CoastalFoundation', 'CoastalVendorIntegration', 'CoastalExpansion58')]
pairs.append((repo / 'tools/unreal/host_editor/CoastalHostEditor', trial / 'Source/CoastalHostEditor'))
changed = []
for source, destination in pairs:
    if not source.is_dir() or not destination.is_dir() or destination.is_junction() or destination.is_symlink():
        raise SystemExit('Expected independent source directories: ' + str(destination))
    expected = {p.relative_to(source): p for p in source.rglob('*') if p.suffix in ('.cpp', '.h', '.cs')}
    existing = {p.relative_to(destination) for p in destination.rglob('*') if p.suffix in ('.cpp', '.h', '.cs')}
    stale = existing - expected.keys()
    if stale:
        raise SystemExit('Review stale trial source before removal: ' + ', '.join(map(str, stale)))
    for relative, original in expected.items():
        target = destination / relative
        if target.is_symlink() or target.is_junction() or not target.resolve().is_relative_to(trial.resolve()):
            raise SystemExit('Refusing a source path outside the isolated trial')
        if not target.exists() or original.read_bytes() != target.read_bytes():
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(original, target)
            changed.append(str(target.relative_to(trial)))
report = {'trial': str(trial), 'changed': changed, 'count': len(changed), 'build_required': bool(changed)}
(repo.parent / 'local-evidence/m3-ue58-source-sync.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
