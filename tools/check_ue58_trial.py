"""Run native migration checks only after the independent copy and build succeed."""
import datetime
import hashlib
import json
import os
import subprocess
from pathlib import Path

out = Path('F:/coastline/local-evidence')
copied = json.loads((out / 'm3-ue58-trial-copy.json').read_text())
built = json.loads((out / 'm3-ue58-editor-build.execution.json').read_text())
if not copied.get('content_copy_complete') or built['exit_code'] != 0:
    raise SystemExit('Complete independent content copy and native build first')
scratch = Path('F:/coastline/Cache/UE58/Temp')
scratch.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
env.update(TEMP=str(scratch), TMP=str(scratch))
env['UE-LocalDataCachePath'] = 'F:/coastline/Cache/UE58/DDC'
report_dir = out / ('m3-ue58-native-report-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ'))
cmd = ['C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe',
       str(Path(copied['target']) / 'CoastalExploration.uproject'),
       '/Engine/Maps/Entry', '-NullRHI', '-Unattended', '-NoSound', '-NoSplash',
       '-DDC=InstalledNoZenLocalFallback', '-stdout', '-FullStdOutLogOutput',
       '-ExecCmds=Automation RunTests Coastal', '-TestExit=Automation Test Queue Empty',
       '-ReportExportPath=' + str(report_dir)]
record = {'command': cmd, 'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()}
with (out / 'm3-ue58-native-tests.log').open('w', encoding='utf-8') as log:
    completed = subprocess.run(cmd, cwd=copied['target'], env=env, stdout=log, stderr=subprocess.STDOUT)
unchanged = all(Path(p).exists() and hashlib.sha256(Path(p).read_bytes()).hexdigest() == digest
                for p, digest in copied['original_hashes'].items())
record.update(exit_code=completed.returncode, original_critical_files_unchanged=unchanged,
              report_directory=str(report_dir), report_generated=(report_dir / 'index.json').exists(),
              ended_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
(out / 'm3-ue58-native-tests.execution.json').write_text(json.dumps(record, indent=2))
print(json.dumps(record))
raise SystemExit(completed.returncode if unchanged else 1)
