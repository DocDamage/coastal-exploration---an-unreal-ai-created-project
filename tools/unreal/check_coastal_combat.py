"""Exercise authoritative shot, LOS, reload, pause, defeat and epoch cleanup in disposable PIE."""
import json
import time
from pathlib import Path
import unreal as u

REPORT = Path('F:/coastline/local-evidence/m3-coastal-combat-live.json')
AUTHORING = json.loads(Path('F:/coastline/local-evidence/m3-coastal-combat-authoring.json').read_text())
REPORT.write_text(json.dumps({'passed': False, 'status': 'starting'}), encoding='utf-8')
save_set = str(globals().get('save_set', ''))
if not save_set.startswith('coastal_test_'):
    raise RuntimeError('Pass an active disposable save_set beginning coastal_test_')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn = u.GameplayStatics.get_player_character(world, 0)
pc = u.GameplayStatics.get_player_controller(world, 0)
move = pawn.get_component_by_class(u.CharacterMovementComponent)
ui = pc.get_component_by_class(u.CoastalUISessionComponent)
saves_found = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
               if 'UEDPIE_' in s.get_path_name() and s.has_active_campaign()]
if len(saves_found) != 1 or str(saves_found[0].get_active_save_set()) != save_set:
    raise RuntimeError('Expected the named disposable campaign')
saves = saves_found[0]
targets = {a.get_actor_label().removeprefix('coastal_combat — '): a
           for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalCombatTarget)}
required = {'Clear training target', 'Occluded training target', 'Harbour sentry'}
if not required.issubset(targets):
    raise RuntimeError('Authored combat targets are missing')

phase = 'place'; next_time = 0.0; deadline = time.monotonic() + 180
combat = None; rows = []; state = {}; finished = False


def aim(actor):
    camera = u.GameplayStatics.get_player_camera_manager(world, 0).get_camera_location()
    pc.set_control_rotation(u.MathLibrary.find_look_at_rotation(camera, actor.get_actor_location()))


def key(name, pressed):
    if not u.CoastalVendorInspection.submit_combat_test_key(name, pressed):
        raise RuntimeError('Guarded controller input probe rejected ' + name)

def close_pause():
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport()]
    if len(panels) != 1:
        raise RuntimeError('Pause panel is ambiguous')
    panels[0].call_method('Execute', args=('back',))

def vitals():
    return combat.get_health() + combat.get_shield()


def finish(error=None):
    global finished
    if finished:
        return
    finished = True
    u.unregister_slate_post_tick_callback(handle)
    for name in ('LeftMouseButton', 'R', 'Gamepad_RightTrigger', 'Gamepad_LeftShoulder'):
        u.CoastalVendorInspection.submit_combat_test_key(name, False)
    REPORT.write_text(json.dumps({'passed': error is None, 'error': error, 'rows': rows,
        'save_set': save_set, 'scope': 'Disposable scripted PIE; physical-device aiming remains separate'},
        indent=2), encoding='utf-8')


