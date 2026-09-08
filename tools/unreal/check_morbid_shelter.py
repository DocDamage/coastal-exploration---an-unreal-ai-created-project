"""Check the real pause command, completion and interruptions in disposable PIE."""
import json
import time
from pathlib import Path
import unreal as u

REPORT = Path('F:/coastline/local-evidence/m3-morbid-shelter-live.json')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn = u.GameplayStatics.get_player_character(world, 0)
pc = u.GameplayStatics.get_player_controller(world, 0)
ui = pc.get_component_by_class(u.CoastalUISessionComponent)
action = pawn.get_component_by_class(u.CoastalCampingActionComponent)
move = pawn.get_component_by_class(u.CharacterMovementComponent)
saves = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
         if 'UEDPIE_' in s.get_path_name() and s.has_active_campaign()]
if len(saves) != 1 or not str(saves[0].get_active_save_set()).startswith('coastal_test_') or ui.has_modal():
    raise RuntimeError('Requires one active disposable test campaign with gameplay input')
markers = u.GameplayStatics.get_all_actors_with_tag(world, 'Coastal.ShelterRest')
if len(markers) != 1:
    raise RuntimeError('Expected one authored shelter rest point')
marker = markers[0]
original_mesh = pawn.mesh.get_skeletal_mesh_asset()
original_anim = pawn.mesh.get_anim_instance().get_class()
expected_clip = u.load_asset('/Game/MorbidMotions_Pack/Animations_Vol2/30fps/ANIM_idle_sleeping_A')
expected_mesh = u.load_asset('/Game/MorbidMotions_Pack/Demo/Characters/Mannequins/Meshes/SKM_Quinn_Simple')
start = marker.get_actor_location() + u.Vector(0, 150, 0)
if not u.CoastalPlacementLibrary.is_dry_destination(pawn, u.Transform(location=start)):
    raise RuntimeError('Shelter approach is not dry and unobstructed')
move.stop_movement_immediately()
pawn.set_actor_location(start, False, True)
rows = []
phase = 0
began = time.monotonic()
deadline = began + 2
done = False


def normal():
    anim = pawn.mesh.get_anim_instance()
    return pawn.mesh.get_skeletal_mesh_asset() == original_mesh and anim and anim.get_class() == original_anim


def finish(error=None):
    global done
    if done:
        return
    done = True
    u.unregister_slate_post_tick_callback(handle)
    move.stop_movement_immediately()
    REPORT.write_text(json.dumps({'passed': error is None, 'error': error, 'rows': rows,
                                 'elapsed_seconds': time.monotonic() - began,
                                 'scope': 'Scripted native PIE; physical device acceptance separate'}, indent=2), encoding='utf-8')


def panel_command(command):
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport()]
    if len(panels) != 1:
        raise RuntimeError('Ambiguous native menu')
    panels[0].call_method('Execute', args=(command,))


def tick(delta):
    global phase, deadline
    try:
        now = time.monotonic()
        if now - began > 40:
            raise RuntimeError('Shelter verification timed out')
        if now < deadline:
            return
        if phase == 0:
            if not action.can_offer_action(u.CoastalCampingAction.REST_IN_SHELTER):
                raise RuntimeError('Shelter action was not offered at its safe approach')
            ui.open_pause()
            phase, deadline = 1, now + .5
        elif phase == 1:
            panel_command('shelter_rest')
            phase, deadline = 2, now + .3
        elif phase == 2:
            if ui.has_modal() or not action.is_action_active() or pawn.mesh.get_skeletal_mesh_asset() != expected_mesh:
                raise RuntimeError('Native menu did not start Morbid presentation')
            if pawn.mesh.get_anim_instance().get_animation_asset() != expected_clip:
                raise RuntimeError('Wrong supplied rest animation')
            rows.append({'case': 'native_menu_starts_actual_morbid_clip', 'passed': True})
            phase, deadline = 3, now + 4
        elif phase == 3:
            if action.is_action_active() or not normal():
                raise RuntimeError('Completion did not restore locomotion')
            rows.append({'case': 'completion_restores_original_mesh_and_anim', 'passed': True})
            result = action.start_action(u.CoastalCampingAction.REST_IN_SHELTER)
            if result != u.CoastalCampingStartResult.STARTED:
                raise RuntimeError('Repeat action rejected: ' + str(result) + ' ' + action.last_detail)
            ui.open_pause()
            phase, deadline = 4, now + .5
        elif phase == 4:
            if action.is_action_active() or not normal():
                raise RuntimeError('Menu cancellation did not restore locomotion')
            rows.append({'case': 'menu_cancellation', 'passed': True})
            panel_command('back')
            phase, deadline = 5, now + .5
        elif phase == 5:
            if action.start_action(u.CoastalCampingAction.REST_IN_SHELTER) != u.CoastalCampingStartResult.STARTED:
                raise RuntimeError('Could not start jump interruption case')
            pawn.jump()
            phase, deadline = 6, now + .5
        elif phase == 6:
            if action.is_action_active() or not normal():
                raise RuntimeError('Jump cancellation failed')
            rows.append({'case': 'jump_cancellation', 'passed': True})
            finish()
    except Exception as exc:
        finish(str(exc))


REPORT.write_text(json.dumps({'passed': False, 'status': 'running'}), encoding='utf-8')
handle = u.register_slate_post_tick_callback(tick)
print('Shelter check running')
