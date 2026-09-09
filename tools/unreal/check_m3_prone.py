"""Real native prone movement, body obstruction, standing and save lifecycle in disposable PIE."""
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
prone=pawn.get_component_by_class(u.CoastalProneComponent)
creator=pawn.get_component_by_class(u.CoastalMutableCharacterComponent)
move=pawn.get_component_by_class(u.CharacterMovementComponent)
capsule=pawn.get_component_by_class(u.CapsuleComponent)
grapple=pawn.get_component_by_class(u.CoastalGrappleComponent)
ui=pc.get_component_by_class(u.CoastalUISessionComponent)
original=pawn.get_actor_transform()
control=pc.get_control_rotation()
standing=capsule.get_unscaled_capsule_half_height()
crouch_speed=move.max_walk_speed_crouched
step_height=move.max_step_height
rows,state=[],{}
report=ROOT/'local-evidence/m3-prone-live.json'
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
            raise RuntimeError(label+' timed out: '+prone.last_detail+' cue='+str(prone.pose_cue))
        yield .05

def back():
    p=next(p for p in u.WidgetLibrary.get_all_widgets_of_class(world,u.CoastalPanelWidget,False)
           if p.is_in_viewport() and p.is_visible())
    p.call_method('Execute',args=('back',))

def obstacle(point,scale):
    a=u.CoastalVendorInspection.spawn_zipline_test_obstacle(u.Vector(*point),u.Vector(*scale))
    if a is None: raise RuntimeError('Obstacle guard rejected')
    state.setdefault('obstacles',[]).append(a)
    return a

def run():
    while ui.has_modal(): back(); yield .2
    pawn.set_actor_location(u.Vector(13000,1700,348),False,True)
    pawn.set_actor_rotation(u.Rotator(yaw=0),False)
    pc.set_control_rotation(u.Rotator(yaw=0))
    move.stop_movement_immediately(); yield .5
    yield from wait_for(lambda:move.is_moving_on_ground() and not saves.is_busy(),8,'setup')
    floor=pawn.get_actor_location().z-standing
    check('prone component installed',prone is not None)
    key('C',True); yield .15; key('C',False)
    check('C enters prone',prone.is_prone() and str(prone.pose_cue)=='prone_enter',detail=prone.last_detail)
    check('native crouch lowers capsule',capsule.get_unscaled_capsule_half_height()<standing)
    check('capsule floor preserved',abs(pawn.get_actor_location().z-capsule.get_unscaled_capsule_half_height()-floor)<2)
    yield from wait_for(lambda:str(prone.pose_cue)=='prone_idle',4,'entry animation')
    yield .3
    anim=creator.get_generated_body().get_anim_instance()
    check('retargeted idle presented',anim.activity_pose is not None and anim.activity_pose.get_name()=='CC_anim_Prone_Idle')
    check('grapple gated',not grapple.is_grapple_available())
    check('emote gated',not creator.play_next_emote())
    body=creator.get_generated_body()
    check('prone head lowered',body.get_socket_location('head').z-floor<65,
          height=body.get_socket_location('head').z-floor)
    check('prone hands meet ground',max(abs(body.get_socket_location('hand_'+side).z-floor) for side in ('l','r'))<15)
    view=pawn.get_component_by_class(u.CameraComponent)
    state['view']=(view,view.get_attach_parent(),view.get_attach_socket_name(),view.get_relative_transform(),view.get_editor_property('use_pawn_control_rotation'))
    view.detach_from_component(u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,u.DetachmentRule.KEEP_WORLD,False)
    view.set_editor_property('use_pawn_control_rotation',False)
    focus=pawn.get_actor_location()+u.Vector(0,0,-15)
    eye=focus+u.Vector(20,-320,90)
    view.set_world_location_and_rotation(eye,u.MathLibrary.find_look_at_rotation(eye,focus),False,False)
    yield .2
    u.AutomationLibrary.take_high_res_screenshot(1280,720,str(ROOT/'local-evidence/m3-prone-idle.png'))
    yield .3
    restore_view()
    roof=obstacle((13200,1700,390),(3,2,.2))
    wall=obstacle((13400,1700,320),(.15,2,1.2))
    before=pawn.get_actor_location()
    key('W',True); yield .3
    check('crawl start selected',str(prone.pose_cue).startswith('prone_start'),cue=str(prone.pose_cue))
    yield 3
    check('crawl loop selected',str(prone.pose_cue).startswith('prone_loop'),cue=str(prone.pose_cue))
    check('native crawling moves',pawn.get_actor_location().x-before.x>80,distance=pawn.get_actor_location().x-before.x)
    yield 2
    check('long body cannot cross wall',pawn.get_actor_location().x<13300,x=pawn.get_actor_location().x)
    key('W',False); yield .2
    yield from wait_for(lambda:str(prone.pose_cue)=='prone_idle',3,'crawl stop')
    check('roof rejects standing',not prone.try_stand(),detail=prone.last_detail)
    check('blocked stand retains low capsule',prone.is_prone() and capsule.get_unscaled_capsule_half_height()<standing)
    check('save under low cover succeeds',saves.save_now()==u.CoastalSaveResult.SAVED)
    check('reload under low cover succeeds',saves.load_campaign(str(saves.get_active_save_set()))==u.CoastalSaveResult.LOADED)
    yield 1
    check('reload retires stance and restores capsule',not prone.is_prone() and abs(capsule.get_unscaled_capsule_half_height()-standing)<.1)
    check('reload remains recoverable',not saves.is_recovery_required())
    for a in state.pop('obstacles',[]): a.destroy_actor()
    while ui.has_modal(): back(); yield .2
    pawn.set_actor_location(u.Vector(13000,1700,348),False,True); move.stop_movement_immediately(); yield .5
    yield from wait_for(lambda:creator.is_appearance_ready() and not creator.is_generating() and move.is_moving_on_ground(),8,'reload ready')
    key('Gamepad_FaceButton_Right',True); yield .1; key('Gamepad_FaceButton_Right',False)
    yield from wait_for(lambda:str(prone.pose_cue)=='prone_idle',4,'controller entry')
    check('clear stand accepted',prone.try_stand())
    check('exit clip selected',str(prone.pose_cue)=='prone_exit')
    yield from wait_for(lambda:not prone.is_prone(),4,'exit animation')
    check('standing capsule restored',abs(capsule.get_unscaled_capsule_half_height()-standing)<.1)
    check('crouch speed and steps restored',move.max_walk_speed_crouched==crouch_speed and move.max_step_height==step_height)
    check('pawn controller and movement preserved',pawn.get_controller()==pc and pawn.get_component_by_class(u.CharacterMovementComponent)==move)

