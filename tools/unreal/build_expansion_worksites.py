"""Assemble focused vendor-art destinations in the existing First Signal world.

No vendor demo gameplay, water, sky, camera or post-process owners are installed.
Supply destination='powell', 'harbour' or 'sea_platform' to run one saved batch.
"""
import json
from pathlib import Path
import unreal as u

HOST = Path('F:/coastline/LocalHost/CoastalExploration/Content')
OUT = Path('F:/coastline/local-evidence')
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if world.get_name() != 'L_FirstSignal' or level.is_in_play_in_editor():
    raise RuntimeError('Expected First Signal outside PIE')
group = globals().get('destination', 'powell')
if group not in ('powell', 'harbour', 'sea_platform'):
    raise ValueError('Unknown destination')
prefix = 'Coastal.Expansion.' + group + '.'
known = {str(t): a for a in actors.get_all_level_actors() for t in a.tags if str(t).startswith(prefix)}
written = []
cache = {}


def asset(name, root):
    key = root + '/' + name
    if key not in cache:
        matches = list((HOST / root).rglob(name + '.uasset'))
        if len(matches) != 1:
            raise RuntimeError('Expected one installed mesh: ' + key)
        path = '/Game/' + matches[0].relative_to(HOST).with_suffix('').as_posix()
        cache[key] = u.load_asset(path)
        if not isinstance(cache[key], u.StaticMesh):
            raise RuntimeError('Cannot load ' + path)
    return cache[key]


def create(key, sm, position, yaw=0, scale=(1,1,1), collision=False, materials=None):
    tag = prefix + key
    a = known.get(tag)
    if not a:
        a = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(*position))
        a.tags = [u.Name(tag)]
        known[tag] = a
    a.set_actor_label(group + ' — ' + key)
    a.set_folder_path('Coastal Expansion/' + group)
    a.set_actor_location_and_rotation(u.Vector(*position), u.Rotator(yaw=yaw), False, True)
    a.set_actor_scale3d(u.Vector(*scale))
    c = a.static_mesh_component
    c.set_static_mesh(sm)
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    for i, material in enumerate(materials or []):
        if material:
            c.set_material(i, u.load_asset(material))
    written.append(a.get_path_name())
    return a


def fitted(key, name, root, center, size, yaw=0, collision=False):
    sm = asset(name, root)
    extent = sm.get_bounds().box_extent
    scale = tuple(w/(2*v) if abs(v)>0.001 else 1.0 for w,v in zip(size,extent.to_tuple()))
    a = create(key, sm, (0,0,0), yaw, scale, collision)
    origin = a.get_actor_bounds(False)[0]
    a.set_actor_location(u.Vector(*center)-origin, False, True)
    return a


cube = u.load_asset('/Engine/BasicShapes/Cube')
def box(key, center, size):
    material = '/Game/Coastal/M2/Materials/' + ('M_DockTimber' if group == 'powell' else 'M_RustedRail' if group == 'sea_platform' else 'M_Shore')
    return create(key,cube,center,scale=tuple(v/100 for v in size),collision=True,materials=[material])


if group == 'powell':
    # Retain the authored ruined dock cluster and its materials, excluding the
    # demonstration's distant shore layout and all non-art actors.
    rows = json.loads((OUT / 'm3-powell-scene.json').read_text())
    for i, row in enumerate(rows):
        x,y,z = row['position']
        if not row['mesh'].startswith('/Game/Docks/VOL2_Powell/Meshes/') or abs(x)>2600 or not -3200<y<2200:
            continue
        if any(s in row['mesh'] for s in ('SM_Rock_', 'SM_Rocks_', 'SM_Sand_Plane', 'SM_Grass', 'SM_Plant')):
            continue
        a=create(str(i)+' '+row['actor'],u.load_asset(row['mesh']),
                 (x+27000,y+4500,z+600),row['rotation'][1],row['scale'],False,row['materials'])
        a.set_actor_rotation(u.Rotator(pitch=row['rotation'][0],yaw=row['rotation'][1],roll=row['rotation'][2]),False)
    # Authored restoration decking provides a continuous route around broken
    # vendor planks; unsafe decorative collision never controls player access.
    box('Arrival deck',(25500,4000,280),(2600,1000,40))
    box('Restored pier walk',(27000,3600,280),(1300,2200,40))
    box('Seaward viewing deck',(27000,2300,280),(2200,1000,40))
elif group == 'harbour':
    root='Industrial_Harbour/Meshes'
    # Working quay and open warehouse, with a clear west entrance and yard.
    box('Quay foundation',(30000,23000,150),(6200,5400,400))
    fitted('Quay surface','SM_Ground',root,(30000,23000,354),(6200,5400,8))
    for i in range(5):
        fitted('Seawall '+str(i),'SM_SeaWall',root,(27600+i*1200,25680,110),(1200,180,520))
    for side in [-1,1]:
        for i in range(4):
            center=(30000+i*600,23000+side*1250,950)
            fitted(f'Warehouse wall {side} {i}','SM_BrickHouse_Wall',root,center,(600,50,1200))
            box(f'Warehouse wall collision {side} {i}',center,(600,35,1200))
    for i in range(4):
        fitted('Warehouse roof '+str(i),'SM_RoofPlane_3x3',root,(30000+i*600,23000,1570),(610,2600,60))
    for i,(x,y) in enumerate([(28500,24700),(29400,24700),(30600,24700)]):
        fitted('Storage silo '+str(i),'SM_SiloLittle',root,(x,y,1000),(650,650,1300),collision=True)
    for i in range(6):
        fitted('Quay cargo '+str(i),'SM_WoodCrate',root,(28100+(i%3)*250,21200+(i//3)*250,440),(180,180,180),collision=True)
    for i,(x,y) in enumerate([(27800,21700),(27800,25200),(32100,21700),(32100,25200)]):
        fitted('Quay lamp '+str(i),'SM_StreetLight',root,(x,y,950),(100,180,1200))
elif group == 'sea_platform':
    root='HorrorOceanRig/Meshes'
    box('Platform reinforced floor',(40000,-14000,770),(3400,3400,60))
    for x in [-1,1]:
        for y in [-1,1]:
            fitted(f'Platform floor {x} {y}','SM_HOR_Flat_Floor_1',root,
                   (40000+x*825,-14000+y*825,806),(1650,1650,12))
            fitted(f'Platform leg {x} {y}','SM_HOR_Cage_Beam_1',root,
                   (40000+x*1400,-14000+y*1400,50),(220,220,1500))
    fitted('Abandoned operations building','SM_HOR_Full_Building_1',root,(40600,-14600,1350),(1600,1500,1080),collision=True)
    fitted('Platform tower','SM_HOR_Cage_Tower_1',root,(40700,-12900,2200),(750,750,2800))
    fitted('Weathered helipad','SM_HOR_Helipad',root,(39100,-14500,830),(1300,1300,35))
    fitted('Mechanical plant','SM_HOR_Mech_Machine_1',root,(39100,-12800,1080),(650,450,540))
    # Keep a west-side arrival gap; physical rails protect the other deck edges.
    box('North safety rail',(40000,-12310,865),(3400,15,130))
    box('South safety rail',(40000,-15690,865),(3400,15,130))
    box('East safety rail',(41690,-14000,865),(15,3400,130))

if not level.save_current_level():
    raise RuntimeError('Destination save failed')
(OUT / ('m3-'+group+'-placement.json')).write_text(json.dumps({'destination':group,'actors':written},indent=2))
print(json.dumps({'destination':group,'actors':len(written)}))
