"""Scripted PIE coverage for First Signal water, dry shore and safety limits."""
import hashlib
import json
import time
from pathlib import Path
import unreal as u

REPORT = (Path(__file__).resolve().parents[3] / 'local-evidence/m3-all-waters-live-hardened.json')
CONFIG = json.loads((Path(__file__).resolve().parents[3] / 'local-evidence/m3-all-waters-configuration.json').read_text())
SAVE_DIR = Path(u.Paths.project_saved_dir()) / 'SaveGames'
TEST_SAVE = str(globals().get('save_set', 'coastal_test_m3_swimming_0908a'))
if not TEST_SAVE.startswith('coastal_test_'):
    raise RuntimeError('Use a disposable swimming campaign')


def save_hashes():
    return {str(p.relative_to(SAVE_DIR)).replace('\\', '/'): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(SAVE_DIR.rglob('*')) if p.is_file() and not p.name.startswith('Coastal_' + TEST_SAVE + '_')}


REPORT.write_text(json.dumps({'passed': False, 'status': 'starting'}))
try:
    level = u.get_editor_subsystem(u.LevelEditorSubsystem)
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
    if not level.is_in_play_in_editor() or not world:
        raise RuntimeError('Expected disposable First Signal PIE')
    pawn = u.GameplayStatics.get_player_character(world, 0)
    movement = pawn.get_component_by_class(u.CharacterMovementComponent)
    capsule = pawn.get_component_by_class(u.CapsuleComponent)
    swimming = pawn.get_component_by_class(u.CoastalSwimmingComponent)
    recovery = pawn.get_component_by_class(u.CoastalPlayerRecoveryComponent)
    coordinators = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
                    if 'UEDPIE_' in s.get_path_name() and s.is_configured()]
    if len(coordinators) != 1 or not movement or not capsule or not swimming or not recovery:
        raise RuntimeError('Expected one configured PIE player gameplay stack')
    saves = coordinators[0]
    if not saves.has_active_campaign() and saves.load_campaign(TEST_SAVE) != u.CoastalSaveResult.LOADED:
        raise RuntimeError('Could not load the existing disposable swimming campaign')
    if str(saves.get_active_save_set()) != TEST_SAVE:
        raise RuntimeError('Use only the established disposable swimming campaign')
    before_saves = save_hashes()
    # A rerun may begin while the pawn is still presenting its swim mesh.
    # The authored character defaults define the required dry locomotion pair.
    default_mesh = u.get_default_object(pawn.get_class()).mesh
    original_mesh = default_mesh.get_skeletal_mesh_asset()
    original_anim = default_mesh.get_editor_property('anim_class')
except Exception as exc:
    REPORT.write_text(json.dumps({'passed': False, 'status': 'preflight_failed', 'error': str(exc)}))
    raise


def normal_presentation():
    return (pawn.mesh.get_skeletal_mesh_asset() == original_mesh
            and pawn.mesh.get_anim_instance().get_class() == original_anim)


def volume_label():
    volume = movement.get_physics_volume()
    return volume.get_actor_label() if volume else None


def place(position):
    movement.stop_movement_immediately()
    pawn.consume_movement_input_vector()
    if not pawn.set_actor_location(u.Vector(*position), False, True):
        raise RuntimeError('Test placement failed at ' + str(position))


dry_actors = [a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalSafetyVolume)
              if a.get_editor_property('kind') == u.CoastalSafetyKind.DRY_CHECKPOINT]
if not dry_actors:
    raise RuntimeError('No dry checkpoints were available in PIE')
dry_cases = [{'name': 'checkpoint: ' + a.get_actor_label(),
              'position': list(a.get_actor_location().to_tuple())} for a in dry_actors]
dry_probe = CONFIG['low_shore_probes']['dry_floor']
wet_probe = CONFIG['low_shore_probes']['submerged_floor']
dry_cases.append({'name': 'low shore above water',
                  'position': [dry_probe[0], dry_probe[1], dry_probe[2] + 98]})
