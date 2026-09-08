"""Build only the independent migration trial, with process-local scratch on F:."""
import datetime
import json
import os
import subprocess
from pathlib import Path

out = Path('F:/coastline/local-evidence')
project = Path('F:/coastline/LocalHost58/CoastalExploration/CoastalExploration.uproject')
if not (out / 'm3-ue58-trial-copy.json').exists() or not project.exists():
    raise SystemExit('Independent trial copy must complete first')
scratch = Path('F:/coastline/Cache/UE58/Temp')
scratch.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
env.update(TEMP=str(scratch), TMP=str(scratch))
env['UE-LocalDataCachePath'] = 'F:/coastline/Cache/UE58/DDC'
env['UBA_ROOT'] = 'F:/coastline/Cache/UE58/UBA'
cmd = ['C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat',
       'TP_ThirdPersonEditor', 'Win64', 'Development', '-Project=' + str(project),
       '-WaitMutex', '-NoHotReloadFromIDE', '-NoUBA', '-UBANoDetour', '-MaxParallelActions=2']
record = {'command': cmd, 'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
          'scope': 'Independent trial; original UE5.7.4 host unchanged'}
with (out / 'm3-ue58-editor-build.log').open('w', encoding='utf-8') as log:
    completed = subprocess.run(cmd, cwd=str(project.parent), env=env,
                               stdout=log, stderr=subprocess.STDOUT)
record.update(exit_code=completed.returncode,
              ended_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
              log='m3-ue58-editor-build.log')
(out / 'm3-ue58-editor-build.execution.json').write_text(json.dumps(record, indent=2))
print(json.dumps(record))
raise SystemExit(completed.returncode)
