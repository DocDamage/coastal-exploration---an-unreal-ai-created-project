"""Read-only campaign load and gameplay-stack smoke check in the isolated 5.8 host."""
import hashlib
import json
import time
from pathlib import Path
import unreal as u

trial = Path('F:/coastline/LocalHost58/CoastalExploration').resolve()
if Path(u.Paths.get_project_file_path()).resolve().parent != trial:
    raise RuntimeError('This migration check is restricted to the independent 5.8 trial')
save_set = str(globals().get('save_set', 'coastal_test_m3_swimming_0908a'))
if not save_set.startswith('coastal_test_') or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789_' for c in save_set):
    raise RuntimeError('Use an existing disposable test campaign')
out = Path('F:/coastline/local-evidence/m3-ue58-baseline-live.json')
save_dir = trial / 'Saved/SaveGames'
def hashes():
    return {str(p.relative_to(save_dir)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in save_dir.glob('*.sav')}
before = hashes()
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
coordinators = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name()]
if not world or len(coordinators) != 1 or coordinators[0].has_active_campaign():
    raise RuntimeError('Start fresh PIE at the campaign screen first')
saves = coordinators[0]
loaded = saves.load_campaign(u.Name(save_set))
if loaded not in (u.CoastalSaveResult.LOADED, u.CoastalSaveResult.RECOVERED_PREVIOUS):
    raise RuntimeError('Existing campaign load failed: ' + str(loaded) + ' ' + saves.last_detail)
generation = saves.get_generation()
started = time.monotonic()
out.write_text(json.dumps({'passed': False, 'status': 'running', 'save_set': save_set}))
def tick(delta):
    if time.monotonic() - started < 2.0:
        return
    u.unregister_slate_post_tick_callback(handle)
    report = {'passed': False, 'save_set': save_set, 'load_result': str(loaded),
              'engine': u.SystemLibrary.get_engine_version(), 'project': str(trial),
              'generation': generation, 'scope': 'Scripted PIE baseline, copied old save, no save write requested'}
    try:
        pawn = u.GameplayStatics.get_player_character(world, 0)
        controller = u.GameplayStatics.get_player_controller(world, 0)
        ui = controller.get_component_by_class(u.CoastalUISessionComponent)
        movement = pawn.get_component_by_class(u.CharacterMovementComponent)
        components = {name: bool(pawn.get_component_by_class(getattr(u, name))) for name in
                      ['CoastalSwimmingComponent', 'CoastalCampingActionComponent', 'CoastalPlayerRecoveryComponent']}
        report.update(player_class=pawn.get_class().get_path_name(),
                      mesh=pawn.mesh.get_skeletal_mesh_asset().get_path_name(),
                      animation_class=pawn.mesh.get_anim_instance().get_class().get_path_name(),
                      movement_mode=str(movement.movement_mode), components=components,
                      ui_initialized=bool(ui and ui.is_initialized()),
                      journal=[str(x) for x in saves.get_journal()],
                      configured=saves.is_configured(), recovery_required=saves.is_recovery_required(),
                      generation_unchanged=saves.get_generation() == generation,
                      trial_save_hashes_unchanged=before == hashes())
        if (not all(components.values()) or not report['ui_initialized'] or not report['configured']
                or report['recovery_required'] or not report['generation_unchanged']
                or not report['trial_save_hashes_unchanged']):
            raise RuntimeError('One or more gameplay-stack or read-only load checks failed')
        report['passed'] = True
    except Exception as exc:
        report['error'] = str(exc)
    out.write_text(json.dumps(report, indent=2), encoding='utf-8')
handle = u.register_slate_post_tick_callback(tick)
print('UE5.8 baseline gameplay-stack and copied-save check queued')
