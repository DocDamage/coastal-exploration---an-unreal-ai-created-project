"""Exercise the actual Dynamic Rope grapple against the authored gantry in a disposable campaign."""
import json
import time
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[3]
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn=u.GameplayStatics.get_player_character(world,0)
pc=pawn.get_controller()
saves=next(s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name())
if not str(saves.get_active_save_set()).startswith('coastal_test_'):
    raise RuntimeError('Disposable campaign required')
grapple=pawn.get_component_by_class(u.CoastalGrappleComponent)
if not grapple:
    raise RuntimeError('Native grapple component missing')
rope=grapple.get_editor_property('rope')
creator=pawn.get_component_by_class(u.CoastalMutableCharacterComponent)
ui=pc.get_component_by_class(u.CoastalUISessionComponent)
move=pawn.get_component_by_class(u.CharacterMovementComponent)
original=pawn.get_actor_transform()
rotation=pc.get_control_rotation()
boom=pawn.get_component_by_class(u.SpringArmComponent)
boom_transform=boom.get_relative_transform()
boom_control=boom.get_editor_property('use_pawn_control_rotation')
boom_length=boom.target_arm_length
view=pawn.get_component_by_class(u.CameraComponent)
view_parent=view.get_attach_parent()
view_socket=view.get_attach_socket_name()
view_relative=view.get_relative_transform()
view_control=view.get_editor_property('use_pawn_control_rotation')
rows,state=[],{}
started=time.monotonic()
report=ROOT/'local-evidence/m3-grapple-live.json'

def check(name,ok,**detail):
    if not ok:
        raise RuntimeError(name+': '+str(detail))
    rows.append(dict(case=name,passed=True,**detail))

def key(name,down):
    check('input '+name+' '+str(down),u.CoastalVendorInspection.submit_combat_test_key(name,down))

def back():
    panel=next(p for p in u.WidgetLibrary.get_all_widgets_of_class(world,u.CoastalPanelWidget,False)
        if p.is_in_viewport() and p.is_visible())
    panel.call_method('Execute',args=('back',))

def wait_for(predicate,seconds,label):
    deadline=time.monotonic()+seconds
    while not predicate():
        if time.monotonic()>deadline:
            raise RuntimeError(label+' timed out; phase='+str(rope.get_phase())+' '+grapple.last_detail)
        yield .1

def restore_view():
    if state.pop('view_detached',False):
        view.attach_to_component(view_parent,view_socket,u.AttachmentRule.KEEP_RELATIVE,
            u.AttachmentRule.KEEP_RELATIVE,u.AttachmentRule.KEEP_RELATIVE,False)
        view.set_relative_transform(view_relative,False,True)
        view.set_editor_property('use_pawn_control_rotation',view_control)

