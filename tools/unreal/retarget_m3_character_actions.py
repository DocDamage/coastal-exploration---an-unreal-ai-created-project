"""Retarget useful supplied actions into owned, in-place presentation clips."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
base = '/Game/Coastal/Character'
definition = u.load_asset(base + '/DA_CoastalCharacter')
retarget = u.load_asset(base + '/Rigs/RTG_PlayerToMutable')
target = u.load_asset('/Game/Character/Body/SK_BaseBody')
sources = [
    ('pickup', '/Game/CampingAnimations/Animations/AS_Collects',
     '/Game/CampingAnimations/Demo/Mannequins/Meshes/SKM_Quinn_Simple', 0.5, 2.8),
    ('player_hit', '/Game/MotoInteractionAnims/Animations/Get_Hits/AS_Get_Hit_Front',
     '/Game/MotoInteractionAnims/Demo/Characters/Mannequins/Meshes/SKM_Manny', 0.0, 1.2),
]
actions, report = {}, []
for cue, source_path, mesh_path, start, duration in sources:
    clip, source_mesh = u.load_asset(source_path), u.load_asset(mesh_path)
    if not clip or not source_mesh:
        raise RuntimeError('Missing inspected action source: ' + cue)
    output = base + '/Animations/CA_' + clip.get_name()
    new = u.load_asset(output) if u.EditorAssetLibrary.does_asset_exist(output) else None
    if not new:
        inputs = u.IKRetargetBatchOperationInputs()
        inputs.set_editor_property('assets_to_retarget', [u.EditorAssetLibrary.find_asset_data(source_path)])
        inputs.set_editor_property('source_mesh', source_mesh)
        inputs.set_editor_property('target_mesh', target)
        inputs.set_editor_property('ik_retarget_asset', retarget)
        inputs.set_editor_property('prefix', 'CA_')
        inputs.set_editor_property('target_path', base + '/Animations')
        inputs.set_editor_property('include_referenced_assets', False)
        inputs.set_editor_property('overwrite_existing_files', False)
        result = u.IKRetargetBatchOperation.run_batch_retarget(inputs)
        if len(result) != 1:
            raise RuntimeError('Retarget did not produce exactly one action: ' + cue)
        new = result[0].get_asset()
    new.set_editor_property('enable_root_motion', False)
    new.set_editor_property('force_root_lock', True)
    # These are presentation clips on two meshes; source notifies must not execute twice.
    u.AnimationLibrary.remove_all_animation_notify_tracks(new)
    u.EditorAssetLibrary.save_loaded_asset(new)
    action = u.CoastalCharacterAction()
    action.set_editor_property('clip', new)
    action.set_editor_property('start_seconds', start)
    action.set_editor_property('duration_seconds', duration)
    actions[cue] = action
    report.append(dict(cue=cue, source=source_path, source_skeleton=clip.get_editor_property('skeleton').get_path_name(),
        output=new.get_path_name(), skeleton=new.get_editor_property('skeleton').get_path_name(), length=new.get_play_length(),
        start=start, maximum_duration=duration, root_motion=False, force_root_lock=True, notifies_removed=True))
definition.set_editor_property('actions', actions)
u.EditorAssetLibrary.save_loaded_asset(definition)
(ROOT / 'local-evidence/m3-character-action-retarget.json').write_text(json.dumps(report, indent=2))
print('CHARACTER_ACTIONS', report)
