"""Legacy editor capture experiment; use capture_expansion_pie.py for verified files.

Ordinary Shot requests in an editor-only world may produce no image. A request
receipt below does not establish that the renderer wrote a screenshot.
"""
import time
from pathlib import Path
import unreal as u

OUT=Path('F:/coastline/local-evidence')
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
views=globals().get('views',[
    ('hallsands',(-4000,8000,9000),(-19000,22000,2200)),
    ('baelo',(14500,15000,6600),(8000,23000,800)),
    ('powell',(30200,300,1900),(27000,3600,500)),
    ('harbour',(25800,19000,2800),(30200,23000,850)),
    ('sea-platform',(35600,-17500,2800),(40000,-14000,1200)),
    ('camp',(8040,8670,1530),(7630,9150,1280))])
index=0
deadline=time.monotonic()+2
task=None
busy=False
capture_pending=False


def tick(delta):
    global index, deadline, task, busy, capture_pending
    if busy or time.monotonic()<deadline:
        return
    if index>=len(views):
        u.unregister_slate_post_tick_callback(handle)
        (OUT/'m3-expansion-capture-requests.txt').write_text('Editor requests issued; verify actual image files separately.\n')
        return
    name,position,target=views[index]
    busy=True
    try:
        if not capture_pending:
            p,t=u.Vector(*position),u.Vector(*target)
            u.get_editor_subsystem(u.LevelEditorSubsystem).editor_set_game_view(True)
            editor.set_level_viewport_camera_info(p,u.MathLibrary.find_look_at_rotation(p,t))
            capture_pending=True
        else:
            # Ordinary viewport capture does not force every loaded vendor
            # texture mip into memory, unlike the high-resolution helper.
            u.SystemLibrary.execute_console_command(editor.get_editor_world(),
                'Shot filename='+str(OUT/('m3-expansion-'+name+'.png')))
            index+=1
            capture_pending=False
        deadline=time.monotonic()+3
    finally:
        busy=False


handle=u.register_slate_post_tick_callback(tick)
print('Queued expansion composition captures')
