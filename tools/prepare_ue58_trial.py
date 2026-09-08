"""Create an independent UE5.8 trial; never convert or link the working host in place."""
import hashlib
import json
import shutil
import sys
import time
from pathlib import Path

SOURCE = Path('F:/coastline/LocalHost/CoastalExploration')
TARGET = Path('F:/coastline/LocalHost58/CoastalExploration')
ENGINE = Path('C:/Program Files/Epic Games/UE_5.8')
EVIDENCE = Path('F:/coastline/local-evidence')
REPO = Path(__file__).resolve().parents[1]
DAY_NIGHT = Path('C:/Program Files/Epic Games/UE_5.7/Engine/Plugins/Marketplace/Lightweifafe9bd6710bV1')

resume_native = '--resume-native' in sys.argv
if TARGET.exists() and not resume_native:
    raise SystemExit('Trial target already exists; refusing to overwrite any work')
version = json.loads((ENGINE / 'Engine/Build/Build.version').read_text())
if version['MinorVersion'] != 8 or not (ENGINE / 'Engine/Binaries/Win64/UnrealEditor.exe').exists():
    raise SystemExit('Expected installed Unreal Engine 5.8')
if shutil.disk_usage(TARGET.parent.parent).free < 70 * 1024 ** 3:
    raise SystemExit('Require at least 70 GiB free before independent copy and build')

started = time.time()
baseline = {}
critical = [SOURCE / 'Content/Coastal/Maps/L_FirstSignal.umap',
            SOURCE / 'Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset',
            SOURCE / 'CoastalExploration.uproject']
critical += sorted((SOURCE / 'Saved/SaveGames').glob('*'))
critical += [SOURCE / 'Saved/Config' / platform / 'GameUserSettings.ini'
             for platform in ('Windows', 'WindowsEditor')]
for path in critical:
    if path.is_file():
        baseline[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
TARGET.mkdir(parents=True, exist_ok=resume_native)
ignore = shutil.ignore_patterns('__pycache__', '*.pyc')
for name in (('Config', 'Source', 'Build') if resume_native else ('Config', 'Source', 'Build', 'Content')):
    src = SOURCE / name
    if src.exists():
        shutil.copytree(src, TARGET / name, ignore=ignore, dirs_exist_ok=resume_native)
save_dir = SOURCE / 'Saved/SaveGames'
if save_dir.exists():
    shutil.copytree(save_dir, TARGET / 'Saved/SaveGames')
for platform in ('Windows', 'WindowsEditor'):
    settings = SOURCE / 'Saved/Config' / platform / 'GameUserSettings.ini'
    if settings.exists():
        destination = TARGET / 'Saved/Config' / platform / settings.name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(settings, destination)
for name in ('CoastalFoundation', 'CoastalVendorIntegration'):
    src = REPO / 'Plugins' / name
    dst = TARGET / 'Plugins' / name
    dst.mkdir(parents=True)
    shutil.copy2(src / (name + '.uplugin'), dst)
    for folder in ('Source', 'Content', 'Config', 'Resources'):
        if (src / folder).exists():
            shutil.copytree(src / folder, dst / folder, ignore=ignore)
# Recompile the installed optional plugin from its source in the trial only.
dst = TARGET / 'Plugins/DayNightSystem'
dst.mkdir(parents=True)
for folder in ('Source', 'Content', 'Config', 'Resources'):
    if (DAY_NIGHT / folder).exists():
        shutil.copytree(DAY_NIGHT / folder, dst / folder, ignore=ignore)
descriptor = json.loads((DAY_NIGHT / 'DayNightSystem.uplugin').read_text(encoding='utf-8-sig'))
descriptor.pop('EngineVersion', None)
descriptor['Installed'] = False
(dst / 'DayNightSystem.uplugin').write_text(json.dumps(descriptor, indent=2) + '\n')

project = json.loads((SOURCE / 'CoastalExploration.uproject').read_text())
project['EngineAssociation'] = '5.8'
(TARGET / 'CoastalExploration.uproject').write_text(json.dumps(project, indent=2) + '\n')
report = {'source': str(SOURCE), 'target': str(TARGET), 'engine': version,
          'independent_plugin_copies': True, 'content_copy_complete': not resume_native,
          'copied_saves_are_test_copies': True, 'original_hashes': baseline,
          'excluded': ['Binaries', 'Intermediate', 'DerivedDataCache', 'Saved except SaveGames and GameUserSettings'],
          'native_build': 'pending', 'runtime_acceptance': 'pending',
          'elapsed_seconds': time.time() - started}
(EVIDENCE / 'm3-ue58-trial-copy.json').write_text(json.dumps(report, indent=2))
print(json.dumps({k: v for k, v in report.items() if k != 'original_hashes'}))
