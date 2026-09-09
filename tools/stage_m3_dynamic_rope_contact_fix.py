"""Keep the paid plugin private; fix pierce-eye clearance in the independent host only."""
import hashlib
import json
import shutil
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
source=Path(r'D:\Unreal\UE_5.8\Engine\Plugins\Marketplace\DynamicRa1b70179ea1eV4')
target=ROOT/'LocalHost58/CoastalExploration/Plugins/DynamicRope'
relative=Path('Source/DynamicRope/Private/RopeComponentThrow.cpp')
expected='8e5f57922ff90cc9963c73251ca596da22c79ff1b792a2d3c4a121c35ad770fe'
sha=lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
if sha(source/relative)!=expected:
    raise RuntimeError('Installed source version changed; inspect before applying this fix')
if target.exists():
    raise RuntimeError('Private plugin already exists; inspect its report and build first')
manifest={str(p.relative_to(source)):sha(p) for p in source.rglob('*') if p.is_file()}
shutil.copytree(source,target,ignore=shutil.ignore_patterns('Binaries','Intermediate'))
path=target/relative
data=path.read_bytes()
needle=b'Anchor.LocalSurfacePosition = BoneXform.InverseTransformPosition(TailWorld);'
if data.count(needle)!=1:
    raise RuntimeError('Expected one socket-tail commit site')
replacement=needle+b'\r\n\t\t\t// Socket placement already specifies the rope centerline at the eye.\r\n\t\t\tAnchor.SurfaceOffset = 0.0f;'
path.write_bytes(data.replace(needle,replacement))
unchanged=all(sha(source/name)==digest for name,digest in manifest.items())
if not unchanged:
    raise RuntimeError('Engine plugin changed unexpectedly')
report=dict(source=str(source),target=str(target),engine_unchanged=unchanged,
    changed_file=str(relative),source_sha256=expected,patched_sha256=sha(path),
    engine_manifest=manifest)
output=ROOT/'local-evidence/m3-dynamic-rope-contact-fix.json'
output.write_text(json.dumps(report,indent=2))
print(json.dumps({k:v for k,v in report.items() if k!='engine_manifest'}))
