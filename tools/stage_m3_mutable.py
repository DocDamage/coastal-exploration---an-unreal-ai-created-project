"""Stage the inspected Mutable dependency closure into the private UE5.8 host.

Existing assets are never overwritten. Original host and pre-existing saves are
recorded separately so subsequent editor acceptance can verify preservation.
"""
import hashlib
import json
import shutil
from pathlib import Path
from ue58_paths import WORKSPACE, TRIAL, PROJECT, EVIDENCE, validate_trial
from extract_expansion_content import _reject_link_ancestors


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def main():
    validate_trial()
    source = WORKSPACE / 'MutableSample/Content'
    inventory = json.loads((EVIDENCE / 'm3-mutable-sample-inventory.json').read_text())
    if Path(inventory['project']).resolve() != (source.parent / 'MutableSample.uproject').resolve():
        raise RuntimeError('Inventory source does not match installed sample')
    baseline = EVIDENCE / 'm3-character-preservation-baseline.json'
    if not baseline.exists():
        paths = list((TRIAL / 'Saved/SaveGames').rglob('*'))
        rollback = WORKSPACE / 'LocalHost/CoastalExploration'
        paths += list((rollback / 'Saved/SaveGames').rglob('*'))
        paths += [PROJECT, rollback / 'CoastalExploration.uproject',
                  TRIAL / 'Content/Coastal/Maps/L_FirstSignal.umap',
                  rollback / 'Content/Coastal/Maps/L_FirstSignal.umap']
        baseline.write_text(json.dumps({str(p): digest(p) for p in paths if p.is_file()}, indent=2))
        shutil.copy2(PROJECT, EVIDENCE / 'm3-character-host-before.uproject')
    files = {}
    for row in inventory['assets']:
        package = row['package']
        if not package.startswith('/Game/'):
            raise RuntimeError('Unexpected package root')
        stem = source / package[len('/Game/'):]
        matches = [p for p in stem.parent.glob(stem.name + '.*') if p.suffix in ('.uasset', '.uexp', '.ubulk')]
        if not matches:
            raise RuntimeError('Missing source package: ' + package)
        for path in matches:
            relative = path.relative_to(source)
            dest = TRIAL / 'Content' / relative
            _reject_link_ancestors(path)
            _reject_link_ancestors(dest)
            if not path.resolve().is_relative_to(source.resolve()) or not dest.resolve().is_relative_to((TRIAL / 'Content').resolve()):
                raise RuntimeError('Path escapes content roots')
            sha = digest(path)
            if dest.exists() and digest(dest) != sha:
                raise RuntimeError('Conflicting host package: ' + str(relative))
            files[relative.as_posix()] = dict(bytes=path.stat().st_size, sha256=sha)
    report = dict(source=str(source), target=str(TRIAL / 'Content'), complete=False, files=files, verified=[])
    output = EVIDENCE / 'm3-mutable-copy.json'
    output.write_text(json.dumps(report, indent=2))
    for relative, row in files.items():
        path, dest = source / relative, TRIAL / 'Content' / relative
        dest.parent.mkdir(parents=True, exist_ok=True)
        if not dest.exists():
            with path.open('rb') as inp, dest.open('xb') as out:
                shutil.copyfileobj(inp, out, 4 * 1024 * 1024)
        if digest(dest) != row['sha256']:
            raise RuntimeError('Copy verification failed: ' + relative)
        report['verified'].append(relative)
        if len(report['verified']) % 100 == 0:
            output.write_text(json.dumps(report, indent=2))
            print(len(report['verified']), 'verified', flush=True)
    report['complete'] = True
    output.write_text(json.dumps(report, indent=2))
    print('Mutable dependency closure staged:', len(files), 'files', sum(x['bytes'] for x in files.values()), 'bytes')


if __name__ == '__main__':
    main()
