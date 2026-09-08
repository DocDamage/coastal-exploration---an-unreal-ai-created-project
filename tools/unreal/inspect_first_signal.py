"""Editor-only composition captures and placed-asset inventory; does not run gameplay."""
import json
import time
from pathlib import Path
import unreal as u

OUT = Path(__file__).resolve().parents[3] / 'local-evidence'
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if level.is_in_play_in_editor() or list(u.EditorLoadingAndSavingUtils.get_dirty_map_packages()):
    raise RuntimeError('Preserve unsaved work before inspection.')
if not level.load_level('/Game/Coastal/Maps/L_FirstSignal'):
    raise RuntimeError('No assembled M2 map to inspect')
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
cls = u.load_class(None, '/Script/CoastalFoundation.CoastalWorldObject')
inventory = []
for actor in u.GameplayStatics.get_all_actors_of_class(editor.get_editor_world(), cls):
    component = actor.get_editor_property('proxy_mesh')
    origin, extent = actor.get_actor_bounds(False)
    inventory.append({'id':str(actor.get_editor_property('world_id')),
                      'mesh':component.get_editor_property('static_mesh').get_path_name(),
                      'center':[origin.x,origin.y,origin.z],
                      'extent':[extent.x,extent.y,extent.z]})
(OUT/'m2-placed-objects.json').write_text(json.dumps(inventory,indent=2))
geometry = []
for name in ['SM_FirstSignalTerrain', 'SM_FirstSignalRadio']:
    sm = u.load_asset('/Game/Coastal/M2/Geometry/' + name)
    bounds = sm.get_bounding_box()
    geometry.append({'name':name,'triangles':sm.get_num_triangles(0),
                     'min':[bounds.min.x,bounds.min.y,bounds.min.z],
                     'max':[bounds.max.x,bounds.max.y,bounds.max.z]})
(OUT/'m2-imported-geometry.json').write_text(json.dumps(geometry,indent=2))

views = [('cabin', (800,-1500,1050),(-19,150,0)),
         ('interior',(-570,70,470),(-14,-145,0)),
         ('trail',(5400,-2100,4000),(-27,70,0)),
         ('dock',(14100,-1900,1800),(-21,125,0)),
         ('overlook',(7050,8300,1460),(-12,-30,0))]
index = 0
deadline = time.monotonic()+12
task = None
capturing = False


def tick(delta):
    global index, deadline, task, capturing
    if capturing or time.monotonic() < deadline:
        return
    if task is not None and not task.is_task_done():
        return
    if index >= len(views):
        u.unregister_slate_post_tick_callback(handle)
        u.EditorPythonScripting.set_keep_python_script_alive(False)
        u.log('COASTAL_M2_EDITOR_CAPTURE_COMPLETE')
        u.SystemLibrary.quit_editor()
        return
    name, position, rotation = views[index]
    editor.set_level_viewport_camera_info(u.Vector(*position),u.Rotator(pitch=rotation[0],yaw=rotation[1],roll=rotation[2]))
    # FinishLoadingBeforeScreenshot pumps Slate while synchronously waiting for assets.
    # Prevent that nested tick from entering this capture again.
    capturing = True
    try:
        task = u.AutomationLibrary.take_high_res_screenshot(1600,900,str(OUT/('m2-'+name+'.png')),delay=5.0)
        index += 1
        deadline = time.monotonic()+10
    finally:
        capturing = False


handle = u.register_slate_post_tick_callback(tick)
u.EditorPythonScripting.set_keep_python_script_alive(True)
