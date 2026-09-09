"""Capture the generated character's live aiming pose, then restore test positioning."""
import json
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn = u.GameplayStatics.get_player_character(world,0)
pc = pawn.get_controller()
saves = next(s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name())
if not str(saves.get_active_save_set()).startswith('coastal_test_'):
    raise RuntimeError('Disposable campaign required')
original = pawn.get_actor_transform()
rotation = pc.get_control_rotation()
boom = pawn.get_component_by_class(u.SpringArmComponent)
boom_transform = boom.get_relative_transform()
boom_control = boom.get_editor_property('use_pawn_control_rotation')
pawn.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
pawn.consume_movement_input_vector()
pawn.set_actor_location(u.Vector(29200,23000,448),False,True)
pawn.set_actor_rotation(u.Rotator(pitch=0,yaw=0,roll=0),False)
pc.set_control_rotation(u.Rotator(pitch=-12,yaw=0,roll=0))
boom.set_editor_property('use_pawn_control_rotation',False)
boom.set_relative_rotation(u.Rotator(pitch=-12,yaw=120,roll=0),False,False)
started = time.monotonic()
state = dict(phase=0)
destination = ROOT/'local-evidence'/str(globals().get('capture_name', 'm3-combat-animation-aim-side.png'))


def tick(delta):
    now = time.monotonic()-started
    if state['phase']==0 and now>1.5:
        if globals().get('configure_weapon'):
            configure_weapon()
        u.CoastalVendorInspection.submit_combat_test_key('RightMouseButton',True)
        state['phase']=1
    elif state['phase']==1 and now>2.1:
        u.AutomationLibrary.take_high_res_screenshot(1280,720,str(destination))
        state['phase']=2
    elif state['phase']==2 and now>4:
        u.CoastalVendorInspection.submit_combat_test_key('RightMouseButton',False)
        pawn.set_actor_transform(original,False,True)
        pc.set_control_rotation(rotation)
        boom.set_relative_transform(boom_transform,False,True)
        boom.set_editor_property('use_pawn_control_rotation',boom_control)
        u.unregister_slate_post_tick_callback(handle)
        (ROOT/'local-evidence/m3-combat-animation-capture.json').write_text(json.dumps(dict(capture=str(destination),restored=True)))


handle = u.register_slate_post_tick_callback(tick)
