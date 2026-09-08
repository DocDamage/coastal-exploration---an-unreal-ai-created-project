"""Disposable PIE arrival/camera inspection; restores the player's camera afterward."""
import json
import time
from pathlib import Path
import unreal as u

OUT = Path('F:/coastline/local-evidence')
w = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn = u.GameplayStatics.get_player_character(w, 0)
pc = u.GameplayStatics.get_player_controller(w, 0)
boom = pawn.get_component_by_class(u.SpringArmComponent)
original = (boom.get_editor_property('relative_location'), boom.get_editor_property('target_arm_length'),
            boom.get_editor_property('do_collision_test'), pc.get_control_rotation())
views = [
    ('hallsands', (-6500,22000,192), (-4000,8000,9000), (-19000,22000,2200)),
    ('baelo', (8000,20000,526), (14500,15000,6600), (8000,23000,800)),
    ('powell', (25500,4000,398), (30200,300,1900), (27000,3600,500)),
    ('harbour', (28200,23000,448), (25800,19000,2800), (30200,23000,850)),
    ('sea-platform', (38600,-14000,898), (35600,-17500,2800), (40000,-14000,1200)),
    ('camp', (7600,9100,1318), (8060,8660,1550), (7600,9200,1260)),
]
rows = []
index, phase, deadline = 0, 0, time.monotonic()


def finish():
    boom.set_editor_property('relative_location', original[0])
    boom.set_editor_property('target_arm_length', original[1])
    boom.set_editor_property('do_collision_test', original[2])
    pc.set_control_rotation(original[3])
    u.unregister_slate_post_tick_callback(handle)
    (OUT/'m3-expansion-pie-arrivals.json').write_text(json.dumps(rows, indent=2))


def tick(delta):
    global index, phase, deadline
    if time.monotonic() < deadline:
        return
    try:
        if index >= len(views):
            finish()
            return
        name, arrival, camera, target = views[index]
        if phase == 0:
            safe = u.CoastalPlacementLibrary.is_dry_destination(pawn, u.Transform(location=u.Vector(*arrival)))
            if not safe:
                raise RuntimeError('Unsafe arrival '+name)
            pawn.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
            pawn.set_actor_location_and_rotation(u.Vector(*arrival), u.Rotator(), False, True)
            boom.set_editor_property('target_arm_length', 0.0)
            boom.set_editor_property('do_collision_test', False)
            boom.set_world_location(u.Vector(*camera), False, True)
            pc.set_control_rotation(u.MathLibrary.find_look_at_rotation(u.Vector(*camera), u.Vector(*target)))
            phase = 1
            deadline = time.monotonic()+4
        else:
            actual = list(pawn.get_actor_location().to_tuple())
            rows.append({'name': name, 'arrival': arrival, 'actual_after_ticks': actual,
                         'stayed_at_arrival': abs(actual[0]-arrival[0]) < 40 and abs(actual[1]-arrival[1]) < 40,
                         'recovery_detail': pawn.get_component_by_class(u.CoastalPlayerRecoveryComponent).last_detail})
            u.SystemLibrary.execute_console_command(w, 'Shot SHOWUI filename='+str(OUT/('m3-expansion-pie-'+name+'.png')), pc)
            index += 1
            phase = 0
            deadline = time.monotonic()+3
    except Exception as exc:
        rows.append({'error': str(exc)})
        finish()
        raise


handle = u.register_slate_post_tick_callback(tick)
print('Queued six PIE arrival captures; camera restored on completion.')
