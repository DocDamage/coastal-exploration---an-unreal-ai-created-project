"""Import only the ten selected private WAVs into the independent UE5.8 host."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
EXPECTED = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != EXPECTED.resolve():
    raise RuntimeError('Expected independent LocalHost58 project')
if not u.SystemLibrary.get_engine_version().startswith('5.8.'):
    raise RuntimeError('Expected Unreal 5.8')
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('Stop PIE before import')
if u.EditorLoadingAndSavingUtils.get_dirty_map_packages() or u.EditorLoadingAndSavingUtils.get_dirty_content_packages():
    raise RuntimeError('Preserve existing unsaved work before import')

manifest = json.loads((ROOT / 'CoastalExploration/data/m3_interaction_audio.json').read_text())
staging = json.loads((ROOT / 'local-evidence/m3-interaction-audio-staging.json').read_text())
destination = manifest['destination']
effects = u.load_asset('/Game/Coastal/Audio/SC_M1Effects')
if not effects:
    raise RuntimeError('Existing Effects class missing')
rows = []
for clip in staging['clips']:
    source = Path(clip['source'])
    if hashlib.sha256(source.read_bytes()).hexdigest() != clip['sha256']:
        raise RuntimeError('Staged WAV hash mismatch')
    name = 'SW_M3_' + clip['id']
    asset_path = destination + '/' + name
    if u.EditorAssetLibrary.does_asset_exist(asset_path):
        raise RuntimeError('Refusing to overwrite existing audio: ' + asset_path)
    task = u.AssetImportTask()
    task.set_editor_property('filename', str(source))
    task.set_editor_property('destination_path', destination)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    task.set_editor_property('replace_existing', False)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = u.load_asset(asset_path)
    if not isinstance(sound, u.SoundWave):
        raise RuntimeError('SoundWave import failed: ' + asset_path)
    sound.set_editor_property('sound_class_object', effects)
    sound.set_editor_property('virtualization_mode', u.VirtualizationMode.PLAY_WHEN_SILENT)
    sound.set_editor_property('looping', False)
    sound.set_editor_property('volume', clip['gain'])
    if not u.EditorAssetLibrary.save_loaded_asset(sound):
        raise RuntimeError('Failed to save ' + asset_path)
    rows.append({'id': clip['id'], 'asset': sound.get_path_name(),
                 'duration': sound.get_editor_property('duration'),
                 'volume': sound.get_editor_property('volume'),
                 'class': sound.get_editor_property('sound_class_object').get_path_name(),
                 'source_sha256': clip['sha256']})

report = {'imported': len(rows), 'clips': rows,
          'gameplay_validation': 'not_run', 'audible_acceptance': 'not_run'}
(ROOT / 'local-evidence/m3-interaction-audio-import.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
