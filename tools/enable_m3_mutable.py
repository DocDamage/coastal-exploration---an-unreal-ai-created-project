"""Enable installed sample dependencies in the independent host only, editor closed."""
import json
import subprocess
from ue58_paths import PROJECT, EVIDENCE, engine_root, validate_trial

validate_trial()
processes = subprocess.check_output(['powershell', '-NoProfile', '-Command',
    'Get-CimInstance Win32_Process -Filter "Name=\'UnrealEditor.exe\' OR Name=\'UnrealEditor-Cmd.exe\'" | Select-Object -ExpandProperty CommandLine'], text=True)
if str(PROJECT).replace('\\', '/').lower() in processes.replace('\\', '/').lower():
    raise SystemExit('Close the host editor cleanly before enabling dependencies')
copy = json.loads((EVIDENCE / 'm3-mutable-copy.json').read_text())
if not copy['complete']:
    raise SystemExit('Mutable copy must finish before enablement')
names = ['Mutable', 'HairStrandsMutable', 'HairStrands', 'MutableClothing', 'IKRig']
installed = {p.stem: str(p) for p in (engine_root() / 'Engine/Plugins').rglob('*.uplugin') if p.stem in names}
if set(installed) != set(names):
    raise SystemExit('Missing installed dependencies: ' + str(set(names) - set(installed)))
data = json.loads(PROJECT.read_text())
for name in names:
    entry = next((p for p in data['Plugins'] if p['Name'] == name), None)
    if entry is None:
        data['Plugins'].append(dict(Name=name, Enabled=True))
    else:
        entry['Enabled'] = True
PROJECT.write_text(json.dumps(data, indent=2) + '\n')
(EVIDENCE / 'm3-mutable-plugin-enablement.json').write_text(json.dumps(dict(project=str(PROJECT), dependencies=installed), indent=2))
print('Enabled installed Mutable, groom, clothing, and IK retarget dependencies.')
