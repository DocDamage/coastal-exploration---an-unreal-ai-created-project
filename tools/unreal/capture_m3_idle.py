"""Capture naturally scheduled idle variation and restore the temporary camera angle."""
import json
import time
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[3]
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn=u.GameplayStatics.get_player_character(world,0)
saves=next(s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name())
if not str(saves.get_active_save_set()).startswith('coastal_test_'):
    raise RuntimeError('Disposable campaign required')
creator=pawn.get_component_by_class(u.CoastalMutableCharacterComponent)
boom=pawn.get_component_by_class(u.SpringArmComponent)
original=boom.get_relative_transform()
control=boom.get_editor_property('use_pawn_control_rotation')
boom.set_editor_property('use_pawn_control_rotation',False)
boom.set_relative_rotation(u.Rotator(pitch=-12,yaw=120),False,False)
started=time.monotonic()
state=dict(captured=False)

def tick(delta):
    if state.get('stepping'):
        return
    state['stepping']=True
    try:
        if str(creator.active_action)=='idle_variation_relaxed' and not state['captured']:
            state['captured']=True
            state['capture_time']=time.monotonic()
            u.AutomationLibrary.take_high_res_screenshot(1280,720,str(ROOT/'local-evidence/m3-idle-variation.png'))
        if (state['captured'] and time.monotonic()-state['capture_time']>2) or time.monotonic()-started>70:
            boom.set_relative_transform(original,False,True)
            boom.set_editor_property('use_pawn_control_rotation',control)
            u.unregister_slate_post_tick_callback(handle)
            (ROOT/'local-evidence/m3-idle-capture.json').write_text(json.dumps(dict(captured=state['captured'],restored=True)))
    finally:
        state['stepping']=False

handle=u.register_slate_post_tick_callback(tick)