water_cases = [
    {'name': 'low shore below water', 'position': [wet_probe[0], wet_probe[1], wet_probe[2] + 98]},
    {'name': 'Powell Dock water', 'position': [25500, 1500, -60]},
    {'name': 'Industrial Harbour water', 'position': [28200, 27000, -60]},
    {'name': 'Sea Platform water', 'position': [40000, -18000, -60]},
    {'name': 'south open sea', 'position': [5000, -30000, -60]},
    {'name': 'west open sea', 'position': [-25000, 0, -60]},
    {'name': 'east open sea', 'position': [40000, 0, -60]},
]
rows = []
phase = 'dry_place'
case_index = 0
deadline = time.monotonic()
drive_start = None
drive_game_start = 0.0
drive_input_frames = 0
overlap_target = None
overlap_labels = []
overlap_frames = []
overlap_transitions = []
overlap_moving_frames = {'overlap_in': 0, 'overlap_out': 0}
last_overlap_position = None
last_overlap_state = None
physics_volume_events = []
volume_delegate = None
volume_delegate_available = False
volume_delegate_error = None
recovery_case = 0
finished = False
began = time.monotonic()


def on_physics_volume_changed(new_volume):
    if phase in ('overlap_in', 'overlap_out'):
        physics_volume_events.append({'leg': phase, 'after_post_tick_frame': len(overlap_frames),
                                      'volume': new_volume.get_actor_label() if new_volume else None,
                                      'seconds': time.monotonic() - began})


try:
    volume_delegate = capsule.get_editor_property('physics_volume_changed_delegate')
    volume_delegate.add_callable(on_physics_volume_changed)
    volume_delegate_available = True
except Exception as exc:
    volume_delegate_error = str(exc)


def finish(error=None):
    global finished
    if finished:
        return
    finished = True
    saves_preserved = False
    try:
        movement.stop_movement_immediately()
        pawn.consume_movement_input_vector()
        if volume_delegate_available:
            volume_delegate.remove_callable(on_physics_volume_changed)
        after_saves = save_hashes()
        saves_preserved = after_saves == before_saves
        if not saves_preserved:
            error = error or 'Campaign save files changed outside the disposable swimming campaign'
    except Exception as exc:
        error = error or str(exc)
    finally:
        u.unregister_slate_post_tick_callback(handle)
    result = {'passed': error is None, 'error': error, 'rows': rows,
              'dry_checkpoint_count': len(dry_actors), 'elapsed_seconds': time.monotonic() - began,
              'other_campaign_save_hashes_preserved': saves_preserved, 'save_set': TEST_SAVE,
              'overlap_frame_counts': overlap_moving_frames,
              'overlap_frames': overlap_frames,
              'overlap_transitions': overlap_transitions,
              'physics_volume_delegate': {'available': volume_delegate_available,
                  'error': volume_delegate_error, 'events': physics_volume_events},
              'scope': ('Scripted PIE CharacterMovement across authored water, shore and limits; '
                        'physical device and Windows package acceptance remain separate.')}
    REPORT.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result))


def assert_swimming(context):
    if (not swimming.is_surface_swimming() or not movement.is_swimming()
            or not swimming.is_recovery_protected() or saves.is_player_return_active()):
        raise RuntimeError(context + ' did not maintain protected native swimming: '
            + json.dumps({'position': list(pawn.get_actor_location().to_tuple()),
                          'surface_swimming': swimming.is_surface_swimming(),
                          'movement_mode': str(movement.movement_mode),
                          'physics_volume': volume_label(),
                          'recovery_active': saves.is_player_return_active(),
                          'detail': swimming.last_detail}))


