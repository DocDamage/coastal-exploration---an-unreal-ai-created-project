"""Capture existing save/map/source hashes before the activity expansion."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
report = ROOT / 'local-evidence/m3-activities-preservation-baseline.json'
if report.exists():
    raise SystemExit('Existing baseline retained')
paths = []
for host in ('LocalHost', 'LocalHost58'):
    project = ROOT / host / 'CoastalExploration'
    paths.extend((project / 'Saved/SaveGames').glob('*.sav'))
    paths.extend((project / 'Content/Coastal').rglob('*.umap'))
    paths.extend(project.glob('*.uproject'))
source = ROOT / 'CoastalExploration'
paths.extend(p for p in (source / 'Plugins').rglob('*') if p.suffix in ('.h', '.cpp', '.cs', '.uplugin'))
report.write_text(json.dumps({str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2))
print(json.dumps({'baseline': str(report), 'files': len(paths)}))
