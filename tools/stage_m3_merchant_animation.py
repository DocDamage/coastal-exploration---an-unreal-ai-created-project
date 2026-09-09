"""Stage only the selected UE5 merchant FBXs outside Git, without overwriting changed files."""
import hashlib
import json
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
ROOT = REPO.parent
STAGE = ROOT / 'LocalVendor/M3MerchantAnimation'
REPORT = ROOT / 'local-evidence/m3-merchant-animation-staging.json'


def write_once(path, payload):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_bytes() != payload:
        raise RuntimeError('Refusing to overwrite changed private staging: ' + str(path))
    if not path.exists():
        path.write_bytes(payload)


def main():
    spec = json.loads((REPO / 'data/m3_merchant_animation.json').read_text(encoding='utf-8'))
    archive = ROOT / 'even newer assets and animations' / spec['archive']
    rows = []
    with zipfile.ZipFile(archive) as source:
        infos = [i for i in source.infolist() if not i.is_dir()]
        fbx = [i for i in infos if i.filename.lower().endswith('.fbx')]
        docs = [i.filename for i in infos if Path(i.filename).name.lower() in
                ('license', 'license.txt', 'readme', 'readme.txt')]
        if len(fbx) != 54 or docs:
            raise RuntimeError('Merchant archive inventory changed; inspect before staging')
        for item in spec['selected']:
            payload = source.read(item['member'])
            if not payload.startswith(b'Kaydara FBX Binary'):
                raise RuntimeError('Selected payload is not binary FBX: ' + item['member'])
            target = STAGE / (item['id'] + '.fbx')
            write_once(target, payload)
            rows.append(dict(item, source=str(target), size=len(payload),
                             sha256=hashlib.sha256(payload).hexdigest()))
    report = {
        'status': 'selected_ue5_fbx_staged_not_imported',
        'archive': str(archive),
        'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
        'fbx_entries': len(fbx),
        'ue4_fbx_entries': sum('/UE4/' in i.filename for i in fbx),
        'ue5_fbx_entries': sum('/UE5/' in i.filename for i in fbx),
        'license_or_readme_entries': docs,
        'selected': rows
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
