"""Walk authored paths in a disposable PIE campaign using CharacterMovement.

Supply routes=[{'name': str, 'start': capsule_xyz, 'points': [ground_xyz]}].
This exercises collision and recovery, not physical keyboard/controller input.
"""
import json
import time
from pathlib import Path
import unreal as u

REPORT = (Path(__file__).resolve().parents[3] / 'local-evidence/m3-outer-coast-traversal.json')
REPORT.write_text(json.dumps({'passed': False, 'status': 'starting'}), encoding='utf-8')
try:
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
    if world is None or not globals().get('routes'):
        raise RuntimeError('Expected active disposable PIE campaign and explicit routes')
    pawn = u.GameplayStatics.get_player_character(world, 0)
    controller = u.GameplayStatics.get_player_controller(world, 0)
    if pawn is None or controller is None:
        raise RuntimeError('PIE player is unavailable')
    movement = pawn.get_component_by_class(u.CharacterMovementComponent)
    recovery = pawn.get_component_by_class(u.CoastalPlayerRecoveryComponent)
    coordinators = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
                    if 'UEDPIE_' in s.get_path_name() and s.has_active_campaign()]
    if movement is None or recovery is None or len(coordinators) != 1:
        raise RuntimeError('Expected movement, recovery and one active campaign coordinator')
    saves = coordinators[0]
    if not str(saves.get_active_save_set()).startswith('coastal_test_'):
        raise RuntimeError('Traversal requires a disposable campaign')
except Exception as exc:
    REPORT.write_text(json.dumps({'passed': False, 'status': 'preflight_failed',
                                  'error': str(exc)}), encoding='utf-8')
    raise
rows = []
route_index = point_index = 0
started = False
finished = False
deadline = time.monotonic()
last_position = None
start_time = time.monotonic()


def finish(error=None):
    global finished
    if finished:
        return
    finished = True
    try:
        movement.stop_movement_immediately()
        pawn.consume_movement_input_vector()
    except Exception as exc:
        error = error or ('PIE cleanup failed: ' + str(exc))
    finally:
        u.unregister_slate_post_tick_callback(handle)
    report = {'passed': error is None, 'error': error, 'rows': rows,
              'elapsed_seconds': time.monotonic() - start_time,
              'scope': 'Scripted CharacterMovement walking; physical device input and packaging not tested'}
    REPORT.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report))


def tick(delta):
    global route_index, point_index, started, deadline, last_position
    if finished:
        return
    try:
        if not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
            raise RuntimeError('PIE ended before traversal completed')
        if route_index >= len(routes):
            finish()
            return
        route = routes[route_index]
        now = u.GameplayStatics.get_time_seconds(world)
        if time.monotonic() - start_time > 900:
            raise RuntimeError('Traversal exceeded its wall-time budget')
        if not started:
            arrival = u.Vector(*route['start'])
            if not u.CoastalPlacementLibrary.is_dry_destination(pawn, u.Transform(location=arrival)):
                raise RuntimeError('Invalid start: ' + route['name'])
            movement.stop_movement_immediately()
            if not pawn.set_actor_location(arrival, False, True):
                raise RuntimeError('Initial placement failed: ' + route['name'])
            rows.append({'route': route['name'], 'start': route['start']})
            last_position = arrival
            point_index = 0
            started = True
            target = u.Vector(*route['points'][0])
            deadline = now + 20 + (target - arrival).length() / 150
            return
        actual = pawn.get_actor_location()
        if (actual - last_position).length() > 2500:
            raise RuntimeError('Unexpected relocation during ' + route['name'])
        last_position = actual
        target = u.Vector(*route['points'][point_index])
        if abs(actual.z - (target.z + 98)) > 800:
            raise RuntimeError('Pawn left route elevation: ' + route['name'])
        direction = u.Vector(target.x - actual.x, target.y - actual.y, 0)
        if direction.length() < 80:
            if not movement.is_moving_on_ground():
                raise RuntimeError('Route point is not grounded: ' + route['name'])
            rows.append({'route': route['name'], 'point': point_index,
                         'position': list(actual.to_tuple()), 'generation': saves.get_generation(),
                         'recovery_detail': recovery.last_detail})
            point_index += 1
            if point_index == len(route['points']):
                movement.stop_movement_immediately()
                pawn.consume_movement_input_vector()
                route_index += 1
                started = False
            else:
                next_target = u.Vector(*route['points'][point_index])
                deadline = now + 20 + (next_target - actual).length() / 150
            return
        if now > deadline:
            raise RuntimeError('Traversal timed out at ' + route['name'] + ' point ' + str(point_index)
                               + ' position ' + str(actual))
        pawn.add_movement_input(direction.normal(), 1.0, False)
    except Exception as exc:
        finish(str(exc))


handle = u.register_slate_post_tick_callback(tick)
print('Queued scripted walking for ' + str(len(routes)) + ' routes')
