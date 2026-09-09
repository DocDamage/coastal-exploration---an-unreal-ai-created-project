"""Exercise pause-menu wait/follow, movement, nonblocking collision, recovery, and epoch reset in disposable PIE."""
import hashlib
import json
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
REPORT = ROOT / 'local-evidence/m3-coastal-companion-live.json'
SAVE_DIR = Path(u.Paths.project_saved_dir()) / 'SaveGames'
TEST_SAVE = str(globals().get('save_set', ''))
if not TEST_SAVE.startswith('coastal_test_'):
    raise RuntimeError('Pass an active disposable save_set beginning coastal_test_')


def other_save_hashes():
    own = 'Coastal_' + TEST_SAVE + '_'
    return {str(p.relative_to(SAVE_DIR)).replace('\\', '/'): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(SAVE_DIR.rglob('*')) if p.is_file() and not p.name.startswith(own)}


REPORT.write_text(json.dumps({'passed': False, 'status': 'starting'}), encoding='utf-8')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if not world or not level.is_in_play_in_editor():
    raise RuntimeError('Expected disposable First Signal PIE')
pawn = u.GameplayStatics.get_player_character(world, 0)
pc = u.GameplayStatics.get_player_controller(world, 0)
ui = pc.get_component_by_class(u.CoastalUISessionComponent)
recovery = pawn.get_component_by_class(u.CoastalPlayerRecoveryComponent)
saves_found = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
               if 'UEDPIE_' in s.get_path_name() and s.is_configured()]
directors = u.GameplayStatics.get_all_actors_of_class(world, u.CoastalCompanionDirector)
if len(saves_found) != 1 or len(directors) != 1 or not ui or not recovery:
    raise RuntimeError('Expected one configured PIE player and companion director')
saves, director = saves_found[0], directors[0]
if str(saves.get_active_save_set()) != TEST_SAVE:
    raise RuntimeError('Expected the named active disposable campaign')
before_saves = other_save_hashes()

phase = 'initialize'
next_game = 0.0
next_wall = 0.0
began_wall = time.monotonic()
rows, state = [], {}
finished = False


def companion():
    return director.get_companion()


def mode(dog):
    return str(dog.get_editor_property('mode'))


def animation_path(dog):
    instance = dog.mesh.get_anim_instance()
    animation = instance.get_animation_asset() if instance else None
    return animation.get_path_name() if animation else None


def execute_pause_command(command):
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport()]
    if len(panels) != 1:
        raise RuntimeError('Pause panel is ambiguous while executing ' + command)
    panels[0].call_method('Execute', args=(command,))


def finish(error=None):
    global finished
    if finished:
        return
    finished = True
    try:
        if ui.has_modal():
            execute_pause_command('back')
        preserved = other_save_hashes() == before_saves
        if not preserved:
            error = error or 'A campaign outside the disposable namespace changed'
    except Exception as exc:
        preserved = False
        error = error or str(exc)
    u.unregister_slate_post_tick_callback(handle)
    result = {'passed': error is None, 'error': error, 'rows': rows, 'save_set': TEST_SAVE,
              'other_campaign_save_hashes_preserved': preserved,
              'elapsed_seconds': time.monotonic() - began_wall,
              'scope': ('Scripted PIE over the real pause command and CharacterMovement; '
                        'physical-device and packaged acceptance remain separate.')}
    REPORT.write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result))


