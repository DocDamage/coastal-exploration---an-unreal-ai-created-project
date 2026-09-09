"""End activity PIE and restore the captured transient editor performance setting."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
expected = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != expected.resolve():
    raise RuntimeError('Independent host required')
u.get_editor_subsystem(u.LevelEditorSubsystem).editor_request_end_play()
path = ROOT / 'local-evidence/m3-activities-editor-settings.json'
if path.exists():
    settings = u.load_object(None,'/Script/UnrealEd.Default__EditorPerformanceSettings')
    settings.set_editor_property('bThrottleCPUWhenNotForeground',json.loads(path.read_text())['throttle_cpu_when_not_foreground'])
print('PIE end requested; background setting restored')
