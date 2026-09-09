"""Observe generated bones during existing movement, pickup, camping and authoritative damage."""
import json
import math
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn = u.GameplayStatics.get_player_character(world, 0)
pc = pawn.get_controller()
ui = pc.get_component_by_class(u.CoastalUISessionComponent)
creator = pawn.get_component_by_class(u.CoastalMutableCharacterComponent)
bridge = pawn.get_component_by_class(u.CoastalInteractionBridge)
move = pawn.get_component_by_class(u.CharacterMovementComponent)
camp = pawn.get_component_by_class(u.CoastalCampingActionComponent)
combat = pawn.get_component_by_class(u.CoastalCombatComponent)
saves = next(s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name())
if not str(saves.get_active_save_set()).startswith('coastal_test_') or not creator.is_appearance_ready():
    raise RuntimeError('Generated character in disposable campaign required')
report = ROOT / 'local-evidence/m3-character-actions-live.json'
rows, state = [], {}
started = time.monotonic()
original = pawn.get_actor_transform()
source_mesh, source_anim = pawn.mesh.skeletal_mesh_asset, pawn.mesh.anim_class
body = creator.get_generated_body()


def check(name, ok, **detail):
    if not ok:
        raise RuntimeError(name + ': ' + str(detail))
    rows.append(dict(case=name, passed=True, **detail))


def command(name):
    panel = next(p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
                 if p.is_in_viewport() and p.is_visible())
    panel.call_method('Execute', args=(name,))


def bones(mesh):
    return {n: list((mesh.get_socket_location(n) - pawn.get_actor_location()).to_tuple())
            for n in ('head', 'pelvis', 'hand_l', 'hand_r', 'foot_l', 'foot_r')}


def motion(samples):
    return max(math.dist(a[n], b[n]) for a, b in zip(samples, samples[1:]) for n in a)


def visible(name):
    check(name, creator.get_generated_body() == body and creator.is_appearance_ready()
          and not body.get_editor_property('hidden_in_game') and pawn.mesh.get_editor_property('hidden_in_game'))