def tick(_delta):
    global phase, next_game, next_wall
    try:
        wall = time.monotonic()
        game = u.GameplayStatics.get_time_seconds(world)
        if wall - began_wall > 150:
            raise RuntimeError('Companion live check timed out in ' + phase)
        if not level.is_in_play_in_editor():
            raise RuntimeError('PIE ended during companion check')
        dog = companion()
        if phase == 'initialize':
            if not dog or not director.is_initialized() or 'FOLLOWING' not in mode(dog):
                return
            if dog.get_component_by_class(u.CharacterMovementComponent).get_physics_volume() and dog.get_component_by_class(u.CharacterMovementComponent).get_physics_volume().get_editor_property('water_volume'):
                raise RuntimeError('Initial companion regroup selected a water PhysicsVolume')
            state['initial_location'] = dog.get_actor_location()
            ui.open_pause()
            phase, next_wall = 'paint_wait', wall + .35
        elif phase == 'paint_wait' and wall >= next_wall:
            execute_pause_command('companion_toggle')
            if ui.has_modal():
                next_wall = wall + .35
                return
            phase, next_game = 'wait_command', game + .35
        elif phase == 'wait_command' and game >= next_game:
            if ui.has_modal() or dog.is_following_requested() or 'WAITING' not in mode(dog):
                raise RuntimeError('Painted pause command did not enter wait mode')
            rows.append({'case': 'painted_pause_menu_wait_command', 'passed': True})
            state['wait_location'] = dog.get_actor_location()
            pawn.set_actor_location(pawn.get_actor_location() + u.Vector(500, 0, 0), False, True)
            phase, next_game = 'wait_holds', game + 1.0
        elif phase == 'wait_holds' and game >= next_game:
            moved = (dog.get_actor_location() - state['wait_location']).length()
            if moved > 5:
                raise RuntimeError('Wait mode moved {:.2f} cm'.format(moved))
            rows.append({'case': 'wait_holds_position', 'passed': True, 'drift_cm': moved})
            capsule = dog.capsule_component
            if capsule.get_collision_response_to_channel(u.CollisionChannel.ECC_PAWN) != u.CollisionResponseType.ECR_IGNORE:
                raise RuntimeError('Companion capsule does not ignore Pawn collision')
            center = dog.get_actor_location() + u.Vector(
                0, 0, pawn.capsule_component.get_scaled_capsule_half_height()
                - dog.capsule_component.get_scaled_capsule_half_height())
            original = pawn.get_actor_transform()
            pawn.set_actor_location(center - u.Vector(100, 0, 0), False, True)
            if not pawn.set_actor_location(center + u.Vector(100, 0, 0), True, False):
                raise RuntimeError('Player swept movement did not cross companion')
            error = (pawn.get_actor_location() - (center + u.Vector(100, 0, 0))).length()
            pawn.set_actor_transform(original, False, True)
            if error > 5:
                raise RuntimeError('Companion blocked player sweep by {:.2f} cm'.format(error))
            rows.append({'case': 'companion_does_not_block_player_capsule', 'passed': True,
                         'sweep_endpoint_error_cm': error})
            ui.open_pause()
            phase, next_wall = 'paint_follow', wall + .35
        elif phase == 'paint_follow' and wall >= next_wall:
            execute_pause_command('companion_toggle')
            if ui.has_modal():
                next_wall = wall + .35
                return
            state['follow_start'] = dog.get_actor_location()
            state['locomotion_animations'] = set()
            phase, next_game = 'follow_moves', game + 2.0
        elif phase == 'follow_moves':
            path = animation_path(dog)
            if path:
                state['locomotion_animations'].add(path)
            if game < next_game:
                return
            moved = (dog.get_actor_location() - state['follow_start']).length()
            expected = ('A_type1_Walk_Loop_v01', 'A_type1_Run_Loop_v01')
            if not dog.is_following_requested() or moved < 20:
                raise RuntimeError('Follow command did not produce CharacterMovement ({:.2f} cm)'.format(moved))
            if not any(any(name in path for name in expected) for path in state['locomotion_animations']):
                raise RuntimeError('Follow movement did not present a supplied locomotion clip')
            rows.append({'case': 'follow_moves_with_supplied_animation', 'passed': True,
                         'distance_cm': moved, 'animations': sorted(state['locomotion_animations'])})
            dog.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            if not dog.set_actor_location(u.Vector(-25000, 0, -60), False, True):
                raise RuntimeError('Could not place companion in the known water guard fixture')
            state['wet_guard_deadline'] = game + 6.0
            phase = 'wet_guard'
            return
        elif phase == 'wet_guard':
            volume = dog.get_component_by_class(u.CharacterMovementComponent).get_physics_volume()
            dry = u.CoastalPlacementLibrary.is_dry_destination(dog, dog.get_actor_transform())
            nonwater = not volume or not volume.get_editor_property('water_volume')
            close = (dog.get_actor_location() - pawn.get_actor_location()).length() <= 800
            active = dog.capsule_component.get_collision_enabled() != u.CollisionEnabled.NO_COLLISION
            if not (dry and nonwater and close and active and 'FOLLOWING' in mode(dog)):
                if game < state['wet_guard_deadline']:
                    return
                raise RuntimeError('Current-wet guard did not produce a nearby active dry regroup')
            rows.append({'case': 'current_wet_guard_dry_regroup', 'passed': True,
                         'distance_to_player_cm': (dog.get_actor_location() - pawn.get_actor_location()).length(),
                         'location': list(dog.get_actor_location().to_tuple())})
            if not recovery.request_defeat_return():
                raise RuntimeError('Existing exclusive recovery owner rejected companion recovery check')
            state['saw_regroup'] = False
            state['recovery_deadline'] = game + 6.0
            phase = 'recovery'
        elif phase == 'recovery':
            state['saw_regroup'] |= 'REGROUPING' in mode(dog)
            if saves.is_player_return_active():
                return
            if (not state['saw_regroup'] or 'FOLLOWING' not in mode(dog)
                    or not u.CoastalPlacementLibrary.is_dry_destination(dog, dog.get_actor_transform())):
                if game < state['recovery_deadline']:
                    return
                raise RuntimeError('Companion did not suspend and resume around safe return')
            volume = dog.get_component_by_class(u.CharacterMovementComponent).get_physics_volume()
            if volume and volume.get_editor_property('water_volume'):
                raise RuntimeError('Recovery regroup selected a water PhysicsVolume')
            rows.append({'case': 'recovery_suspends_then_dry_regroups', 'passed': True,
                         'location': list(dog.get_actor_location().to_tuple())})
            if not director.set_following(False):
                raise RuntimeError('Could not establish transient wait state before epoch test')
            result = saves.load_campaign(u.Name(TEST_SAVE))
            if result not in (u.CoastalSaveResult.LOADED, u.CoastalSaveResult.RECOVERED_PREVIOUS):
                raise RuntimeError('Disposable campaign reload failed: ' + str(result))
            phase, next_game = 'epoch_reset', game + 6.0
        elif phase == 'epoch_reset':
            if not dog.is_following_requested() or 'FOLLOWING' not in mode(dog):
                if game < next_game:
                    return
                raise RuntimeError('Campaign epoch did not reset wait and finish regroup')
            if not u.CoastalPlacementLibrary.is_dry_destination(dog, dog.get_actor_transform()):
                raise RuntimeError('Campaign epoch regroup is not a verified dry placement')
            rows.append({'case': 'campaign_epoch_resets_transient_wait_and_regroups', 'passed': True})
            finish()
    except Exception as exc:
        finish(str(exc))


handle = u.register_slate_post_tick_callback(tick)
print('Queued coastal companion live acceptance for ' + TEST_SAVE)
