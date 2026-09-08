"""Exercise pause, wet save/load and native boundary recovery in disposable PIE."""
import json
import time
from pathlib import Path
import unreal as u

REPORT = Path('F:/coastline/local-evidence/m3-swimming-interruptions.json')
REPORT.write_text(json.dumps({'passed': False, 'status': 'starting'}))
try:
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
    pawn = u.GameplayStatics.get_player_character(world, 0)
    pc = u.GameplayStatics.get_player_controller(world, 0)
    swim = pawn.get_component_by_class(u.CoastalSwimmingComponent)
    move = pawn.get_component_by_class(u.CharacterMovementComponent)
    ui = pc.get_component_by_class(u.CoastalUISessionComponent)
    saves = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
             if 'UEDPIE_' in s.get_path_name() and s.has_active_campaign()]
    if len(saves) != 1 or str(saves[0].get_active_save_set()) != 'coastal_test_m3_swimming_0908a':
        raise RuntimeError('Expected the dedicated disposable swimming campaign')
    saves = saves[0]
    if swim.is_surface_swimming() or ui.has_modal():
        raise RuntimeError('Finish the route on dry ground before running interruptions')
    original_mesh = pawn.mesh.get_skeletal_mesh_asset()
    original_anim = pawn.mesh.get_anim_instance().get_class()
except Exception as exc:
    REPORT.write_text(json.dumps({'passed': False, 'status': 'preflight_failed', 'error': str(exc)}))
    raise

rows = []
phase = 0
began = time.monotonic()
deadline = began
boundary_deadline = None
pause_position = None
finished = False


def normal_presentation():
    return (pawn.mesh.get_skeletal_mesh_asset() == original_mesh
            and pawn.mesh.get_anim_instance().get_class() == original_anim)


def finish(error=None):
    global finished
    if finished:
        return
    finished = True
    try:
        move.stop_movement_immediately()
        pawn.consume_movement_input_vector()
    finally:
        u.unregister_slate_post_tick_callback(handle)
    REPORT.write_text(json.dumps({'passed': error is None, 'error': error, 'rows': rows,
                                 'elapsed_seconds': time.monotonic()-began,
                                 'scope': 'Scripted PIE with disposable test placement; native pause/save/load/recovery'},
                                indent=2), encoding='utf-8')


def tick(delta):
    global phase, deadline, pause_position, boundary_deadline
    try:
        now = time.monotonic()
        if now < deadline:
            return
        if not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
            raise RuntimeError('PIE ended during interruption checks')
        if phase in (0, 6):
            # This is test setup, not a player-facing water entry or travel action.
            move.stop_movement_immediately()
            pawn.set_actor_location(u.Vector(11500, -2700, -60), False, True)
            phase += 1
            deadline = now + 2
        elif phase == 1:
            if not swim.is_surface_swimming() or not swim.is_recovery_protected():
                raise RuntimeError('Swimming did not activate at the isolated test position')
            pause_position = pawn.get_actor_location()
            ui.open_pause()
            phase = 2
            deadline = now + 1
        elif phase == 2:
            if not ui.has_modal() or swim.is_surface_swimming() or not normal_presentation():
                raise RuntimeError('Pause did not release swimming presentation')
            if (pawn.get_actor_location()-pause_position).length() > 5:
                raise RuntimeError('Player moved while paused')
            rows.append({'case': 'pause_restores_presentation_and_holds_position', 'passed': True})
            panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
                      if p.is_in_viewport()]
            if len(panels) != 1:
                raise RuntimeError('Pause panel is ambiguous')
            panels[0].call_method('Execute', args=('back',))
            phase = 3
            deadline = now + 2
        elif phase == 3:
            if not swim.is_surface_swimming() or not swim.is_recovery_protected():
                raise RuntimeError('Swimming did not resume after closing pause')
            rows.append({'case': 'resume_reenters_bounded_swimming', 'passed': True})
            result = saves.save_now()
            if result != u.CoastalSaveResult.SAVED:
                raise RuntimeError('Wet save failed: ' + str(result))
            rows.append({'case': 'save_while_swimming', 'generation': saves.get_generation(), 'passed': True})
            phase = 4
            deadline = now + 1
        elif phase == 4:
            result = saves.load_campaign('coastal_test_m3_swimming_0908a')
            if result != u.CoastalSaveResult.LOADED:
                raise RuntimeError('Wet campaign reload failed: ' + str(result))
            phase = 5
            deadline = now + 2
        elif phase == 5:
            if (swim.is_surface_swimming() or not normal_presentation()
                    or not u.CoastalPlacementLibrary.is_dry_destination(pawn, pawn.get_actor_transform())):
                raise RuntimeError('Wet save reload did not restore dry locomotion')
            rows.append({'case': 'wet_save_load_uses_dry_checkpoint', 'position': list(pawn.get_actor_location().to_tuple()),
                         'generation': saves.get_generation(), 'passed': True})
            phase = 6
        elif phase == 7:
            if not swim.is_surface_swimming():
                raise RuntimeError('Second swimming entry failed')
            boundary_deadline = now + 20
            phase = 8
        elif phase == 8:
            if saves.is_player_return_active():
                pawn.consume_movement_input_vector()
                phase = 9
                return
            if now > boundary_deadline:
                raise RuntimeError('Leaving the marked swim region did not trigger recovery')
            pawn.add_movement_input(u.Vector(-1, 0, 0), 1.0, False)
        elif phase == 9:
            if now > boundary_deadline:
                raise RuntimeError('Boundary recovery did not finish')
            if saves.is_player_return_active():
                return
            if (swim.is_surface_swimming() or not normal_presentation()
                    or not u.CoastalPlacementLibrary.is_dry_destination(pawn, pawn.get_actor_transform())):
                raise RuntimeError('Boundary recovery did not restore dry locomotion')
            rows.append({'case': 'native_swim_outside_zone_recovers', 'position': list(pawn.get_actor_location().to_tuple()),
                         'passed': True})
            finish()
    except Exception as exc:
        finish(str(exc))


handle = u.register_slate_post_tick_callback(tick)
print('Queued swimming pause/save/load/boundary checks')
