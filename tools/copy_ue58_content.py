"""Resume only the independently owned trial content using native Windows copying."""
import json
import os
import subprocess
from pathlib import Path

root = Path('F:/coastline')
source = root / 'LocalHost/CoastalExploration/Content'
target = root / 'LocalHost58/CoastalExploration/Content'
record_path = root / 'local-evidence/m3-ue58-trial-copy.json'
record = json.loads(record_path.read_text())
if Path(record['target']).resolve() != target.parent.resolve() or target.is_junction() or target.is_symlink():
    raise SystemExit('Expected independently owned trial content directory')
record['content_copy_complete'] = False
record_path.write_text(json.dumps(record, indent=2))
flags = ['/E', '/COPY:DAT', '/DCOPY:DAT', '/R:1', '/W:1', '/MT:2', '/J',
         '/XJ', '/XD', '__pycache__', '/XF', '*.pyc', '/NJH', '/NJS', '/NFL', '/NDL', '/NP']
pairs = [(source, target)]
for folder, directories, _ in os.walk(source):
    for name in directories:
        link = Path(folder) / name
        if link.is_junction() or link.is_symlink():
            resolved = link.resolve()
            if not any(resolved.is_relative_to(allowed) for allowed in (source.resolve(), (root / 'LocalVendor').resolve())):
                raise SystemExit('Review linked content outside the owned workspace: ' + str(link))
            destination = target / link.relative_to(source)
            if destination.is_junction() or destination.is_symlink():
                raise SystemExit('Trial content must be independent: ' + str(destination))
            pairs.append((resolved, destination))
codes = []
with (root / 'local-evidence/m3-ue58-content-copy.log').open('w') as log:
    for origin, destination in pairs:
        result = subprocess.run(['robocopy', str(origin), str(destination)] + flags,
                                stdout=log, stderr=subprocess.STDOUT)
        codes.append(result.returncode)
        if result.returncode >= 8:
            break
missing, wrong_size, expected = [], [], 0
for folder, _, files in os.walk(source):
    for name in files:
        original = Path(folder) / name
        relative = original.relative_to(source)
        if '__pycache__' in relative.parts or original.suffix == '.pyc':
            continue
        expected += 1
        copied = target / relative
        if not copied.is_file():
            missing.append(str(relative))
        elif copied.stat().st_size != original.stat().st_size:
            wrong_size.append(str(relative))
record['content_copy_complete'] = all(c < 8 for c in codes) and not missing and not wrong_size
record['robocopy_exit_codes'] = codes
record['expected_content_files'] = expected
record['missing_content_files'] = missing
record['wrong_size_content_files'] = wrong_size
record['materialized_content_links'] = [str(b) for _, b in pairs[1:]]
record_path.write_text(json.dumps(record, indent=2))
print(json.dumps({'content_copy_complete': record['content_copy_complete'],
                  'robocopy_exit_codes': codes, 'expected_content_files': expected,
                  'missing': len(missing), 'wrong_size': len(wrong_size)}))
raise SystemExit(0 if record['content_copy_complete'] else 1)
