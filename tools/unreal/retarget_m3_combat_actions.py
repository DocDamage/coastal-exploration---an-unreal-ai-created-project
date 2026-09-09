"""Retarget the installed pistol and directional hits; preserve prior action bindings."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
expected = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != expected.resolve():
    raise RuntimeError('Independent host required')
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('End PIE before retargeting')
base = '/Game/Coastal/Character'
definition = u.load_asset(base + '/DA_CoastalCharacter')
retarget = u.load_asset(base + '/Rigs/RTG_PlayerToMutable')
target = u.load_asset('/Game/Character/Body/SK_BaseBody')
source_mesh = u.load_asset('/Game/MotoInteractionAnims/Demo/Characters/Mannequins/Meshes/SKM_Manny')
sources = [('pistol_hold', 'Combat/Pistol/AS_Pistol_Hold_Loop'),
           ('pistol_equip', 'Combat/Pistol/AS_Pistol_Equip'),
           ('pistol_unequip', 'Combat/Pistol/AS_Pistol_Unequip')]
for direction in ('Front', 'Back', 'Left', 'Right'):
    sources += [('pistol_aim_' + direction.lower(), 'Combat/Pistol/AS_Pistol_Point_' + direction),
                ('pistol_shoot_' + direction.lower(), 'Combat/Pistol/AS_Pistol_Shoot_' + direction),
                ('player_hit' + ('' if direction == 'Front' else '_' + direction.lower()), 'Get_Hits/AS_Get_Hit_' + direction)]
actions = dict(definition.get_editor_property('actions'))
report = []
for cue, relative in sources:
    path = '/Game/MotoInteractionAnims/Animations/' + relative
    clip = u.load_asset(path)
    if not clip or not source_mesh or not target or not retarget:
        raise RuntimeError('Missing action source: ' + path)
    output = base + '/Animations/CA_' + clip.get_name()
    new = u.load_asset(output) if u.EditorAssetLibrary.does_asset_exist(output) else None
    if not new:
        inputs = u.IKRetargetBatchOperationInputs()
        for key, value in dict(assets_to_retarget=[u.EditorAssetLibrary.find_asset_data(path)],
                source_mesh=source_mesh, target_mesh=target, ik_retarget_asset=retarget,
                prefix='CA_', target_path=base + '/Animations', include_referenced_assets=False,
                overwrite_existing_files=False).items():
            inputs.set_editor_property(key, value)
        results = u.IKRetargetBatchOperation.run_batch_retarget(inputs)
        if len(results) != 1:
            raise RuntimeError('Expected one retarget output: ' + cue)
        new = results[0].get_asset()
    if new.get_editor_property('skeleton') != target.get_editor_property('skeleton'):
        raise RuntimeError('Incompatible retarget skeleton: ' + cue)
    new.set_editor_property('enable_root_motion', False)
    new.set_editor_property('force_root_lock', True)
    u.AnimationLibrary.remove_all_animation_notify_tracks(new)
    if not u.EditorAssetLibrary.save_loaded_asset(new):
        raise RuntimeError('Action save failed: ' + cue)
    action = u.CoastalCharacterAction()
    action.set_editor_property('clip', new)
    action.set_editor_property('duration_seconds', min(5.0, new.get_play_length()))
    upper_body = cue.startswith('pistol_')
    action.set_editor_property('upper_body', upper_body)
    actions[cue] = action
    report.append(dict(cue=cue, source=path, output=new.get_path_name(), length=new.get_play_length(), upper_body=upper_body))
definition.set_editor_property('actions', actions)
if not u.EditorAssetLibrary.save_loaded_asset(definition):
    raise RuntimeError('Action definition save failed')
(ROOT / 'local-evidence/m3-combat-actions-retarget.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