steps=run(); next_tick=0.
def restore_view():
    if 'view' in state:
        view,parent,socket,relative,control_flag=state.pop('view')
        view.attach_to_component(parent,socket,u.AttachmentRule.KEEP_RELATIVE,u.AttachmentRule.KEEP_RELATIVE,u.AttachmentRule.KEEP_RELATIVE,False)
        view.set_relative_transform(relative,False,True)
        view.set_editor_property('use_pawn_control_rotation',control_flag)
report.write_text(json.dumps(dict(status='running',passed=False)),encoding='utf-8')
def tick(delta):
    global next_tick
    if state.get('stepping') or time.monotonic()<next_tick: return
    error=None
    try:
        state['stepping']=True
        if time.monotonic()-started>120: raise RuntimeError('Prone deadline')
        next_tick=time.monotonic()+next(steps); return
    except StopIteration: pass
    except Exception as exc: error=str(exc)
    finally: state['stepping']=False
    u.unregister_slate_post_tick_callback(handle)
    for name in ('C','W','Gamepad_FaceButton_Right'): u.CoastalVendorInspection.submit_combat_test_key(name,False)
    for a in state.get('obstacles',[]): a.destroy_actor()
    restore_view()
    if prone.is_prone(): prone.try_stand()
    move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    pawn.set_actor_transform(original,False,True); pc.set_control_rotation(control)
    report.write_text(json.dumps(dict(status='finished',passed=error is None,error=error,cases=rows,
        seconds=time.monotonic()-started),indent=2),encoding='utf-8')
handle=u.register_slate_post_tick_callback(tick)
