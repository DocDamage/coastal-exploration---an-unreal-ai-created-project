"""Copy the two owned staged Content roots into the independent UE5.8 trial."""
import hashlib
import json
import shutil
from pathlib import Path
from ue58_paths import WORKSPACE, TRIAL, EVIDENCE, validate_trial
from extract_expansion_content import _reject_link_ancestors

validate_trial()
source = WORKSPACE / 'M3DestinationStaging57/Content'
report = {'source': str(source), 'target': str(TRIAL / 'Content'), 'assets': {}, 'passed': False}
def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()

for name in ('Atlantis_Ruins', 'ModularSciFiStation'):
    root = source / name
    _reject_link_ancestors(root)
    files = sorted(p for p in root.rglob('*') if p.is_file())
    if not files or not any(p.suffix == '.uasset' for p in files):
        raise RuntimeError('Staged content is missing: ' + name)
    before = {str(p.relative_to(root)): (p.stat().st_size, p.stat().st_mtime_ns) for p in files}
    rows = []
    for file in files:
        _reject_link_ancestors(file)
        target = TRIAL / 'Content' / name / file.relative_to(root)
        _reject_link_ancestors(target)
        target.parent.mkdir(parents=True, exist_ok=True)
        expected = digest(file)
        if target.exists() and digest(target) != expected:
            raise RuntimeError('Refusing to replace changed target content: ' + str(target))
        if not target.exists():
            shutil.copy2(file, target)
        if digest(target) != expected:
            raise RuntimeError('Copied asset hash mismatch: ' + str(target))
        rows.append({'path': str(file.relative_to(root)), 'bytes': file.stat().st_size, 'sha256': expected})
    after = {str(p.relative_to(root)): (p.stat().st_size, p.stat().st_mtime_ns)
             for p in root.rglob('*') if p.is_file()}
    if before != after:
        raise RuntimeError('Staging changed during copy; wait for Launcher then rerun: ' + name)
    report['assets'][name] = rows
    print(name, len(rows), 'files verified', flush=True)
report['passed'] = True
(EVIDENCE / 'm3-staged-destination-content-copy.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('Both staged content roots copied and hash verified.', flush=True)
