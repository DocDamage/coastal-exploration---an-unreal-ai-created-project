"""Capture the new places in disposable PIE and restore the original camera."""
import json
import time
from pathlib import Path
import unreal as u

world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
if world is None:
    raise RuntimeError('Expected disposable PIE session')
pawn = u.GameplayStatics.get_player_character(world, 0)
pc = u.GameplayStatics.get_player_controller(world, 0)
boom = pawn.get_component_by_class(u.SpringArmComponent)
movement = pawn.get_component_by_class(u.CharacterMovementComponent)
original = (boom.get_editor_property('relative_location'), boom.get_editor_property('target_arm_length'),
            boom.get_editor_property('do_collision_test'), pc.get_control_rotation())
views = globals().get('views', [
    ('prison', (21600, 29000, 498), (23800, 26900, 2050), (20500, 29000, 600)),
    ('village', (-1600, 15300, 2298), (1200, 19200, 4400), (-4500, 14500, 2700)),
])
index, phase, deadline = 0, 0, time.monotonic()
rows = []


def finish():
    u.unregister_slate_post_tick_callback(handle)
    try:
        boom.set_editor_property('relative_location', original[0])
        boom.set_editor_property('target_arm_length', original[1])
        boom.set_editor_property('do_collision_test', original[2])
        pc.set_control_rotation(original[3])
    finally:
        Path('F:/coastline/local-evidence/m3-omitted-capture-requests.json').write_text(
            json.dumps(rows, indent=2), encoding='utf-8')


def tick(delta):
    global index, phase, deadline
    if time.monotonic() < deadline:
        return
    try:
        if index == len(views):
            finish()
            return
        name, arrival, camera, target = views[index]
        if phase == 0:
            if not u.CoastalPlacementLibrary.is_dry_destination(pawn, u.Transform(location=u.Vector(*arrival))):
                raise RuntimeError('Capture arrival is unsafe: ' + name)
            movement.stop_movement_immediately()
            pawn.set_actor_location(u.Vector(*arrival), False, True)
            boom.set_editor_property('target_arm_length', 0.0)
            boom.set_editor_property('do_collision_test', False)
            boom.set_world_location(u.Vector(*camera), False, True)
            pc.set_control_rotation(u.MathLibrary.find_look_at_rotation(u.Vector(*camera), u.Vector(*target)))
            phase = 1
        else:
            path = 'F:/coastline/local-evidence/m3-omitted-' + name + '.png'
            u.SystemLibrary.execute_console_command(world, 'Shot SHOWUI filename=' + path, pc)
            rows.append({'name': name, 'requested': path, 'actual': list(pawn.get_actor_location().to_tuple()),
                         'scope': 'Capture requested; inspect generated PNG separately'})
            index += 1
            phase = 0
        deadline = time.monotonic() + 4
    except Exception as exc:
        rows.append({'error': str(exc)})
        finish()


handle = u.register_slate_post_tick_callback(tick)
print('Queued ' + str(len(views)) + ' ordinary PIE captures')
