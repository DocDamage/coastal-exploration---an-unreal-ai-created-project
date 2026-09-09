"""Run native migration checks only after the independent copy and build succeed."""
import datetime
import hashlib
import json
import subprocess
from pathlib import Path
from ue58_paths import EVIDENCE, TRIAL, PROJECT, build_environment, engine_root, validate_trial

validate_trial()
out = EVIDENCE
copied = json.loads((out / 'm3-ue58-trial-copy.json').read_text())
built = json.loads((out / 'm3-ue58-editor-build.execution.json').read_text())
if not copied.get('content_copy_complete') or built['exit_code'] != 0:
    raise SystemExit('Complete independent content copy and native build first')
env = build_environment()
report_dir = out / ('m3-ue58-native-report-' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ'))
cmd = [str(engine_root() / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'),
       str(PROJECT),
       '/Engine/Maps/Entry', '-NullRHI', '-Unattended', '-NoSound', '-NoSplash',
       '-DDC=InstalledNoZenLocalFallback', '-stdout', '-FullStdOutLogOutput',
       '-ExecCmds=Automation RunTests Coastal', '-TestExit=Automation Test Queue Empty',
       '-ReportExportPath=' + str(report_dir)]
record = {'command': cmd, 'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()}
with (out / 'm3-ue58-native-tests.log').open('w', encoding='utf-8') as log:
    completed = subprocess.run(cmd, cwd=str(TRIAL), env=env, stdout=log, stderr=subprocess.STDOUT)
# Resolve the retained rollback hashes after a workspace relocation.
original = {}
for old_path, digest in copied['original_hashes'].items():
    suffix = old_path.replace(chr(92), '/').split('/coastline/', 1)[-1]
    relocated = TRIAL.parents[1] / suffix
    if not relocated.resolve().is_relative_to((TRIAL.parents[1] / 'LocalHost').resolve()):
        raise RuntimeError('Unexpected original preservation path: ' + old_path)
    original[relocated] = digest
unchanged = all(p.exists() and hashlib.sha256(p.read_bytes()).hexdigest() == digest
                for p, digest in original.items())
record.update(exit_code=completed.returncode, original_critical_files_unchanged=unchanged,
              report_directory=str(report_dir), report_generated=(report_dir / 'index.json').exists(),
              ended_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
(out / 'm3-ue58-native-tests.execution.json').write_text(json.dumps(record, indent=2))
print(json.dumps(record))
raise SystemExit(completed.returncode if unchanged else 1)
