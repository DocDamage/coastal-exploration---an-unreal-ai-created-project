"""Add missing owned staging content; never overwrite an existing host package.

Requires the reviewed private preflight report. Copies packages at their original
/Game paths so references and external actors remain resolvable. This imports
content only; no project config, active map, controller or gameplay wiring changes.
"""
import hashlib
import json
import shutil
from pathlib import Path
from ue58_paths import WORKSPACE, TRIAL, EVIDENCE, validate_trial
from extract_expansion_content import _reject_link_ancestors


def digest(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def main():
    validate_trial()
    source = WORKSPACE / 'M3DestinationStaging57/Content'
    target = TRIAL / 'Content'
    plan = json.loads((EVIDENCE / 'm3-staging-new-content-plan.json').read_text())
    if Path(plan['source']).resolve() != source.resolve() or Path(plan['target']).resolve() != target.resolve():
        raise RuntimeError('Reviewed preflight must identify the current source and independent target')
    _reject_link_ancestors(source)
    _reject_link_ancestors(target)
    snapshot = {}
    for row in plan['rows']:
        path = source / row['path']
        dest = target / row['path']
        _reject_link_ancestors(path)
        _reject_link_ancestors(dest)
        if not path.resolve().is_relative_to(source.resolve()) or not dest.resolve().is_relative_to(target.resolve()):
            raise RuntimeError('Path escapes reviewed content roots')
        stat = path.stat()
        if stat.st_size != row['bytes']:
            raise RuntimeError('Source changed since preflight: ' + row['path'])
        snapshot[row['path']] = (stat.st_size, stat.st_mtime_ns)
    report = dict(source=str(source), target=str(target), complete=False,
                  copied=[], already_present=[], different_preserved=[])
    output = EVIDENCE / 'm3-staging-additions-copy.json'
    for row in plan['rows']:
        path, dest = source / row['path'], target / row['path']
        if row['status'] != 'missing':
            if not dest.is_file():
                raise RuntimeError('An existing host file disappeared: ' + row['path'])
            key = 'different_preserved' if row['status'] == 'different' else 'already_present'
            report[key].append(row['path'])
            continue
        expected = digest(path)
        if dest.exists():
            if digest(dest) != expected:
                raise RuntimeError('Concurrent or changed target: ' + str(dest))
        else:
            dest.parent.mkdir(parents=True, exist_ok=True)
            # Exclusive creation avoids overwriting a concurrently added asset.
            with path.open('rb') as inp, dest.open('xb') as out:
                shutil.copyfileobj(inp, out, 4 * 1024 * 1024)
            shutil.copystat(path, dest)
        if digest(dest) != expected:
            raise RuntimeError('Copied hash mismatch: ' + str(dest))
        report['copied'].append(dict(path=row['path'], bytes=row['bytes'], sha256=expected))
        if len(report['copied']) % 100 == 0:
            output.write_text(json.dumps(report, indent=2), encoding='utf-8')
            print(len(report['copied']), 'new files verified', flush=True)
    after = {p.relative_to(source).as_posix(): (p.stat().st_size, p.stat().st_mtime_ns)
             for p in source.rglob('*') if p.is_file()}
    if after != snapshot:
        raise RuntimeError('Staging changed during migration; inspect the partial copy before continuing')
    report['complete'] = True
    output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('Imported', len(report['copied']), 'files; preserved', len(report['different_preserved']),
          'differing host packages.', flush=True)


if __name__ == '__main__':
    main()