def capture_overlap_frame(leg):
    global last_overlap_position, last_overlap_state
    position = pawn.get_actor_location()
    velocity = movement.velocity
    state = {'leg': leg, 'position': list(position.to_tuple()),
             'velocity': list(velocity.to_tuple()), 'physics_volume': volume_label(),
             'movement_mode': str(movement.movement_mode),
             'surface_swimming': swimming.is_surface_swimming(),
             'recovery_protected': swimming.is_recovery_protected(),
             'return_active': saves.is_player_return_active(),
             'presentation_mesh': str(pawn.mesh.get_skeletal_mesh_asset()),
             'detail': swimming.last_detail}
    overlap_frames.append(state)
    transition_key = (state['physics_volume'], state['movement_mode'], state['surface_swimming'],
                      state['recovery_protected'], state['return_active'], state['presentation_mesh'])
    if transition_key != last_overlap_state:
        overlap_transitions.append({'frame': len(overlap_frames) - 1, **state})
        last_overlap_state = transition_key
    if last_overlap_position and (position - last_overlap_position).length() > .05:
        overlap_moving_frames[leg] += 1
    last_overlap_position = position


def tick(delta):
    global phase, case_index, deadline, drive_start, overlap_target, recovery_case
    global drive_game_start, drive_input_frames
    global last_overlap_position, last_overlap_state
    try:
        now = u.GameplayStatics.get_time_seconds(world)
        if time.monotonic() - began > 300:
            raise RuntimeError('Water verification exceeded its wall-clock safety limit')
        if not level.is_in_play_in_editor():
            raise RuntimeError('PIE ended during all-water checks')
        if phase == 'dry_place':
            place(dry_cases[case_index]['position'])
            phase, deadline = 'dry_check', now + .8
        elif phase == 'dry_check' and now >= deadline:
            case = dry_cases[case_index]
            if (swimming.is_surface_swimming() or movement.is_swimming() or not normal_presentation()
                    or not u.CoastalPlacementLibrary.is_dry_destination(pawn, pawn.get_actor_transform())):
                raise RuntimeError(case['name'] + ' incorrectly entered swimming')
            rows.append({'case': case['name'], 'passed': True,
                         'position': list(pawn.get_actor_location().to_tuple())})
            case_index += 1
            if case_index < len(dry_cases):
                phase = 'dry_place'
            else:
                case_index, phase = 0, 'water_place'
        elif phase == 'water_place':
            place(water_cases[case_index]['position'])
            phase, deadline = 'water_check', now + 1.5
        elif phase == 'water_check' and now >= deadline:
            case = water_cases[case_index]
            assert_swimming(case['name'])
            drive_start = pawn.get_actor_location()
            drive_game_start = u.GameplayStatics.get_time_seconds(world)
            drive_input_frames = 0
            phase, deadline = 'water_drive', now + 20.0
        elif phase == 'water_drive':
            simulated = u.GameplayStatics.get_time_seconds(world) - drive_game_start
            if now > deadline:
                raise RuntimeError('Water input check did not receive enough simulated frames')
            if simulated < 1.0 or drive_input_frames < 10:
                pawn.add_movement_input(u.Vector(1, 0, 0), 1.0, False)
                drive_input_frames += 1
            else:
                case = water_cases[case_index]
                assert_swimming(case['name'])
                delta = pawn.get_actor_location() - drive_start
                distance = (delta.x ** 2 + delta.y ** 2) ** .5
                if distance < 25:
                    raise RuntimeError(case['name'] + ' did not respond to existing movement input')
                rows.append({'case': case['name'], 'passed': True, 'distance_cm': distance,
                             'input_frames': drive_input_frames, 'simulated_seconds': simulated,
                             'position': list(pawn.get_actor_location().to_tuple()),
                             'physics_volume': volume_label()})
                case_index += 1
                if case_index < len(water_cases):
                    phase = 'water_place'
                else:
                    place((10800, -2700, -60))
                    phase, deadline = 'overlap_settle', now + 1.5
        elif phase == 'overlap_settle' and now >= deadline:
            assert_swimming('global-to-basin route start')
            overlap_labels.clear()
            overlap_frames.clear()
            overlap_transitions.clear()
            physics_volume_events.clear()
            overlap_moving_frames['overlap_in'] = overlap_moving_frames['overlap_out'] = 0
            last_overlap_position = pawn.get_actor_location()
            last_overlap_state = None
            overlap_target = u.Vector(11600, -2700, -60)
            phase, deadline = 'overlap_in', now + 45
        elif phase in ('overlap_in', 'overlap_out'):
            capture_overlap_frame(phase)
            assert_swimming('overlapping-zone handoff')
            overlap_labels.append(volume_label())
            if pawn.mesh.get_skeletal_mesh_asset() == original_mesh:
                raise RuntimeError('Swimming presentation was lost during overlapping-zone handoff')
            direction = u.Vector(overlap_target.x - pawn.get_actor_location().x,
                                 overlap_target.y - pawn.get_actor_location().y, 0)
            if direction.length() < 50:
                if overlap_moving_frames[phase] < 60:
                    raise RuntimeError(phase + ' crossed the seam in fewer than 60 observed moving frames')
                if phase == 'overlap_in':
                    movement.stop_movement_immediately()
                    pawn.consume_movement_input_vector()
                    overlap_target = u.Vector(10800, -2700, -60)
                    last_overlap_position = pawn.get_actor_location()
                    phase, deadline = 'overlap_out', now + 45
                else:
                    labels = set(label for label in overlap_labels if label)
                    if not any('Sheltered basin' in label for label in labels):
                        raise RuntimeError('Higher-priority sheltered zone was not selected')
                    if not any('First Signal coastal waters' in label for label in labels):
                        raise RuntimeError('Lower-priority coastal zone was not restored')
                    default_events = [event for event in physics_volume_events
                                      if event['volume'] and 'DefaultPhysicsVolume' in event['volume']]
                    if volume_delegate_available and default_events:
                        raise RuntimeError('Physics volume oscillated through DefaultPhysicsVolume during the seam')
                    rows.append({'case': 'basin boundary both directions', 'passed': True,
                                 'physics_volumes': sorted(labels),
                                 'moving_frames': dict(overlap_moving_frames),
                                 'intra_frame_volume_delegate_available': volume_delegate_available,
                                 'physics_volume_event_count': len(physics_volume_events)})
                    recovery_case, phase = 0, 'recovery_place'
                return
            if now > deadline:
                raise RuntimeError('Overlapping-zone route timed out')
            pawn.add_movement_input(direction.normal(), .1, False)
        elif phase == 'recovery_place':
            positions = [(-34400, 0, -60), (5000, -41400, -60)]
            place(positions[recovery_case])
            phase, deadline = 'recovery_settle', now + 1.5
        elif phase == 'recovery_settle' and now >= deadline:
            assert_swimming('safety-limit approach')
            phase, deadline = 'recovery_drive', now + 15
        elif phase == 'recovery_drive':
            if saves.is_player_return_active():
                movement.stop_movement_immediately()
                pawn.consume_movement_input_vector()
                phase = 'recovery_wait'
            else:
                if now > deadline:
                    raise RuntimeError('Safety-limit recovery did not begin')
                direction = u.Vector(-1, 0, 0) if recovery_case == 0 else u.Vector(0, -1, 0)
                pawn.add_movement_input(direction, 1.0, False)
        elif phase == 'recovery_wait':
            if saves.is_player_return_active():
                return
            if (swimming.is_surface_swimming() or movement.is_swimming()
                    or not normal_presentation()
                    or not u.CoastalPlacementLibrary.is_dry_destination(pawn, pawn.get_actor_transform())):
                raise RuntimeError('Safety recovery did not restore dry locomotion')
            names = ['west OutOfBounds limit', 'south DeepWater transition']
            rows.append({'case': names[recovery_case], 'passed': True,
                         'position': list(pawn.get_actor_location().to_tuple()),
                         'recovery_detail': recovery.last_detail})
            recovery_case += 1
            if recovery_case < 2:
                phase = 'recovery_place'
            else:
                finish()
    except Exception as exc:
        finish(str(exc))


handle = u.register_slate_post_tick_callback(tick)
print('Queued all-water, dry-shore, overlap and safety-limit swimming checks')
