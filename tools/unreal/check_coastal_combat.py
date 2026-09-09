"""Exercise authoritative shot, LOS, reload, pause, defeat and epoch cleanup in disposable PIE."""
import json
import time
from pathlib import Path
import unreal as u

REPORT = (Path(__file__).resolve().parents[3] / 'local-evidence/m3-coastal-combat-live.json')
AUTHORING = json.loads((Path(__file__).resolve().parents[3] / 'local-evidence/m3-coastal-combat-authoring.json').read_text())
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
walls = [a for a in u.GameplayStatics.get_all_actors_of_class(world, u.StaticMeshActor)
         if a.get_actor_label() == 'coastal_combat — Combat LOS wall']
if len(walls) != 1:
    raise RuntimeError('Expected one authored combat cover wall')
cover_wall = walls[0]
required = {'Clear training target', 'Occluded training target', 'Harbour sentry'}
if not required.issubset(targets):
    raise RuntimeError('Authored combat targets are missing')

phase = 'place'; next_time = 0.0; deadline = time.monotonic() + 180
combat = None; rows = []; state = {}; finished = False
original_dilation = u.GameplayStatics.get_global_time_dilation(world)
u.GameplayStatics.set_global_time_dilation(world, 1.0)


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
    u.GameplayStatics.set_global_time_dilation(world, original_dilation)
    if 'cover_transform' in state:
        cover_wall.set_actor_transform(state['cover_transform'], False, True)
        cover_wall.static_mesh_component.set_mobility(state['cover_mobility'])
        targets['Harbour sentry'].set_editor_property('require_recently_rendered', state['render_gate'])
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
            key('LeftMouseButton', True)
            phase = 'pause_input'; next_time = now + .25; return
        if phase == 'pause_input':
            key('LeftMouseButton', False)
            if combat.get_current_ammo() != state['pause_ammo']:
                raise RuntimeError('Paused mouse mapping mutated ammo')
            rows.append({'case': 'pause_blocks_mouse_input', 'passed': True})
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
            u.GameplayStatics.set_global_time_dilation(world, 0.1)
            phase = 'slow_first'; next_time = now + .2; deadline = now + 20; return
        if phase in ('slow_first', 'slow_second', 'fast_first'):
            probe = json.loads(u.CoastalVendorInspection.probe_combat_shot())
            if (probe.get('result') != 'Applied' or probe.get('receipts') != 1
                    or probe.get('tracers') != 1 or probe.get('max_endpoint_error_cm', 999) > .1
                    or probe['ammo_after'] != probe['ammo_before'] - 1):
                raise RuntimeError('Authoritative tracer/clock probe failed: ' + json.dumps(probe))
            rows.append({'case': phase + '_authoritative_tracer', 'passed': True, 'probe': probe})
            if phase == 'slow_first':
                phase = 'slow_second'; next_time = now + .15; return
            if phase == 'slow_second':
                u.GameplayStatics.set_global_time_dilation(world, 4.0)
                phase = 'fast_first'; next_time = now + .2; return
            state['fast_shot_time'] = probe['real_time']
            state['fast_attempts'] = 0
            phase = 'fast_gap'; next_time = now + .04; return
        if phase == 'fast_gap':
            rejected = json.loads(u.CoastalVendorInspection.probe_combat_shot())
            elapsed = rejected['real_time'] - state['fast_shot_time']
            if elapsed >= .1:
                # A slow editor frame cannot test the sub-interval boundary. Retry
                # from the new accepted shot, with a bounded wall-clock deadline.
                state['fast_attempts'] += 1
                if rejected.get('result') != 'Applied' or state['fast_attempts'] >= 10:
                    raise RuntimeError('Insufficient frame cadence for fast cooldown boundary')
                state['fast_shot_time'] = rejected['real_time']
                next_time = now + .02; return
            if (rejected.get('result') != 'Cooldown' or rejected.get('receipts') != 0
                    or rejected.get('tracers') != 0 or rejected['ammo_after'] != rejected['ammo_before']):
                raise RuntimeError('Dilated cooldown emitted effects or mutated ammo: ' + json.dumps(rejected))
            rows.append({'case': 'fast_cooldown_no_receipt_or_tracer', 'passed': True,
                         'elapsed_real_seconds': elapsed, 'probe': rejected})
            u.GameplayStatics.set_global_time_dilation(world, 1.0)
            state['shoulder_clip'] = combat.get_current_ammo()
            state['shoulder_total'] = combat.get_current_ammo() + combat.get_reserve_ammo()
            key('Gamepad_LeftShoulder', True)
            phase = 'shoulder_reload'; next_time = now + .2; return
        if phase == 'shoulder_reload':
            key('Gamepad_LeftShoulder', False)
            if combat.try_fire() != u.CoastalCombatResult.RELOADING:
                raise RuntimeError('Gamepad shoulder did not start reload')
            phase = 'shoulder_complete'; next_time = now + 2.1; deadline = now + 20; return
        if phase == 'shoulder_complete':
            if combat.get_current_ammo() == state['shoulder_clip']:
                next_time = now + .2; return
            if (combat.get_current_ammo() != state['initial_clip']
                    or combat.get_current_ammo() + combat.get_reserve_ammo() != state['shoulder_total']):
                raise RuntimeError('Gamepad reload did not conserve authoritative ammo')
            rows.append({'case': 'gamepad_shoulder_reload', 'passed': True})
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
            state['cover_transform'] = cover_wall.get_actor_transform()
            state['cover_mobility'] = cover_wall.static_mesh_component.mobility
            cover_wall.static_mesh_component.set_mobility(u.ComponentMobility.MOVABLE)
            state['render_gate'] = targets['Harbour sentry'].get_editor_property('require_recently_rendered')
            # Isolate world-cover rejection from the separate recently-rendered gate.
            targets['Harbour sentry'].set_editor_property('require_recently_rendered', False)
            middle = (pawn.get_actor_location() + targets['Harbour sentry'].get_actor_location()) * .5
            cover_wall.set_actor_location(middle + u.Vector(0, 0, 40), False, True)
            cover_wall.set_actor_scale3d(u.Vector(.25, 3, 3))
            if (cover_wall.get_actor_location() - (middle + u.Vector(0, 0, 40))).length() > 1:
                raise RuntimeError('Temporary cover wall did not move into the shot path')
            state['cover_health'] = vitals()
            phase = 'sentry_cover'; next_time = now + 2.25; return
        if phase == 'sentry_cover':
            if vitals() != state['cover_health']:
                raise RuntimeError('Sentry damaged the player through world cover')
            rows.append({'case': 'sentry_world_cover_blocks_pawn_trace', 'passed': True})
            cover_wall.set_actor_transform(state.pop('cover_transform'), False, True)
            cover_wall.static_mesh_component.set_mobility(state['cover_mobility'])
            targets['Harbour sentry'].set_editor_property('require_recently_rendered', state['render_gate'])
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
            # Loading can also schedule Mutable regeneration and asset work.
            # Give this phase its own bounded window instead of inheriting the
            # earlier sentry/defeat deadline.
            phase = 'epoch'; next_time = time.monotonic() + .5; deadline = time.monotonic() + 30; return
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
