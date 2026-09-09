"""Exercise real cable travel and input on the native pawn in a disposable campaign."""
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
rider=pawn.get_component_by_class(u.CoastalZiplineRiderComponent)
creator=pawn.get_component_by_class(u.CoastalMutableCharacterComponent)
grapple=pawn.get_component_by_class(u.CoastalGrappleComponent)
move=pawn.get_component_by_class(u.CharacterMovementComponent)
ui=pc.get_component_by_class(u.CoastalUISessionComponent)
line=u.GameplayStatics.get_all_actors_of_class(world,u.CoastalZipline)[0]
original=pawn.get_actor_transform()
control=pc.get_control_rotation()
boom=pawn.get_component_by_class(u.SpringArmComponent)
boom_transform=boom.get_relative_transform()
boom_control=boom.get_editor_property('use_pawn_control_rotation')
view=pawn.get_component_by_class(u.CameraComponent)
view_parent=view.get_attach_parent()
view_socket=view.get_attach_socket_name()
view_relative=view.get_relative_transform()
view_control=view.get_editor_property('use_pawn_control_rotation')
report=ROOT/'local-evidence/m3-zipline-live.json'
rows,state=[],{}
started=time.monotonic()

def check(name,ok,**detail):
    if not ok: raise RuntimeError(name+': '+str(detail))
    rows.append(dict(case=name,passed=True,**detail))

def key(name,pressed):
    check('input '+name+' '+str(pressed),u.CoastalVendorInspection.submit_combat_test_key(name,pressed))

def wait_for(predicate,seconds,label):
    deadline=time.monotonic()+seconds
    while not predicate():
        if time.monotonic()>deadline:
            raise RuntimeError(label+' timed out: '+line.last_detail+' '+rider.last_detail+
                ' phase='+str(line.cable.get_phase())+' progress='+str(rider.progress))
        yield .05

def back():
    panel=next(p for p in u.WidgetLibrary.get_all_widgets_of_class(world,u.CoastalPanelWidget,False)
               if p.is_in_viewport() and p.is_visible())
    panel.call_method('Execute',args=('back',))

def approach():
    pawn.set_actor_location(u.Vector(12620,1700,348),False,True)
    pawn.set_actor_rotation(u.Rotator(yaw=0),False)
    move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    yield 1
    yield from wait_for(lambda:move.is_moving_on_ground() and not saves.is_busy(),5,'boarding setup')

def restore_view():
    if state.pop('view_detached',False):
        view.attach_to_component(view_parent,view_socket,u.AttachmentRule.KEEP_RELATIVE,
            u.AttachmentRule.KEEP_RELATIVE,u.AttachmentRule.KEEP_RELATIVE,False)
        view.set_relative_transform(view_relative,False,True)
        view.set_editor_property('use_pawn_control_rotation',view_control)

def board(name):
    key(name,True); yield .1; key(name,False)
    yield from wait_for(rider.is_riding,4,'boarding')

