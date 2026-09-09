"""Copy the complete ShipAndSea source folder into the private UE5.8 host."""
import hashlib
import json
import shutil
from pathlib import Path
from ue58_paths import TRIAL, EVIDENCE, validate_trial

validate_trial()
source = TRIAL.parents[1] / 'M3DestinationStaging57/Content/ShipAndSea'
destination = TRIAL / 'Content/ShipAndSea'
if not (source / 'Meshes/SM_SpeedBoat.uasset').is_file():
    raise SystemExit('M3 source speedboat is missing')
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
rows = []
for original in sorted(source.rglob('*')):
    if not original.is_file():
        continue
    target = destination / original.relative_to(source)
    if not target.resolve().is_relative_to((TRIAL / 'Content').resolve()):
        raise RuntimeError('Target leaves independent host')
    expected = digest(original)
    if target.exists() and digest(target) != expected:
        raise RuntimeError('Conflicting host asset retained: ' + str(target))
    rows.append(dict(source=str(original), target=str(target), sha256=expected, existing=target.exists()))
for row in rows:
    target = Path(row['target'])
    if not row['existing']:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(row['source'], target)
    if digest(target) != row['sha256']:
        raise RuntimeError('Copied asset verification failed: ' + str(target))
report = dict(files=len(rows), added=sum(not r['existing'] for r in rows), verified=True,
              source=str(source), destination=str(destination), rows=rows)
(EVIDENCE / 'm3-speedboat-content-copy.json').write_text(json.dumps(report, indent=2))
print(json.dumps({k:v for k,v in report.items() if k != 'rows'}))
