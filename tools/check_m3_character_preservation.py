"""Verify every pre-integration save and both host maps; allow only the planned host descriptor edit."""
import hashlib
import json
from pathlib import Path
from ue58_paths import EVIDENCE, PROJECT

baseline = json.loads((EVIDENCE / 'm3-character-preservation-baseline.json').read_text())
rows = []
for name, expected in baseline.items():
    path = Path(name)
    actual = hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None
    planned_descriptor = path.resolve() == PROJECT.resolve()
    rows.append(dict(path=str(path), unchanged=actual == expected, planned_descriptor_change=planned_descriptor))
unexpected = [r for r in rows if not r['unchanged'] and not r['planned_descriptor_change']]
report = dict(passed=not unexpected, baseline_files=len(rows),
              preexisting_save_files=sum(Path(r['path']).suffix == '.sav' for r in rows),
              unchanged=sum(r['unchanged'] for r in rows), unexpected_changes=unexpected,
              planned_changes=[r for r in rows if r['planned_descriptor_change']], rows=rows)
(EVIDENCE / 'm3-character-preservation.json').write_text(json.dumps(report, indent=2))
print(json.dumps({k:v for k,v in report.items() if k != 'rows'}))
raise SystemExit(0 if report['passed'] else 1)