def tick(delta):
    global phase, next_time, combat, deadline
    try:
        now = time.monotonic()
        if now < next_time:
            return
        if now > deadline:
            raise RuntimeError('Combat live check timed out in ' + phase)
        if phase == 'place':
            move.stop_movement_immediately()
            pawn.set_actor_location(u.Vector(*AUTHORING['test_player_location']), False, True)
            phase = 'arm'; next_time = now + 1.0; return
        if phase == 'arm':
            combat = pawn.get_component_by_class(u.CoastalCombatComponent)
            if not combat or not combat.is_initialized() or not combat.is_encounter_active():
                raise RuntimeError('Combat did not initialize inside the authored volume')
            state['initial_clip'] = combat.get_current_ammo()
            state['initial_clear_health'] = targets['Clear training target'].get_health()
            aim(targets['Clear training target'])
            phase = 'aim_clear'; next_time = now + .75; return
        if phase == 'aim_clear':
            key('LeftMouseButton', True)
            phase = 'clear'; next_time = now + .25; return
        if phase == 'clear':
            key('LeftMouseButton', False)
            if combat.get_current_ammo() != state['initial_clip'] - 1:
                raise RuntimeError('Accepted shot did not consume exactly one authoritative round')
            if targets['Clear training target'].get_health() >= state['initial_clear_health']:
                raise RuntimeError('Clear shot did not apply point damage')
            rows.append({'case': 'mouse_mapping_authoritative_damage_and_single_ammo', 'passed': True})
            state['blocked_health'] = targets['Occluded training target'].get_health()
            aim(targets['Occluded training target'])
            phase = 'wall_fire'; next_time = now + .2; return
        if phase == 'wall_fire':
            state['wall_ammo'] = combat.get_current_ammo()
            key('Gamepad_RightTrigger', True)
            phase = 'wall_check'; next_time = now + .25; return
        if phase == 'wall_check':
            key('Gamepad_RightTrigger', False)
            if combat.get_current_ammo() != state['wall_ammo'] - 1:
                raise RuntimeError('Gamepad trigger mapping did not fire exactly once')
            if targets['Occluded training target'].get_health() != state['blocked_health']:
                raise RuntimeError('Visibility wall failed to block target damage')
            rows.append({'case': 'gamepad_mapping_and_visibility_wall_blocks_damage', 'passed': True})
            ui.open_pause(); state['pause_ammo'] = combat.get_current_ammo()
            phase = 'pause'; next_time = now + .25; return
        if phase == 'pause':
            result = combat.try_fire()
            if result not in (u.CoastalCombatResult.INPUT_BLOCKED, u.CoastalCombatResult.PAUSED):
                raise RuntimeError('Pause did not reject combat: ' + str(result))
            if combat.get_current_ammo() != state['pause_ammo']:
                raise RuntimeError('Rejected paused shot mutated ammo')
            rows.append({'case': 'pause_rejects_without_mutation', 'passed': True})
            close_pause()
            phase = 'reload'; next_time = now + .5; return
        if phase == 'reload':
            if ui.has_modal():
                close_pause(); next_time = now + .1; return
            state['pre_reload_clip'] = combat.get_current_ammo()
            state['pre_reload_reserve'] = combat.get_reserve_ammo()
            key('R', True)
            phase = 'reload_input'; next_time = now + .25; return
        if phase == 'reload_input':
            key('R', False)
            if combat.try_fire() != u.CoastalCombatResult.RELOADING:
                raise RuntimeError('Keyboard reload mapping did not begin vendor reload')
            ui.open_pause()
            phase = 'reload_paused'; next_time = now + 2.5; return
        if phase == 'reload_paused':
            if (combat.get_current_ammo() != state['pre_reload_clip']
                    or combat.get_reserve_ammo() != state['pre_reload_reserve']):
                raise RuntimeError('Reload advanced while native menu paused the game')
            rows.append({'case': 'reload_timer_pauses_with_game', 'passed': True})
            close_pause()
            phase = 'reload_resume'; next_time = now + .1; return
        if phase == 'reload_resume':
            if ui.has_modal():
                close_pause(); next_time = now + .1; return
            if combat.try_fire() != u.CoastalCombatResult.RELOADING:
                raise RuntimeError('Reload gate expired during pause before vendor completion')
            phase = 'reload_check'; next_time = now + 2.5; deadline = now + 20; return
        if phase == 'reload_check':
            if combat.get_current_ammo() == state['pre_reload_clip']:
                next_time = now + .2; return
            if (combat.get_current_ammo() <= state['pre_reload_clip']
                    or combat.get_current_ammo() != state['initial_clip']
                    or combat.get_reserve_ammo() >= state['pre_reload_reserve']
                    or combat.get_current_ammo() + combat.get_reserve_ammo()
                       != state['pre_reload_clip'] + state['pre_reload_reserve']):
                raise RuntimeError('Reload did not publish authoritative ammo transfer')
            rows.append({'case': 'reload_uses_authoritative_values', 'passed': True})
            move.stop_movement_immediately()
            pawn.set_actor_location(u.Vector(*AUTHORING['sentry_test_location']), False, True)
            away = pawn.get_actor_location() + (pawn.get_actor_location() - targets['Harbour sentry'].get_actor_location())
            pc.set_control_rotation(u.MathLibrary.find_look_at_rotation(pawn.get_actor_location(), away))
            phase = 'offscreen_settle'; next_time = now + 2.5; deadline = now + 20; return
        if phase == 'offscreen_settle':
            state['offscreen_health'] = vitals()
            phase = 'offscreen_check'; next_time = now + 2.25; return
        if phase == 'offscreen_check':
            if vitals() != state['offscreen_health']:
                raise RuntimeError('Sentry attacked while it was not recently rendered')
            rows.append({'case': 'offscreen_sentry_does_not_attack', 'passed': True})
            aim(targets['Harbour sentry'])
            state['visible_health'] = vitals()
            phase = 'visible_sentry'; next_time = now + 2.5; return
        if phase == 'visible_sentry':
            if vitals() >= state['visible_health']:
                next_time = now + .25; return
            rows.append({'case': 'visible_sentry_attacks_in_bounded_los', 'passed': True})
            u.GameplayStatics.apply_damage(pawn, 1000.0, None, targets['Harbour sentry'], u.DamageType)
            phase = 'defeat'; next_time = now + .2; deadline = now + 20; return
        if phase == 'defeat':
            if not combat.is_defeated() or not saves.is_player_return_active():
                raise RuntimeError('Lethal damage did not enter existing recovery flow')
            if combat.get_transient_weapon():
                raise RuntimeError('Defeat did not destroy transient weapon state')
            phase = 'returned'; next_time = now + .25; return
        if phase == 'returned':
            if saves.is_player_return_active():
                if combat.get_transient_weapon():
                    raise RuntimeError('Transient weapon respawned during exclusive recovery')
                next_time = now + .25; return
            if combat.is_defeated() or combat.get_health() != 100.0 or combat.get_shield() != 50.0:
                raise RuntimeError('Safe return did not reset transient health and shield')
            rows.append({'case': 'defeat_uses_recovery_and_resets_transient_state', 'passed': True})
            pawn.set_actor_location(u.Vector(*AUTHORING['test_player_location']), False, True)
            phase = 'rearm'; next_time = now + .75; return
        if phase == 'rearm':
            state['old_weapon'] = combat.get_transient_weapon()
            if not state['old_weapon'] or not combat.is_encounter_active():
                raise RuntimeError('Encounter did not rearm before epoch reset check')
            result = saves.load_campaign(u.Name(save_set))
            if result not in (u.CoastalSaveResult.LOADED, u.CoastalSaveResult.RECOVERED_PREVIOUS):
                raise RuntimeError('Disposable campaign reload failed')
            phase = 'epoch'; next_time = now + .5; return
        if phase == 'epoch':
            if u.SystemLibrary.is_valid(state['old_weapon']):
                raise RuntimeError('Campaign reload retained the prior epoch weapon')
            if combat.get_transient_weapon() and not combat.is_encounter_active():
                raise RuntimeError('Epoch reload leaked a weapon outside the encounter')
            rows.append({'case': 'campaign_epoch_cleans_transient_weapon', 'passed': True})
            finish()
    except Exception as exc:
        finish(str(exc))


handle = u.register_slate_post_tick_callback(tick)
print('Queued coastal combat live acceptance for ' + save_set)
