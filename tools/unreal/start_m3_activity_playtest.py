"""Start the independent activity PIE check with temporary background ticking."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
expected = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != expected.resolve():
    raise RuntimeError('Independent host required')
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if level.is_in_play_in_editor():
    raise RuntimeError('PIE already running; inspect that session')
settings = u.load_object(None, '/Script/UnrealEd.Default__EditorPerformanceSettings')
report = ROOT / 'local-evidence/m3-activities-editor-settings.json'
if not report.exists():
    report.write_text(json.dumps(dict(throttle_cpu_when_not_foreground=settings.get_editor_property('bThrottleCPUWhenNotForeground'))))
settings.set_editor_property('bThrottleCPUWhenNotForeground',False)
level.editor_request_begin_play()
print('Activity PIE requested')