def run():
    global body
    while ui.has_modal():
        command('back'); yield .35
    check('world input available', bridge.allows_world_input())
    # Use the established clear dock route, rather than walking into scenery
    # beside whichever dry checkpoint the campaign last recorded.
    pawn.set_actor_location(u.Vector(12500,1800,348),False,True); yield .7
    move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    samples = []
    start = pawn.get_actor_location()
    state['moving'] = True
    for _ in range(45):
        samples.append(bones(body)); yield .025
    state['moving'] = False
    move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    check('native locomotion moves generated character', (pawn.get_actor_location()-start).length() > 50, distance=(pawn.get_actor_location()-start).length())
    check('retargeted locomotion bones animate', motion(samples) > 3, maximum_bone_change_cm=motion(samples))
    visible('appearance retained during locomotion')
    pawn.jump(); yield .2
    check('native jump starts', move.is_falling())
    visible('appearance retained during jump')
    pawn.stop_jumping(); yield 1.5

    targets = [a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalWorldObject)
               if str(a.world_id).endswith('.battery')]
    check('real campaign battery exists', len(targets) == 1)
    target = targets[0]
    before = creator.action_play_count
    for degrees in range(0, 360, 45):
        angle = math.radians(degrees)
        pawn.set_actor_location(target.get_interaction_point() + u.Vector(120*math.cos(angle), 120*math.sin(angle), 25), False, True)
        yield .8
        if bridge.preview_interaction(target).can_interact and move.is_moving_on_ground():
            break
    check('authoritative pickup accepted', bridge.try_interact(target) == u.CoastalActionResult.APPLIED)
    yield .15
    check('pickup starts selected action', str(creator.active_action) == 'pickup' and creator.action_play_count == before+1,
          active=str(creator.active_action), count=creator.action_play_count, before=before)
    samples = []
    for _ in range(8):
        samples.append(bones(body)); yield .1
    check('pickup montage changes generated bones', motion(samples) > 3, maximum_bone_change_cm=motion(samples))
    u.AutomationLibrary.take_high_res_screenshot(1280,720,str(ROOT / 'local-evidence/m3-character-pickup.png'))
    yield .1
    check('movement input accepted',u.CoastalVendorInspection.submit_combat_test_key('D',True))
    # A screenshot may stall a frame; wait in game time so input reaches movement.
    deadline=u.GameplayStatics.get_time_seconds(world)+.15
    while u.GameplayStatics.get_time_seconds(world)<deadline:
        yield .02
    u.CoastalVendorInspection.submit_combat_test_key('D',False)
    check('movement cancels presentation action', str(creator.active_action) == 'None',
          active=str(creator.active_action),velocity=list(move.velocity.to_tuple()),
          acceleration=list(move.get_current_acceleration().to_tuple()),input_ignored=pc.is_move_input_ignored(),
          modal=ui.has_modal(),world_input=bridge.allows_world_input(),mode=str(move.movement_mode),
          position=list(pawn.get_actor_location().to_tuple()),game_time=u.GameplayStatics.get_time_seconds(world))
    move.stop_movement_immediately(); pawn.consume_movement_input_vector(); yield .3
    check('repeat pickup remains idempotent', bridge.try_interact(target) == u.CoastalActionResult.ALREADY_APPLIED)
    yield .2
    check('rejected pickup does not replay', creator.action_play_count == before+1)

    for action, tag in ((u.CoastalCampingAction.WARM_HANDS, 'Coastal.Campsite'),
                        (u.CoastalCampingAction.REST_BY_FIRE, 'Coastal.Campsite'),
                        (u.CoastalCampingAction.REST_IN_SHELTER, 'Coastal.ShelterRest')):
        marker = next(a for a in u.GameplayStatics.get_all_actors_with_tag(world, tag))
        pawn.set_actor_transform(marker.get_actor_transform(), False, True); yield .7
        result = camp.start_action(action)
        check('existing action starts ' + str(action), result == u.CoastalCampingStartResult.STARTED, result=str(result), detail=camp.last_detail)
        samples = []
        for _ in range(12):
            samples.append(bones(body)); yield .15
        visible('appearance retained ' + str(action))
        check('runtime retarget animates ' + str(action), motion(samples) > .2, maximum_bone_change_cm=motion(samples))
        check('source single-node owner retained ' + str(action), pawn.mesh.get_animation_mode() == u.AnimationMode.ANIMATION_SINGLE_NODE)
        camp.cancel_action(); yield .3
        check('source locomotion restored ' + str(action), pawn.mesh.skeletal_mesh_asset == source_mesh and pawn.mesh.anim_class == source_anim)

    authored = json.loads((ROOT / 'local-evidence/m3-coastal-combat-authoring.json').read_text())
    pawn.set_actor_location(u.Vector(*authored['test_player_location']), False, True); yield 1
    check('existing encounter active', combat.is_encounter_active())
    weapon = combat.get_transient_weapon()
    check('existing weapon follows generated hand', weapon and weapon.root_component.get_attach_parent() == body)
    before = creator.action_play_count
    shield = combat.get_shield()
    u.GameplayStatics.apply_damage(pawn, 1.0, pc, pawn, u.DamageType)
    yield .1
    check('authoritative damage applied', combat.get_shield() == shield-1)
    check('damage starts hit reaction', str(creator.active_action) == 'player_hit' and creator.action_play_count == before+1)
    samples = []
    for _ in range(5):
        samples.append(bones(body)); yield .08
    check('hit reaction changes generated bones', motion(samples) > 3, maximum_bone_change_cm=motion(samples))
    check('weapon stays on animated hand', weapon.root_component.get_attach_parent() == body
          and str(weapon.root_component.get_attach_socket_name()) == 'hand_l'
          and (weapon.get_actor_location()-body.get_socket_location('hand_l')).length() < 1)
    ui.open_pause(); yield .2
    check('menu cancels reaction', str(creator.active_action) == 'None')
    command('character_creator'); yield .35
    old_body = body
    command('character_increase'); yield .5
    while creator.is_generating(): yield .2
    body = creator.get_generated_body()
    check('appearance regenerates inside encounter', body != old_body)
    check('regeneration preserves authoritative weapon', combat.get_transient_weapon() == weapon
          and weapon.root_component.get_attach_parent() == body,
          same_weapon=combat.get_transient_weapon() == weapon, valid=u.SystemLibrary.is_valid(weapon),
          parent=weapon.root_component.get_attach_parent().get_path_name() if u.SystemLibrary.is_valid(weapon) else None,
          expected=body.get_path_name())
    command('back'); yield .5
    while creator.is_generating(): yield .2
    body = creator.get_generated_body()
    check('discard preserves weapon and generated attachment', combat.get_transient_weapon() == weapon
          and weapon.root_component.get_attach_parent() == body)
    while ui.has_modal():
        command('back'); yield .3
    visible('appearance retained after all actions')


steps = run()
next_tick = 0.0
report.write_text(json.dumps(dict(status='running', passed=False)))


def tick(delta):
    global next_tick
    if state.get('stepping'): return
    if state.get('moving'):
        pawn.add_movement_input(u.Vector(0,-1,0),1,False)
    if time.monotonic() < next_tick:
        return
    error = None
    try:
        state['stepping'] = True
        if time.monotonic()-started > 180:
            raise RuntimeError('Action test timed out')
        next_tick = time.monotonic()+next(steps)
        return
    except StopIteration:
        pass
    except Exception as exc:
        error = str(exc)
    finally:
        state['stepping'] = False
    u.unregister_slate_post_tick_callback(handle)
    u.CoastalVendorInspection.submit_combat_test_key('D',False)
    move.stop_movement_immediately(); pawn.consume_movement_input_vector(); pawn.stop_jumping()
    camp.cancel_action()
    pawn.set_actor_transform(original, False, True)
    report.write_text(json.dumps(dict(status='finished', passed=error is None, error=error,
        cases=rows, seconds=time.monotonic()-started), indent=2))


handle = u.register_slate_post_tick_callback(tick)
