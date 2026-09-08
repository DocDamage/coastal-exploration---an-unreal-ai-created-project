"""Put the seaward pier above water while preserving the authored route heights."""
import json
import sys
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT.parent / 'local-evidence'
sys.path.insert(0, str(Path(__file__).parent))
from m3_environment_layout import basin_height
from m2_geometry import terrain, PATHS

level = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
if level.is_in_play_in_editor() or list(u.EditorLoadingAndSavingUtils.get_dirty_map_packages()):
    raise RuntimeError('Preserve unsaved work before basin refinement')
if not level.load_level('/Game/Coastal/Maps/L_FirstSignal'):
    raise RuntimeError('Opening map missing')
ground = [a for a in actors.get_all_level_actors() if a.get_actor_label() == 'Coastal headland and walking trail']
if len(ground) != 1:
    raise RuntimeError('Expected one authored terrain actor')
component = ground[0].get_component_by_class(u.StaticMeshComponent)
path = '/Game/Coastal/M3/Geometry/SM_M3CoastalTerrain'
if component.get_editor_property('static_mesh').get_path_name().split('.')[0] == path:
    raise RuntimeError('Basin already applied; do not offset scenery twice')
# Sample every authored route segment, not only its endpoints.
for route in PATHS:
    for a, b in zip(route, route[1:]):
        for i in range(101):
            x, y = a[0]+(b[0]-a[0])*i/100, a[1]+(b[1]-a[1])*i/100
            if abs(basin_height(x,y)-terrain(x,y)[0]) > .001:
                raise RuntimeError('Basin changed the authored route')
if not u.EditorAssetLibrary.does_asset_exist(path):
    task = u.AssetImportTask()
    task.set_editor_property('filename', str(OUT/'m3-environment-source/SM_M3CoastalTerrain.obj'))
    task.set_editor_property('destination_path', '/Game/Coastal/M3/Geometry')
    task.set_editor_property('destination_name', 'SM_M3CoastalTerrain')
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
mesh = u.load_asset(path)
material = u.load_asset('/Game/Coastal/M2/Materials/M_CoastalGround')
if not mesh or not material:
    raise RuntimeError('Terrain import/material unavailable')
mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag', u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
for i in range(len(mesh.get_editor_property('static_materials'))):
    mesh.set_material(i, material)
u.EditorAssetLibrary.save_loaded_asset(mesh)
component.set_static_mesh(mesh)
for i in range(component.get_num_materials()):
    component.set_material(i, material)
adjusted = 0
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if not label.startswith(('Weathered coastal rock', 'Coastal fir', 'M3 rocks', 'M3 grass')):
        continue
    p = actor.get_actor_location()
    delta = basin_height(p.x, p.y)-terrain(p.x,p.y)[0]
    if abs(delta) > .01:
        actor.set_actor_location(u.Vector(p.x,p.y,p.z+delta), False, False)
        adjusted += 1
if not level.save_current_level():
    raise RuntimeError('Basin map save failed')
(OUT/'m3-basin.json').write_text(json.dumps({'status':'saved','terrain':path,'route_samples_unchanged':True,
    'scenery_adjusted':adjusted,'gameplay':'not_run'},indent=2))
u.log('COASTAL_M3_BASIN_SAVED')
if __name__ == '__main__':
    u.SystemLibrary.quit_editor()
