"""Build only the independent migration trial, with workspace-local scratch."""
import datetime
import json
import subprocess

from ue58_paths import EVIDENCE, PROJECT, build_environment, engine_root, validate_trial

validate_trial()
out = EVIDENCE
project = PROJECT
copy_record = out / 'm3-ue58-trial-copy.json'
if not copy_record.is_file() or not json.loads(copy_record.read_text()).get('content_copy_complete'):
    raise SystemExit('Independent trial copy must complete first')
env = build_environment()
cmd = [str(engine_root() / 'Engine/Build/BatchFiles/Build.bat'),
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
