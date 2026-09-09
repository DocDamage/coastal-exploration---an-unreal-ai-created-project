"""Verify the isolated plugin patch and preservation of its engine installation."""
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
report_path=ROOT/'local-evidence/m3-dynamic-rope-contact-fix.json'
report=json.loads(report_path.read_text())
source=Path(report['source'])
target=Path(report['target'])
sha=lambda p: hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
engine_changes=[name for name,digest in report['engine_manifest'].items() if sha(source/name)!=digest]
private_changes=[]
for name,digest in report['engine_manifest'].items():
    relative=Path(name)
    if relative.parts[0] in ('Binaries','Intermediate'):
        continue
    expected=report['patched_sha256'] if name==report['changed_file'] else digest
    if sha(target/name)!=expected:
        private_changes.append(name)
result=dict(passed=not engine_changes and not private_changes,
    engine_files=len(report['engine_manifest']),engine_changes=engine_changes,
    unexpected_private_changes=private_changes,patched_file=report['changed_file'])
(ROOT/'local-evidence/m3-dynamic-rope-contact-preservation.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result))
raise SystemExit(0 if result['passed'] else 1)
