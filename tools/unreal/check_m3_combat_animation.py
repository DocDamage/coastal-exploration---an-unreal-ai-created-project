"""Verify accepted combat events, directional clips, moving legs and input interruption in PIE."""
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
combat = pawn.get_component_by_class(u.CoastalCombatComponent)
move = pawn.get_component_by_class(u.CharacterMovementComponent)
saves = next(s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name())
if not str(saves.get_active_save_set()).startswith('coastal_test_') or not creator.is_appearance_ready():
    raise RuntimeError('Ready generated character in disposable campaign required')
report = ROOT / 'local-evidence/m3-combat-animation-live.json'
original = pawn.get_actor_transform()
body = creator.get_generated_body()
source = next(a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalCombatTarget)
              if 'Clear training target' in a.get_actor_label())
source_transform = source.get_actor_transform()
source_mobility = source.root_component.mobility
sentries = [(a, a.engagement_range_cm) for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalCombatSentry)]
rows, state = [], {}
started = time.monotonic()


def check(name, ok, **detail):
    if not ok:
        raise RuntimeError(name + ': ' + str(detail))
    rows.append(dict(case=name, passed=True, **detail))


def key(name, pressed):
    check('input ' + name + ' ' + str(pressed), u.CoastalVendorInspection.submit_combat_test_key(name, pressed))


def back():
    panel = next(p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
                 if p.is_in_viewport() and p.is_visible())
    panel.call_method('Execute', args=('back',))


def bones():
    return {n: tuple((body.get_socket_location(n)-pawn.get_actor_location()).to_tuple())
            for n in ('hand_r', 'hand_l', 'foot_l', 'foot_r')}


def game_wait(seconds):
    deadline = u.GameplayStatics.get_time_seconds(world) + seconds
    while u.GameplayStatics.get_time_seconds(world) < deadline:
        yield .1


