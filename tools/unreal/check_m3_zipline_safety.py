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
report=ROOT/'local-evidence/m3-zipline-safety.json'
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

def run():
    while ui.has_modal(): back(); yield .2
    yield from wait_for(line.is_ready,12,'ready line')
    yield from approach()
    before=pawn.get_actor_transform()
    blocker=obstacle(u.Vector(12599,1700,463),u.Vector(.5,1,.5))
    yield .2
    before=pawn.get_actor_transform()
    check('blocked boarding rejected',not rider.try_board(line))
    check('blocked boarding preserves position',(pawn.get_actor_location()-before.translation).length()<.1)
    check('blocked boarding retains walking',move.is_moving_on_ground() and not rider.is_riding())
    check('blocked boarding has no trolley',rider.trolley.hidden_in_game)
    blocker.destroy_actor(); state['obstacles'].remove(blocker)
    yield .3
    yield from approach()
    yield from board('V')
    blocker=obstacle(pawn.get_actor_location()+u.Vector(220,0,0),u.Vector(.15,1,1.8))
    barrier_x=blocker.get_actor_location().x
    yield from wait_for(lambda:not rider.is_riding(),5,'obstructed ride release')
    check('ride obstruction releases',rider.progress<.9 and 'blocked' in rider.last_detail.lower())
    check('capsule cannot cross obstacle',pawn.get_actor_location().x<barrier_x-30)
    check('obstruction clears trolley',rider.trolley.hidden_in_game)
    blocker.destroy_actor(); state['obstacles'].remove(blocker)
    yield from wait_for(move.is_moving_on_ground,5,'obstructed ride landing')
    yield from approach()
    yield from board('V')
    key('V',True); yield .1
    check('held release ends ride',not rider.is_riding())
    yield .5
    check('held key cannot reboard',not rider.is_riding())
    key('V',False); yield .2
    yield from approach()
    yield from board('V')
    ui.open_pause(); yield .2
    key('V',True); yield .2
    while ui.has_modal(): back(); yield .2
    yield .3
    check('held menu key cannot board on resume',not rider.is_riding())
    key('V',False); yield .2

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
    for actor in state.get('obstacles',[]): actor.destroy_actor()
    rider.release(); move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    restore_view()
    pawn.set_actor_transform(original,False,True); pc.set_control_rotation(control)
    boom.set_relative_transform(boom_transform,False,True)
    boom.set_editor_property('use_pawn_control_rotation',boom_control)
    report.write_text(json.dumps(dict(status='finished',passed=error is None,error=error,cases=rows,
        seconds=time.monotonic()-started),indent=2))

handle=u.register_slate_post_tick_callback(tick)
