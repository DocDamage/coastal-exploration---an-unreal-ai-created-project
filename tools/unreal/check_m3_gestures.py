"""Exercise real emote inputs, generated motion, idle variety and interruption in disposable PIE."""
import json
import math
import time
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[3]
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn=u.GameplayStatics.get_player_character(world,0)
pc=pawn.get_controller()
creator=pawn.get_component_by_class(u.CoastalMutableCharacterComponent)
move=pawn.get_component_by_class(u.CharacterMovementComponent)
combat=pawn.get_component_by_class(u.CoastalCombatComponent)
ui=pc.get_component_by_class(u.CoastalUISessionComponent)
saves=next(s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name())
if not str(saves.get_active_save_set()).startswith('coastal_test_') or not creator.is_appearance_ready():
    raise RuntimeError('Ready disposable campaign required')
body=creator.get_generated_body()
original=pawn.get_actor_transform()
rows,state=[],{}
report=ROOT/'local-evidence/m3-gesture-live.json'
started=time.monotonic()

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

def bones():
    return {n:list(body.get_socket_location(n).to_tuple()) for n in ('hand_l','hand_r','head','foot_l','foot_r')}

def game_wait(seconds):
    deadline=u.GameplayStatics.get_time_seconds(world)+seconds
    while u.GameplayStatics.get_time_seconds(world)<deadline:
        yield .1

def run():
    while ui.has_modal():
        back(); yield .3
    pawn.set_actor_location(u.Vector(12500,1800,348),False,True)
    move.stop_movement_immediately(); pawn.consume_movement_input_vector(); yield 3
    check('unarmed and grounded',not combat.is_encounter_active() and move.is_moving_on_ground())
    for index in range(1,5):
        name='G' if index%2 else 'Gamepad_DPad_Down'
        key(name,True); yield .15
        check('emote cycle '+str(index),str(creator.active_action)=='emote_'+str(index),active=str(creator.active_action))
        count=creator.action_play_count
        samples=[bones()]
        for _ in range(5):
            yield .2
            samples.append(bones())
        motion=max(math.dist(a[bone],b[bone]) for a,b in zip(samples,samples[1:]) for bone in samples[0])
        check('visible gesture motion '+str(index),motion>3,change_cm=motion)
        check('held emote does not repeat '+str(index),creator.action_play_count==count)
        u.AutomationLibrary.take_high_res_screenshot(1280,720,str(ROOT/('local-evidence/m3-emote-'+str(index)+'.png')))
        key(name,False)
        yield from game_wait(4.0)
        check('emote completes '+str(index),str(creator.active_action)=='None',active=str(creator.active_action))
    key('G',True); yield .15; key('G',False)
    check('cycle wraps',str(creator.active_action)=='emote_1')
    state['moving']=True; yield .25
    check('movement interrupts emote',str(creator.active_action)=='None')
    check('moving request rejected',not creator.play_next_emote())
    state['moving']=False
    move.stop_movement_immediately(); pawn.consume_movement_input_vector(); yield .4
    pawn.jump(); yield .12
    check('airborne request rejected',not creator.play_next_emote())
    pawn.stop_jumping(); yield 1.5
    key('G',True); yield .15
    check('fresh gesture starts',str(creator.active_action)=='emote_2')
    ui.open_pause(); yield .2
    check('pause cancels gesture',str(creator.active_action)=='None')
    check('modal gesture rejected',not creator.play_next_emote())
    count=creator.action_play_count
    while ui.has_modal():
        back(); yield .3
    yield .2
    check('held key does not resume through menu',creator.action_play_count==count)
    key('G',False); yield .2
    key('G',True); yield .15; key('G',False)
    check('fresh press after menu',str(creator.active_action)=='emote_3')
    yield from game_wait(4)
    yield from game_wait(18.8)
    check('standing idle variation plays',str(creator.active_action)=='idle_variation_relaxed',active=str(creator.active_action))
    state['moving']=True; yield .25
    check('movement interrupts idle variation',str(creator.active_action)=='None')
    state['moving']=False
    move.stop_movement_immediately(); pawn.consume_movement_input_vector()
    pawn.set_actor_location(u.Vector(29200,23000,448),False,True); yield 2
    check('encounter refuses cosmetic emotes',combat.is_encounter_active() and not creator.play_next_emote())

steps=run()
next_tick=0.0
report.write_text(json.dumps(dict(status='running',passed=False)))

def tick(delta):
    global next_tick
    if state.get('stepping'):
        return
    if state.get('moving'):
        pawn.add_movement_input(u.Vector(0,1,0),1,False)
    if time.monotonic()<next_tick:
        return
    error=None
    try:
        state['stepping']=True
        if time.monotonic()-started>240:
            raise RuntimeError('Gesture test timeout')
        next_tick=time.monotonic()+next(steps)
        return
    except StopIteration:
        pass
    except Exception as exc:
        error=str(exc)
    finally:
        state['stepping']=False
    u.unregister_slate_post_tick_callback(handle)
    for name in ('G','Gamepad_DPad_Down'):
        u.CoastalVendorInspection.submit_combat_test_key(name,False)
    move.stop_movement_immediately(); pawn.consume_movement_input_vector(); pawn.stop_jumping()
    pawn.set_actor_transform(original,False,True)
    report.write_text(json.dumps(dict(status='finished',passed=error is None,error=error,cases=rows,
        seconds=time.monotonic()-started),indent=2))

handle=u.register_slate_post_tick_callback(tick)