def run():
    while ui.has_modal():
        back(); yield .3
    for actor, _ in sentries:
        actor.set_editor_property('engagement_range_cm', 1.0)
    source.root_component.set_mobility(u.ComponentMobility.MOVABLE)
    pawn.set_actor_location(u.Vector(12500,1800,348),False,True); yield .6
    check('outside encounter', not combat.is_encounter_active())
    pawn.set_actor_location(u.Vector(29200,23000,448),False,True)
    yield .15
    check('equip animation on encounter entry', str(creator.active_action) == 'pistol_equip',
          active=str(creator.active_action), weapon=bool(combat.get_transient_weapon()),
          grounded=move.is_moving_on_ground(), busy=saves.is_busy(), generating=creator.is_generating(),
          detail=creator.last_detail)
    yield 2
    pawn.set_actor_rotation(u.Rotator(0,0,0),False)
    move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    weapon = combat.get_transient_weapon()
    check('weapon follows generated hand', weapon.root_component.get_attach_parent() == body)
    check('weapon follows animated left hand', str(weapon.root_component.get_attach_socket_name()) == 'hand_l')
    grapple=pawn.get_component_by_class(u.CoastalGrappleComponent)
    check('armed combat suppresses grapple',not grapple.is_grapple_available() and grapple.is_rope_input_suppressed())
    check('combat clears full-body hang layer',body.get_anim_instance().activity_pose is None
        and body.get_anim_instance().activity_weight<.05)
    visual = next(c for c in weapon.get_components_by_class(u.StaticMeshComponent) if c.get_name() == 'CoastalWeaponVisual')
    check('assembled pistol installed', visual.static_mesh.get_name() == 'SM_CoastalPistol')
    muzzle = next(c for c in weapon.get_components_by_class(u.SceneComponent) if c.get_name() == 'Muzzle')
    barrel_tip = u.MathLibrary.transform_location(visual.get_world_transform(), u.Vector(14.581, 0, 4.45))
    check('muzzle follows visible barrel tip', (muzzle.get_world_location()-barrel_tip).length() < .1)
    check('hold pose installed', 'Hold_Loop' in body.get_anim_instance().get_editor_property('WeaponPose').get_name())
    for yaw, direction in ((0,'Front'),(90,'Right'),(180,'Back'),(-90,'Left')):
        pc.set_control_rotation(u.Rotator(pitch=0,yaw=yaw,roll=0))
        key('RightMouseButton',True); yield .45
        check('mouse aim ' + direction, combat.is_aiming())
        pose = body.get_anim_instance().get_editor_property('WeaponPose')
        check('directional aim ' + direction, pose and pose.get_name().endswith('Point_' + direction), pose=pose.get_name() if pose else None)
        yield from game_wait(1.0)
        anim = body.get_anim_instance()
        end_time = anim.get_editor_property('WeaponPoseTime')
        check('aim transition reaches final pose ' + direction, abs(end_time-pose.get_play_length()) < .03, time=end_time)
        yield from game_wait(.45)
        check('held aim does not loop ' + direction, abs(anim.get_editor_property('WeaponPoseTime')-end_time) < .01)
        before = combat.get_current_ammo()
        before_shot_pose = bones()
        key('LeftMouseButton',True); yield .1
        key('LeftMouseButton',False)
        check('accepted shot spends one round ' + direction, combat.get_current_ammo() == before-1)
        check('directional shot ' + direction, str(creator.active_action) == 'pistol_shoot_' + direction.lower(), active=str(creator.active_action))
        after_shot_pose = bones()
        recoil = max(math.dist(before_shot_pose[n],after_shot_pose[n]) for n in ('hand_l','hand_r'))
        check('firing changes generated hands ' + direction, recoil > 1, change_cm=recoil)
        key('RightMouseButton',False); yield .6
    key('Gamepad_LeftTrigger',True); yield .2
    check('gamepad aim', combat.is_aiming())
    key('Gamepad_LeftTrigger',False); yield .2
    check('aim release', not combat.is_aiming())
    for yaw in (0,90):
        pawn.set_actor_rotation(u.Rotator(pitch=0,yaw=yaw,roll=0),False)
        for dx,dy,cue in ((1,0,'player_hit'),(-1,0,'player_hit_back'),(0,1,'player_hit_right'),(0,-1,'player_hit_left')):
            rad = math.radians(yaw)
            offset = u.Vector(dx*math.cos(rad)-dy*math.sin(rad), dx*math.sin(rad)+dy*math.cos(rad),0)*500
            source.set_actor_location(pawn.get_actor_location()+offset,False,True)
            before = combat.get_shield()
            before_count = creator.action_play_count
            before_time = u.GameplayStatics.get_time_seconds(world)
            before_hit_pose = bones()
            u.GameplayStatics.apply_damage(pawn,1.0,pc,source,u.DamageType); yield .12
            check('accepted damage ' + cue, combat.get_shield() == before-1)
            check('facing-relative reaction ' + str(yaw) + ' ' + cue, str(creator.active_action) == cue,
                  active=str(creator.active_action), count_before=before_count,count_after=creator.action_play_count,
                  game_seconds=u.GameplayStatics.get_time_seconds(world)-before_time,busy=saves.is_busy(),
                  generating=creator.is_generating(),grounded=move.is_moving_on_ground(),mode=str(pawn.mesh.get_animation_mode()))
            samples = []
            for _ in range(5):
                samples.append(bones()); yield .08
            # Include onset: most of a brief impact happens before the first
            # post-hit observation, not between two samples during its recovery.
            change = max(math.dist(before_hit_pose[n],sample[n]) for sample in samples for n in ('hand_l','hand_r'))
            check('reaction has visible hand displacement ' + cue, change > 3, change_cm=change)
            yield 1.2
    source.set_actor_transform(source_transform,False,True)
    pc.set_control_rotation(u.Rotator(0,0,0))
    state['moving'] = True
    samples = []
    start = pawn.get_actor_location()
    for _ in range(10):
        samples.append(bones()); yield .08
    key('LeftMouseButton',True); yield .1
    key('LeftMouseButton',False)
    check('shooting while walking', str(creator.active_action).startswith('pistol_shoot'))
    check('native character keeps moving', (pawn.get_actor_location()-start).length() > 50)
    foot_change = max(math.dist(a['foot_l'],b['foot_l']) for a,b in zip(samples,samples[1:]))
    check('lower body still animates', foot_change > 3, change_cm=foot_change)
    source.set_actor_location(pawn.get_actor_location()+pawn.get_actor_right_vector()*500,False,True)
    u.GameplayStatics.apply_damage(pawn,1.0,pc,source,u.DamageType); yield .12
    check('directional reaction while moving', str(creator.active_action)=='player_hit_right',active=str(creator.active_action))
    check('moving reaction preserves locomotion',pawn.get_velocity().length()>50)
    state['moving'] = False
    move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    pawn.jump(); yield .2
    check('native jump still starts while armed',move.is_falling())
    before = combat.get_current_ammo()
    check('airborne shot accepted',combat.try_fire()==u.CoastalCombatResult.APPLIED)
    yield .1
    check('airborne firing clip and ammo agree',str(creator.active_action).startswith('pistol_shoot')
          and combat.get_current_ammo()==before-1)
    pawn.stop_jumping(); yield 1.5
    key('RightMouseButton',True); yield .2
    ui.open_pause(); yield .2
    check('pause cancels action and aim', str(creator.active_action) == 'None' and not combat.is_aiming())
    count = creator.action_play_count
    check('paused fire refused', combat.try_fire() != u.CoastalCombatResult.APPLIED)
    yield .15
    check('refused fire has no animation', creator.action_play_count == count)
    while ui.has_modal():
        back(); yield .3
    check('held aim cannot resume through menu', not combat.is_aiming())
    key('RightMouseButton',False); yield .15
    key('RightMouseButton',True); yield .15
    check('fresh aim after interruption', combat.is_aiming())
    key('RightMouseButton',False)
    pawn.set_actor_location(u.Vector(12500,1800,348),False,True); yield .1
    check('unequip animation', str(creator.active_action) == 'pistol_unequip', active=str(creator.active_action))
    check('weapon removed on encounter exit', not combat.get_transient_weapon())
    yield 3
    check('upper layer retires', body.get_anim_instance().get_editor_property('WeaponPose') is None)


