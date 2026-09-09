"""Run the installed plugin's native pierce-placement tests in the independent host."""
import datetime
import json
import subprocess
from ue58_paths import EVIDENCE, TRIAL, PROJECT, build_environment, engine_root, validate_trial

validate_trial()
stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
report=EVIDENCE/('m3-rope-pierce-native-'+stamp)
cmd=[str(engine_root()/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'),str(PROJECT),
     '/Engine/Maps/Entry','-NullRHI','-Unattended','-NoSound','-NoSplash',
     '-DDC=InstalledNoZenLocalFallback','-stdout','-FullStdOutLogOutput',
     '-ExecCmds=Automation RunTests DynamicRope.Pierce','-TestExit=Automation Test Queue Empty',
     '-ReportExportPath='+str(report)]
with (EVIDENCE/'m3-rope-pierce-native.log').open('w',encoding='utf-8') as log:
    result=subprocess.run(cmd,cwd=TRIAL,env=build_environment(),stdout=log,stderr=subprocess.STDOUT)
summary=json.loads((report/'index.json').read_text(encoding='utf-8-sig')) if (report/'index.json').exists() else {}
record=dict(exit_code=result.returncode,report=str(report),succeeded=summary.get('succeeded',0),
    failed=summary.get('failed',0),passed=result.returncode==0 and summary.get('succeeded',0)>0 and summary.get('failed',1)==0)
(EVIDENCE/'m3-rope-pierce-native.execution.json').write_text(json.dumps(record,indent=2))
print(json.dumps(record))
raise SystemExit(0 if record['passed'] else 1)