def run():
    while ui.has_modal():
        back(); yield .3
    pawn.set_actor_location(u.Vector(12500,1800,348),False,True)
    move.stop_movement_immediately(); pawn.consume_movement_input_vector(); yield 2
    check('native plugin component present',rope.get_class().get_name()=='CoastalGrappleRope')
    check('grapple ready',grapple.is_grapple_available())
    check('rope attached to generated hand',rope.get_attach_parent()==creator.get_generated_body())
    check('idle hook stays stowed after appearance refresh',rope.get_tip_mesh_component().hidden_in_game)
    pc.set_control_rotation(u.Rotator(pitch=65,yaw=180)); yield .5
    key('Q',True); yield .15; key('Q',False); yield .3
    check('open space refuses grapple',not grapple.is_grapple_deployed())
    anchors=[a for a in u.GameplayStatics.get_all_actors_of_class(world,u.CoastalGrappleAnchor)
             if a.get_actor_label()=='coastal_rope - Grapple gantry']
    check('authored gantry present',len(anchors)==1)
    anchor=anchors[0]
    camera=u.GameplayStatics.get_player_camera_manager(world,0)
    pc.set_control_rotation(u.MathLibrary.find_look_at_rotation(camera.get_camera_location(),anchor.get_actor_location()))
    yield .5
    pc.set_control_rotation(u.MathLibrary.find_look_at_rotation(camera.get_camera_location(),anchor.get_actor_location()))
    yield from wait_for(lambda:grapple.get_aim_hud_sample().has_target,8,'anchor targeting')
    check('plugin identifies authored anchor',grapple.get_aim_hud_sample().mesh.get_owner()==anchor)
    key('Q',True); yield .15; key('Q',False)
    yield from wait_for(lambda:rope.get_phase()==u.RopePhase.WRAPPED,10,'committed grapple')
    check('plugin rope wrapped',grapple.is_grapple_deployed())
    tip=rope.get_tip_mesh_component()
    check('original grapnel visible at wrap',tip is not None and not tip.hidden_in_game
        and tip.static_mesh.get_name()=='SM_Grapnel')
    check('grapnel contact sockets authored',tip.does_socket_exist('HookPoint') and tip.does_socket_exist('RopeEye'))
    yield .3
    eye_error=(tip.get_socket_location('RopeEye')-rope.get_node_position(rope.get_node_count()-1)).length()
    check('rope endpoint meets grapnel eye',eye_error<.5,error_cm=eye_error)
    check('emote refused during grapple',not creator.play_next_emote())
    before_length=rope.get_current_rope_length()
    before_distance=(pawn.get_actor_location()-anchor.get_actor_location()).length()
    before_location=pawn.get_actor_location()
    key('Z',True)
    yield from wait_for(lambda:(pawn.get_actor_location()-before_location).length()>100
        and move.movement_mode==u.MovementMode.MOVE_FALLING and grapple.is_hanging_on_rope(),12,'airborne reel movement')
    key('Z',False); yield .2
    check('reel shortens plugin rope',rope.get_current_rope_length()<before_length-30,
        before=before_length,after=rope.get_current_rope_length())
    check('plugin moves native character toward anchor',(pawn.get_actor_location()-anchor.get_actor_location()).length()<before_distance-50,
        before=before_distance,after=(pawn.get_actor_location()-anchor.get_actor_location()).length(),movement_mode=str(move.movement_mode))
    anim=creator.get_generated_body().get_anim_instance()
    yield from wait_for(lambda:grapple.is_hanging_on_rope() and anim.activity_weight>.95,5,'animated rope hang')
    check('retargeted hang fully blended',anim.activity_pose.get_name()=='CA_Hold_Rope_Idle_Anim')
    check('plugin regrips both animated hands',grapple.is_hang_socket_swapped()
        and str(rope.get_attach_socket_name())=='middle_03_r')
    eye_error=(tip.get_socket_location('RopeEye')-rope.get_node_position(rope.get_node_count()-1)).length()
    check('rope eye stays connected under reel tension',eye_error<.5,error_cm=eye_error)
    hook_position=tip.get_socket_location('HookPoint')
    swing_start=pawn.get_actor_location()
    key('D',True)
    game_start=u.GameplayStatics.get_time_seconds(world)
    yield from wait_for(lambda:u.GameplayStatics.get_time_seconds(world)-game_start>.7,5,'swing input')
    key('D',False); yield .2
    swing_delta=pawn.get_actor_location()-swing_start
    check('airborne steering moves along swing',abs(swing_delta.y)>10,delta=list(swing_delta.to_tuple()))
    check('embedded hook stays fixed while character swings',(tip.get_socket_location('HookPoint')-hook_position).length()<.1)
    boom.set_editor_property('use_pawn_control_rotation',False)
    boom.set_relative_rotation(u.Rotator(pitch=-12,yaw=120),False,False)
    yield .4
    u.AutomationLibrary.take_high_res_screenshot(1280,720,str(ROOT/'local-evidence/m3-grapple-reel.png'))
    yield .3
    view.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,False)
    state['view_detached']=True
    view.set_editor_property('use_pawn_control_rotation',False)
    focus=tip.get_socket_location('RopeEye')
    eye=focus+u.Vector(-120,-170,-80)
    view.set_world_location_and_rotation(eye,u.MathLibrary.find_look_at_rotation(eye,focus),False,False)
    yield .3
    u.AutomationLibrary.take_high_res_screenshot(1280,720,str(ROOT/'local-evidence/m3-grapnel-contact.png'))
    yield .3
    restore_view()
    boom.set_relative_transform(boom_transform,False,True)
    boom.target_arm_length=boom_length
    length=rope.get_current_rope_length()
    key('X',True)
    yield from wait_for(lambda:rope.get_current_rope_length()>length+100,5,'keyboard reel out')
    key('X',False); yield .2
    check('keyboard reel out extends rope',rope.get_current_rope_length()>length+100)
    length=rope.get_current_rope_length()
    key('Gamepad_DPad_Right',True)
    yield from wait_for(lambda:rope.get_current_rope_length()<length-100,5,'gamepad reel in')
    key('Gamepad_DPad_Right',False); yield .2
    check('gamepad reel in shortens rope',rope.get_current_rope_length()<length-100)
    length=rope.get_current_rope_length()
    key('Gamepad_DPad_Left',True)
    yield from wait_for(lambda:rope.get_current_rope_length()>length+100,5,'gamepad reel out')
    key('Gamepad_DPad_Left',False); yield .2
    check('gamepad reel out extends rope',rope.get_current_rope_length()>length+100)
    key('Gamepad_DPad_Up',True); yield .15; key('Gamepad_DPad_Up',False)
    yield from wait_for(lambda:not grapple.is_grapple_deployed(),5,'gamepad release')
    yield .3
    check('release clears hanging pose',anim.activity_pose is None and anim.activity_weight<.05)
    check('released hook stowed',tip.hidden_in_game)
    pawn.set_actor_location(u.Vector(12500,1800,348),False,True)
    pawn.set_actor_rotation(original.rotation.rotator(),False)
    move.stop_movement_immediately(); yield 2
    boom.set_relative_transform(boom_transform,False,True)
    boom.set_editor_property('use_pawn_control_rotation',boom_control)
    boom.target_arm_length=boom_length
    # Reacquire after swing rotation and the detached detail camera have been restored.
    for _ in range(5):
        pc.set_control_rotation(u.MathLibrary.find_look_at_rotation(camera.get_camera_location(),anchor.get_actor_location()))
        yield .2
    yield from wait_for(lambda:grapple.get_aim_hud_sample().has_target,8,'gamepad target')
    key('Gamepad_DPad_Up',True); yield .15; key('Gamepad_DPad_Up',False)
    yield from wait_for(lambda:rope.get_phase()==u.RopePhase.WRAPPED,10,'gamepad throw')
    check('gamepad throws plugin grapple',grapple.is_grapple_deployed())
    ui.open_pause(); yield .25
    check('menu releases grapple',not grapple.is_grapple_deployed())
    check('menu suppresses rope input',grapple.is_rope_input_suppressed())
    check('menu stops reel',abs(rope.get_reel_rate())<.01)
    check('menu removes plugin movement constraint',rope.constrain_wielder_location(pawn.get_actor_location()+u.Vector(0,0,3000)) is None)
    while ui.has_modal():
        back(); yield .3
    yield .2
    check('same pawn and controller retained',u.GameplayStatics.get_player_character(world,0)==pawn and pawn.get_controller()==pc)

