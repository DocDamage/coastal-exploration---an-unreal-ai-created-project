"""Exercise endpoint loss, appearance regeneration and campaign reload during real rides."""
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
report=ROOT/'local-evidence/m3-zipline-lifecycle.json'
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
    yield .1
    yield from wait_for(rider.is_riding,4,'boarding')

def obstacle(point,scale):
    actor=u.CoastalVendorInspection.spawn_zipline_test_obstacle(point,scale)
    if actor is None: raise RuntimeError('Guarded obstacle spawn rejected')
    state.setdefault('obstacles',[]).append(actor)
    return actor

def command(name):
    panel=next(p for p in u.WidgetLibrary.get_all_widgets_of_class(world,u.CoastalPanelWidget,False)
               if p.is_in_viewport() and p.is_visible())
    panel.call_method('Execute',args=(name,))

def run():
    while ui.has_modal(): back(); yield .2
    yield from wait_for(line.is_ready,12,'ready line')
    yield from approach()
    yield from board('V')
    state['endpoint']=line.get_editor_property('end_anchor')
    line.set_editor_property('end_anchor',None)
    yield .3
    check('missing endpoint releases rider',not rider.is_riding())
    check('missing endpoint clears trolley',rider.trolley.hidden_in_game)
    check('missing endpoint uses native fall',move.is_falling() or move.is_moving_on_ground())
    check('missing endpoint rejects boarding',not rider.try_board(line))
    line.set_editor_property('end_anchor',state.pop('endpoint'))
    yield from wait_for(line.is_ready,5,'restored endpoint')
    yield from approach()
    yield from board('V')
    old_body=creator.get_generated_body()
    ui.open_pause(); yield .3
    check('creator menu entry releases ride',not rider.is_riding() and rider.trolley.hidden_in_game)
    while ui.has_modal(): back(); yield .2
    yield from wait_for(move.is_moving_on_ground,5,'landing before creator')
    ui.open_pause(); yield .3
    command('character_creator'); yield .3
    command('character_increase'); yield 1
    yield from wait_for(lambda:creator.is_appearance_ready() and not creator.is_generating()
                        and creator.get_generated_body()!=old_body,8,'appearance regeneration')
    check('appearance preview cannot board',not rider.try_board(line))
    command('back'); yield 1
    while ui.has_modal(): back(); yield .2
    yield from wait_for(lambda:creator.is_appearance_ready() and not creator.is_generating(),8,'appearance restore')
    check('appearance refresh preserves stowed hook',grapple.rope.get_tip_mesh_component().hidden_in_game)
    yield from approach()
    yield from board('V')
    yield .3
    anim=creator.get_generated_body().get_anim_instance()
    check('regenerated appearance restores ride pose',anim.activity_pose is not None
          and anim.activity_pose.get_name()=='CA_Two_Handed_Zipline_Anim' and anim.activity_weight>.95)
    check('regenerated appearance restores trolley',not rider.trolley.hidden_in_game)
    check('reload during ride accepted',saves.load_campaign(str(saves.get_active_save_set()))==u.CoastalSaveResult.LOADED)
    yield 1
    yield from wait_for(lambda:creator.is_appearance_ready() and not creator.is_generating(),8,'reload appearance')
    check('campaign reload cancels ride',not rider.is_riding() and rider.trolley.hidden_in_game)
    check('campaign reload restores native movement',move.is_moving_on_ground() or move.is_falling())
    check('campaign reload preserves pawn and movement',u.GameplayStatics.get_player_character(world,0)==pawn
          and pawn.get_controller()==pc
          and pawn.get_component_by_class(u.CharacterMovementComponent)==move)
    while ui.has_modal(): back(); yield .2
    yield from approach()
    yield from board('V')
    check('new session epoch can board',rider.is_riding())
    recovery=pawn.get_component_by_class(u.CoastalPlayerRecoveryComponent)
    check('native recovery accepts return during ride',recovery.request_defeat_return())
    yield .2
    check('native recovery cancels ride',not rider.is_riding() and rider.trolley.hidden_in_game)
    yield from wait_for(lambda:not recovery.is_returning() and not saves.is_player_return_active(),8,'native return')
    check('native return preserves usable campaign',not saves.is_recovery_required()
          and pawn.get_component_by_class(u.CoastalInteractionBridge).allows_world_input())
    yield from wait_for(move.is_moving_on_ground,5,'native return ground')
    check('native return retires ride pose',creator.get_generated_body().get_anim_instance().activity_pose is None)

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
    if 'endpoint' in state: line.set_editor_property('end_anchor',state.pop('endpoint'))
    rider.release(); move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    restore_view()
    pawn.set_actor_transform(original,False,True); pc.set_control_rotation(control)
    boom.set_relative_transform(boom_transform,False,True)
    boom.set_editor_property('use_pawn_control_rotation',boom_control)
    report.write_text(json.dumps(dict(status='finished',passed=error is None,error=error,cases=rows,
        seconds=time.monotonic()-started),indent=2))

handle=u.register_slate_post_tick_callback(tick)
