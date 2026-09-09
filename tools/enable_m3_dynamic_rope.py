"""Enable the installed Dynamic Rope dependency only in the independent UE5.8 host."""
import hashlib
import json
import shutil
from pathlib import Path
from ue58_paths import REPO, TRIAL, PROJECT, EVIDENCE, engine_root, validate_trial

validate_trial()
plugins=list((engine_root()/'Engine/Plugins/Marketplace').glob('*/DynamicRope.uplugin'))
if len(plugins)!=1:
    raise SystemExit('Expected exactly one installed Dynamic Rope plugin')
metadata=json.loads(plugins[0].read_text(encoding='utf-8-sig'))
if not any(m['Name']=='DynamicRope' for m in metadata['Modules']):
    raise SystemExit('Runtime module unavailable')
backup=EVIDENCE/'m3-activities-asset-backups/CoastalExploration-before-dynamic-rope.uproject'
if not backup.exists():
    shutil.copy2(PROJECT,backup)
project=json.loads(PROJECT.read_text())
entry=next((p for p in project['Plugins'] if p['Name']=='DynamicRope'),None)
if entry is None:
    project['Plugins'].append(dict(Name='DynamicRope',Enabled=True))
else:
    entry['Enabled']=True
PROJECT.write_text(json.dumps(project,indent=2)+'\n')
source=REPO/'Plugins/CoastalExpansion58/CoastalExpansion58.uplugin'
destination=TRIAL/'Plugins/CoastalExpansion58/CoastalExpansion58.uplugin'
shutil.copy2(source,destination)
report=dict(plugin=str(plugins[0]),version=metadata.get('VersionName'),project=str(PROJECT),backup=str(backup),
    before_sha256=hashlib.sha256(backup.read_bytes()).hexdigest(),after_sha256=hashlib.sha256(PROJECT.read_bytes()).hexdigest())
(EVIDENCE/'m3-dynamic-rope-enable.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