steps=run()
next_tick=0.0
report.write_text(json.dumps(dict(status='running',passed=False)))

def tick(delta):
    global next_tick
    if state.get('stepping') or time.monotonic()<next_tick:
        return
    error=None
    try:
        state['stepping']=True
        if time.monotonic()-started>120:
            raise RuntimeError('Grapple test timeout')
        next_tick=time.monotonic()+next(steps)
        return
    except StopIteration:
        pass
    except Exception as exc:
        error=str(exc)
    finally:
        state['stepping']=False
    u.unregister_slate_post_tick_callback(handle)
    for name in ('Q','Z','X','D','Gamepad_DPad_Up','Gamepad_DPad_Left','Gamepad_DPad_Right'):
        u.CoastalVendorInspection.submit_combat_test_key(name,False)
    grapple.cancel_grapple()
    restore_view()
    move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    pawn.set_actor_transform(original,False,True); pc.set_control_rotation(rotation)
    boom.set_relative_transform(boom_transform,False,True)
    boom.set_editor_property('use_pawn_control_rotation',boom_control)
    boom.target_arm_length=boom_length
    report.write_text(json.dumps(dict(status='finished',passed=error is None,error=error,cases=rows,
        seconds=time.monotonic()-started),indent=2))

handle=u.register_slate_post_tick_callback(tick)