def run():
    while ui.has_modal(): back(); yield .2
    yield from wait_for(line.is_ready,12,'plugin cable setup')
    check('real Dynamic Rope cable wrapped',line.cable.get_class().get_name()=='CoastalZiplineRope'
        and line.cable.get_phase()==u.RopePhase.WRAPPED)
    providers=[p for p in u.ObjectIterator(u.RopeStaticBodyProvider) if 'UEDPIE_' in p.get_path_name()]
    ignored=[c for p in providers for c in p.ignored_components]
    check('only obsolete terrain hull excluded',len(ignored)==1 and isinstance(ignored[0],u.StaticMeshComponent)
        and ignored[0].static_mesh.get_name()=='SM_M3CoastalTerrain_CollisionFixed')
    check('GPU solver and grapple distance fields retained',not line.cable.use_world_gdf
        and grapple.get_editor_property('rope').use_world_gdf
        and u.SystemLibrary.get_console_variable_int_value('r.DynamicRope.ForceCPUSolve')==0)
    check('native rider installed',rider is not None)
    pawn.set_actor_location(u.Vector(11500,1800,348),False,True); yield 1
    check('distant boarding rejected',not rider.try_board(line))
    yield from approach()
    yield from board('V')
    check('native custom movement owns ride',move.movement_mode==u.MovementMode.MOVE_CUSTOM
        and move.get_editor_property('custom_movement_mode')==42)
    check('grapple blocked while riding',not grapple.is_grapple_available())
    check('emote blocked while riding',not creator.play_next_emote())
    before=pawn.get_actor_location()
    yield from wait_for(lambda:rider.progress>.18,5,'cable travel')
    anim=creator.get_generated_body().get_anim_instance()
    check('installed two-hand animation blended',anim.activity_pose is not None
        and anim.activity_pose.get_name()=='CA_Two_Handed_Zipline_Anim' and anim.activity_weight>.95)
    check('original trolley visible',rider.trolley is not None and not rider.trolley.hidden_in_game
        and rider.trolley.static_mesh.get_name()=='SM_ZiplineTrolley')
    body=creator.get_generated_body()
    trolley_transform=rider.trolley.get_world_transform()
    grip_offsets={side:list(u.MathLibrary.inverse_transform_location(trolley_transform,
        body.get_socket_location('middle_03_'+side)).to_tuple()) for side in ('l','r')}
    # The continuous handle accommodates the generated body's hand spacing.
    grips={side:(v[0]**2+(v[2]+20)**2+max(0,abs(v[1])-28)**2)**.5 for side,v in grip_offsets.items()}
    check('both hands meet trolley bar',max(grips.values())<2,errors_cm=grips,local_hands=grip_offsets)
    check('rider travels down real cable',pawn.get_actor_location().x-before.x>200,
        displacement=list((pawn.get_actor_location()-before).to_tuple()))
    view.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,False)
    state['view_detached']=True
    view.set_editor_property('use_pawn_control_rotation',False)
    focus=pawn.get_actor_location()+u.Vector(100,0,45)
    eye=focus+u.Vector(30,-350,35)
    view.set_world_location_and_rotation(eye,u.MathLibrary.find_look_at_rotation(eye,focus),False,False)
    yield .2
    u.AutomationLibrary.take_high_res_screenshot(1280,720,str(ROOT/'local-evidence/m3-zipline-ride.png'))
    yield .3
    restore_view()
    yield from wait_for(lambda:not rider.is_riding(),12,'ride completion')
    check('ride reaches endpoint',rider.progress>=.98,progress=rider.progress,detail=rider.last_detail)
    yield from wait_for(move.is_moving_on_ground,5,'native landing')
    check('same pawn controller and movement retained',u.GameplayStatics.get_player_character(world,0)==pawn
        and pawn.get_controller()==pc and pawn.get_component_by_class(u.CharacterMovementComponent)==move)
    yield .3
    check('ride pose retires on landing',anim.activity_pose is None and anim.activity_weight<.05)
    check('completed ride hides trolley',rider.trolley.hidden_in_game)
    boom.set_relative_transform(boom_transform,False,True)
    boom.set_editor_property('use_pawn_control_rotation',boom_control)
    yield from approach()
    yield from board('Gamepad_RightThumbstick')
    yield .4
    key('Gamepad_RightThumbstick',True); yield .1; key('Gamepad_RightThumbstick',False)
    check('gamepad release removes ride',not rider.is_riding() and move.movement_mode==u.MovementMode.MOVE_FALLING)
    yield from wait_for(move.is_moving_on_ground,5,'release landing')
    yield from approach()
    yield from board('V')
    ui.open_pause(); yield .3
    check('menu releases ride',not rider.is_riding() and move.movement_mode!=u.MovementMode.MOVE_CUSTOM)
    check('menu clears ride presentation',anim.activity_pose is None)
    check('menu hides trolley',rider.trolley.hidden_in_game)
    while ui.has_modal(): back(); yield .2
    yield .5
    check('world input restored',pawn.get_component_by_class(u.CoastalInteractionBridge).allows_world_input())

steps=run()
next_tick=0.0
report.write_text(json.dumps(dict(status='running',passed=False)))
def tick(delta):
    global next_tick
    if state.get('stepping') or time.monotonic()<next_tick: return
    error=None
    try:
        state['stepping']=True
        if time.monotonic()-started>120: raise RuntimeError('Zipline test deadline')
        next_tick=time.monotonic()+next(steps)
        return
    except StopIteration: pass
    except Exception as exc: error=str(exc)
    finally: state['stepping']=False
    u.unregister_slate_post_tick_callback(handle)
    for name in ('V','Gamepad_RightThumbstick'):
        u.CoastalVendorInspection.submit_combat_test_key(name,False)
    rider.release(); move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    restore_view()
    pawn.set_actor_transform(original,False,True); pc.set_control_rotation(control)
    boom.set_relative_transform(boom_transform,False,True)
    boom.set_editor_property('use_pawn_control_rotation',boom_control)
    report.write_text(json.dumps(dict(status='finished',passed=error is None,error=error,cases=rows,
        seconds=time.monotonic()-started),indent=2))

handle=u.register_slate_post_tick_callback(tick)
