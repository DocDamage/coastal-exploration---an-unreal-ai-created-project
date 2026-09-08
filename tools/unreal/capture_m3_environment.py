"""Capture editor composition only after the M3 dressing recipe has saved."""
import time
from pathlib import Path
import unreal as u

OUT = Path(__file__).resolve().parents[3] / 'local-evidence'
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
views = [('cabin', (800, -1500, 1050), (-19, 150, 0)),
         ('shore', (4000, -2500, 1900), (-22, 80, 0)),
         ('dock', (13200, -1200, 900), (-20, 130, 0))]
views += globals().get('extra_views', [])
prefix = globals().get('capture_prefix', 'm3-environment')
index, deadline, task, capturing = 0, time.monotonic() + 10, None, False


def tick(delta):
    global index, deadline, task, capturing
    if capturing or time.monotonic() < deadline or (task is not None and not task.is_task_done()):
        return
    if index == len(views):
        u.unregister_slate_post_tick_callback(handle)
        u.EditorPythonScripting.set_keep_python_script_alive(False)
        u.log('COASTAL_M3_ENVIRONMENT_CAPTURE_COMPLETE')
        u.SystemLibrary.quit_editor()
        return
    name, position, rotation = views[index]
    editor.set_level_viewport_camera_info(u.Vector(*position), u.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    capturing = True
    try:
        task = u.AutomationLibrary.take_high_res_screenshot(1600, 900, str(OUT / (prefix + '-' + name + '.png')), delay=5.0)
        index += 1
        deadline = time.monotonic() + 10
    finally:
        capturing = False


handle = u.register_slate_post_tick_callback(tick)
u.EditorPythonScripting.set_keep_python_script_alive(True)