steps = run()
next_tick = 0.0
report.write_text(json.dumps(dict(status='running',passed=False)))


def tick(delta):
    global next_tick
    if state.get('stepping'):
        return
    if state.get('moving'):
        pawn.add_movement_input(u.Vector(0,-1,0),1,False)
    if time.monotonic() < next_tick:
        return
    error = None
    try:
        state['stepping'] = True
        if time.monotonic()-started > 150:
            raise RuntimeError('Combat animation test timeout')
        next_tick = time.monotonic()+next(steps)
        return
    except StopIteration:
        pass
    except Exception as exc:
        error = str(exc)
    finally:
        state['stepping'] = False
    u.unregister_slate_post_tick_callback(handle)
    for name in ('RightMouseButton','LeftMouseButton','Gamepad_LeftTrigger'):
        u.CoastalVendorInspection.submit_combat_test_key(name,False)
    move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    pawn.stop_jumping()
    source.set_actor_transform(source_transform,False,True)
    source.root_component.set_mobility(source_mobility)
    for actor, distance in sentries:
        actor.set_editor_property('engagement_range_cm',distance)
    pawn.set_actor_transform(original,False,True)
    report.write_text(json.dumps(dict(status='finished',passed=error is None,error=error,cases=rows,seconds=time.monotonic()-started),indent=2))


handle = u.register_slate_post_tick_callback(tick)
