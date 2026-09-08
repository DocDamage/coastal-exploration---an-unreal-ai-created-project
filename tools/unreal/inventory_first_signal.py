"""Read saved M2 actor properties and mesh collision metadata without entering gameplay."""
import json
from pathlib import Path
import unreal as u
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
world=editor.get_editor_world()
if world.get_name()!='L_FirstSignal':
    if not level.load_level('/Game/Coastal/Maps/L_FirstSignal'): raise RuntimeError('M2 map missing')
    world=editor.get_editor_world()
mesh_editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
cls=u.load_class(None,'/Script/CoastalFoundation.CoastalWorldObject')
rows=[]
for actor in u.GameplayStatics.get_all_actors_of_class(world,cls):
    component=actor.get_editor_property('proxy_mesh')
    mesh=component.get_editor_property('static_mesh')
    item=actor.get_editor_property('pickup_item')
    position=actor.get_actor_location()
    rotation=actor.get_actor_rotation()
    rows.append({'world_id':str(actor.get_editor_property('world_id')),
                 'kind':str(actor.get_editor_property('kind')),
                 'item_id':str(item.get_editor_property('item_id')),
                 'quantity':item.get_editor_property('quantity'),
                 'journal_entry':str(actor.get_editor_property('journal_entry')),
                 'persistent_container':actor.get_editor_property('persistent_pickup_container'),
                 'mesh':mesh.get_path_name(),
                 'simple_collisions':mesh_editor.get_simple_collision_count(mesh),
                 'convex_collisions':mesh_editor.get_convex_collision_count(mesh),
                 'collision_complexity':str(mesh_editor.get_collision_complexity(mesh)),
                 'collision_profile':str(component.get_collision_profile_name()),
                 'position':[position.x,position.y,position.z],
                 'rotation':[rotation.pitch,rotation.yaw,rotation.roll]})
output=Path(__file__).resolve().parents[3]/'local-evidence/m2-authored-inventory.json'
output.write_text(json.dumps(rows,indent=2))
u.log('COASTAL_M2_AUTHORED_INVENTORY '+str(len(rows)))
u.SystemLibrary.quit_editor()
