"""Import the selected UE5 Manny merchant mesh and two clips into the independent UE5.8 host."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
EXPECTED = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
DEST = '/Game/Coastal/M3/Merchant/UE5'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != EXPECTED.resolve():
    raise RuntimeError('Expected independent LocalHost58 project')
if not u.SystemLibrary.get_engine_version().startswith('5.8.'):
    raise RuntimeError('Expected Unreal 5.8')
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if level.is_in_play_in_editor():
    raise RuntimeError('Stop PIE before import')
if u.EditorLoadingAndSavingUtils.get_dirty_map_packages() or u.EditorLoadingAndSavingUtils.get_dirty_content_packages():
    raise RuntimeError('Preserve existing unsaved work before import')
if u.EditorAssetLibrary.does_directory_exist(DEST):
    raise RuntimeError('Refusing to merge into an existing merchant destination')

stage = json.loads((ROOT / 'local-evidence/m3-merchant-animation-staging.json').read_text())
selected = {row['id']: row for row in stage['selected']}
for row in selected.values():
    source = Path(row['source'])
    if hashlib.sha256(source.read_bytes()).hexdigest() != row['sha256']:
        raise RuntimeError('Staged FBX hash mismatch: ' + row['id'])


def task(source, destination, name, options):
    job = u.AssetImportTask()
    job.set_editor_property('filename', str(source))
    job.set_editor_property('destination_path', destination)
    job.set_editor_property('destination_name', name)
    job.set_editor_property('automated', True)
    job.set_editor_property('save', False)
    job.set_editor_property('replace_existing', False)
    job.set_editor_property('options', options)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([job])
    if not job.get_editor_property('imported_object_paths'):
        raise RuntimeError('FBX import produced no assets: ' + name)
    return list(job.get_editor_property('imported_object_paths'))


mesh_options = u.FbxImportUI()
mesh_options.set_editor_property('automated_import_should_detect_type', False)
mesh_options.set_editor_property('import_mesh', True)
mesh_options.set_editor_property('import_as_skeletal', True)
mesh_options.set_editor_property('import_animations', False)
mesh_options.set_editor_property('mesh_type_to_import', u.FBXImportType.FBXIT_SKELETAL_MESH)
mesh_paths = task(selected['mesh']['source'], DEST, 'SKM_Manny', mesh_options)
mesh = u.load_asset(DEST + '/SKM_Manny')
if not isinstance(mesh, u.SkeletalMesh) or not mesh.get_editor_property('skeleton'):
    raise RuntimeError('UE5 merchant skeletal mesh import failed')
skeleton = mesh.get_editor_property('skeleton')

animation_rows = []
for key, name in [('idle', 'AS_IdleBartering'), ('pitch', 'AS_PitchBarter_Rt')]:
    options = u.FbxImportUI()
    options.set_editor_property('automated_import_should_detect_type', False)
    options.set_editor_property('import_mesh', False)
    options.set_editor_property('import_as_skeletal', True)
    options.set_editor_property('import_animations', True)
    options.set_editor_property('mesh_type_to_import', u.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property('skeleton', skeleton)
    paths = task(selected[key]['source'], DEST + '/Animation', name, options)
    animation = u.load_asset(DEST + '/Animation/' + name)
    if not isinstance(animation, u.AnimSequence) or animation.get_editor_property('skeleton') != skeleton:
        raise RuntimeError('Animation did not bind the imported UE5 mesh skeleton: ' + name)
    animation.set_editor_property('enable_root_motion', False)
    if animation.get_play_length() <= 0 or not u.EditorAssetLibrary.save_loaded_asset(animation):
        raise RuntimeError('Invalid or unsaved animation: ' + name)
    animation_rows.append({'id': key, 'asset': animation.get_path_name(),
                           'seconds': animation.get_play_length(), 'imported_paths': paths,
                           'source_sha256': selected[key]['sha256']})
if not u.EditorAssetLibrary.save_loaded_asset(mesh) or not u.EditorAssetLibrary.save_loaded_asset(skeleton):
    raise RuntimeError('Failed to save imported merchant mesh/skeleton')

report = {'imported': True, 'mesh': mesh.get_path_name(), 'skeleton': skeleton.get_path_name(),
          'mesh_imported_paths': mesh_paths, 'animations': animation_rows,
          'same_skeleton': True, 'gameplay_validation': 'not_run'}
(ROOT / 'local-evidence/m3-merchant-animation-import.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
