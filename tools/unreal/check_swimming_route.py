"""Disposable PIE traversal using actual CharacterMovement input, with transition assertions."""
import json
import time
from pathlib import Path
import unreal as u

REPORT = Path('F:/coastline/local-evidence/m3-swimming-route.json')
REPORT.write_text(json.dumps({'passed': False, 'status': 'starting'}), encoding='utf-8')
try:
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
    if not world:
        raise RuntimeError('Expected disposable PIE campaign')
    pawn = u.GameplayStatics.get_player_character(world, 0)
    movement = pawn.get_component_by_class(u.CharacterMovementComponent)
    swim = pawn.get_component_by_class(u.CoastalSwimmingComponent)
    saves = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
             if 'UEDPIE_' in s.get_path_name() and s.has_active_campaign()]
    if len(saves) != 1 or not swim or not movement:
        raise RuntimeError('Expected one active campaign and the compiled swimming component')
    saves = saves[0]
    start = u.Vector(12500, 1800, 348)
    if not u.CoastalPlacementLibrary.is_dry_destination(pawn, u.Transform(location=start)):
        raise RuntimeError('Existing dock dry checkpoint is unavailable')
    movement.stop_movement_immediately()
    if not pawn.set_actor_location(start, False, True):
        raise RuntimeError('Could not place player at initial dry checkpoint')
except Exception as exc:
    REPORT.write_text(json.dumps({'passed': False, 'status': 'preflight_failed', 'error': str(exc)}))
    raise

points = [(12500, 0, False), (11750, 0, False), (11750, -1000, False),
          (11750, -2300, True), (11350, -2700, True), (11750, -2900, True),
          (11750, -2300, True), (11750, -1200, False), (11750, 0, False),
          (12500, 0, False), (12500, 1800, False)]
index = 0
rows = []
began = time.monotonic()
deadline = began + 40
last = start
original_mesh = pawn.mesh.get_editor_property('skeletal_mesh_asset')
original_anim = pawn.mesh.get_anim_instance().get_class()
finished = False


def finish(error=None):
    global finished
    if finished:
        return
    finished = True
    try:
        movement.stop_movement_immediately()
        pawn.consume_movement_input_vector()
    except Exception as exc:
        error = error or str(exc)
    finally:
        u.unregister_slate_post_tick_callback(handle)
    REPORT.write_text(json.dumps({'passed': error is None, 'error': error, 'rows': rows,
                                 'elapsed_seconds': time.monotonic() - began,
                                 'scope': 'Scripted native movement; physical device and package acceptance remain separate'},
                                indent=2), encoding='utf-8')


def tick(delta):
    global index, deadline, last
    try:
        if not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
            raise RuntimeError('PIE ended during swimming route')
        pos = pawn.get_actor_location()
        if (pos - last).length() > 1500:
            raise RuntimeError('Unexpected recovery/relocation during swim route')
        last = pos
        if index == len(points):
            if swim.is_surface_swimming() or not movement.is_moving_on_ground():
                raise RuntimeError('Player did not finish on dry ground')
            if (pawn.mesh.get_editor_property('skeletal_mesh_asset') != original_mesh
                    or pawn.mesh.get_anim_instance().get_class() != original_anim):
                raise RuntimeError('Original mesh/AnimBP was not restored')
            finish()
            return
        x, y, expected_swimming = points[index]
        direction = u.Vector(x-pos.x, y-pos.y, 0)
        if direction.length() < 55:
            actual_swimming = swim.is_surface_swimming()
            rows.append({'point': index, 'position': list(pos.to_tuple()),
                         'swimming': actual_swimming, 'mode': str(movement.movement_mode),
                         'generation': saves.get_generation()})
            if actual_swimming != expected_swimming:
                raise RuntimeError('Unexpected swimming state at point ' + str(index))
            index += 1
            deadline = time.monotonic() + 40
            return
        if time.monotonic() > deadline:
            raise RuntimeError('Route timed out at point ' + str(index) + ': ' + str(pos))
        pawn.add_movement_input(direction.normal(), 1.0, False)
    except Exception as exc:
        finish(str(exc))


handle = u.register_slate_post_tick_callback(tick)
print('Swimming route queued from existing dock checkpoint')
