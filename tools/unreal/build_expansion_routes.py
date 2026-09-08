"""Append signed routes and a campsite using existing campaign/input ownership."""
import json
import math
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[2]
SPEC = json.loads((ROOT / 'data/m3_expansion.json').read_text())
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
world = editor.get_editor_world()
if world.get_name() != 'L_FirstSignal' or level.is_in_play_in_editor():
    raise RuntimeError('Expected First Signal outside PIE')
by_tag = {str(t): a for a in actors.get_all_level_actors() for t in a.tags if str(t).startswith('Coastal.Expansion.')}


def spawn(cls, key, position, yaw=0):
    tag = 'Coastal.Expansion.' + key
    a = by_tag.get(tag)
    if not a:
        a = actors.spawn_actor_from_class(cls, u.Vector(*position), u.Rotator(yaw=yaw))
        a.set_editor_property('tags', [u.Name(tag)])
        by_tag[tag] = a
    a.set_actor_location_and_rotation(u.Vector(*position), u.Rotator(yaw=yaw), False, True)
    a.set_actor_label(key)
    a.set_folder_path('Coastal Expansion/Routes and Camps')
    return a


def mesh(key, path, position, scale=(1, 1, 1), yaw=0, collision=True):
    sm = u.load_asset(path)
    if not isinstance(sm, u.StaticMesh):
        raise RuntimeError('Missing route/camp mesh ' + path)
    a = spawn(u.StaticMeshActor, key, position, yaw)
    c = a.static_mesh_component
    c.set_static_mesh(sm)
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    if path.startswith('/Engine/BasicShapes/'):
        c.set_material(0,u.load_asset('/Game/Coastal/M2/Materials/M_WeatheredTimber'))
    a.set_actor_scale3d(u.Vector(*scale))
    return a


def sign(key, text, position, yaw=0):
    a = spawn(u.TextRenderActor, key, position, yaw)
    c = a.get_component_by_class(u.TextRenderComponent)
    c.set_text(text)
    c.set_world_size(28)
    c.set_text_render_color(u.Color(235, 220, 180, 255))
    c.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
    mesh(key+' post', '/Engine/BasicShapes/Cube', (position[0],position[1],position[2]-75),(.1,.1,1.5))


tools = u.AssetToolsHelpers.get_asset_tools()
for route in SPEC['routes']:
    name = 'SM_Expansion_' + route['id']
    path = '/Game/Coastal/Expansion/Routes/' + name
    if not u.EditorAssetLibrary.does_asset_exist(path) or route['id'] in globals().get('reimport_routes', []):
        task = u.AssetImportTask()
        task.filename = str(ROOT.parent / 'local-evidence/m3-expansion-source' / (name+'.obj'))
        task.destination_path = '/Game/Coastal/Expansion/Routes'
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True
        tools.import_asset_tasks([task])
    sm = u.load_asset(path)
    sm.get_editor_property('body_setup').set_editor_property('collision_trace_flag', u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    u.EditorAssetLibrary.save_loaded_asset(sm)
    mesh('Route '+route['id'], path, (0,0,0))
    if route.get('kind') == 'boardwalk':
        for segment, (a, b) in enumerate(zip(route['points'], route['points'][1:])):
            dx,dy,dz = [y-x for x,y in zip(a,b)]
            distance = math.hypot(dx,dy)
            yaw = math.degrees(math.atan2(dy,dx))
            for side in [-1, 1]:
                nx,ny = -dy/distance*205*side, dx/distance*205*side
                for i in range(math.ceil(distance/400)+1):
                    t=min(1,i*400/distance)
                    mesh(f'Platform rail post {segment} {side} {i}', '/Engine/BasicShapes/Cube',
                         (a[0]+dx*t+nx,a[1]+dy*t+ny,a[2]+dz*t+45),(.09,.09,.9))
                beam=mesh(f'Platform rail {segment} {side}', '/Engine/BasicShapes/Cube',
                     ((a[0]+b[0])/2+nx,(a[1]+b[1])/2+ny,(a[2]+b[2])/2+90),
                     (math.sqrt(distance*distance+dz*dz)/100,.07,.07),yaw)
                beam.set_actor_rotation(u.Rotator(pitch=math.degrees(math.atan2(dz,distance)),yaw=yaw),False)

sign('North trail directions', 'Baelo Claudia  /  Hallsands', (7950,11900,1600), -90)
sign('Powell trail directions', 'Powell Dock  /  Sea Platform', (15300,1200,435), -90)
sign('Harbour trail directions', 'Industrial Harbour', (14900,16600,1040), -90)

# The overlook already has validated dry ground. A platform gives the camp a
# defined continuous surface without altering the saved dry-checkpoint identity.
camp = (7600, 9100, 1220)
mesh('Overlook campsite deck','/Engine/BasicShapes/Cylinder',(camp[0],camp[1],camp[2]-20),(8,8,.4))
approach=mesh('Campsite approach ramp','/Engine/BasicShapes/Cube',(7600,8600,1180),(6.5,2.2,.16),90)
approach.set_actor_rotation(u.Rotator(pitch=6.15,yaw=90),False)
props = '/Game/GanzSe_Camping_Props/Static_Meshes/'
mesh('Overlook campfire',props+'SM_FCP_Campfire_Type1_Color1',(camp[0]+170,camp[1],camp[2]),collision=False)
mesh('Overlook tent',props+'SM_FCP_Tent_Type1_Color1',(camp[0]-180,camp[1]+180,camp[2]),scale=(.35,.35,.35),yaw=90,collision=False)
mesh('Overlook bedroll',props+'SM_FCP_Bedroll_Type1_Color1',(camp[0]-100,camp[1]+180,camp[2]),collision=False)
marker=spawn(u.TargetPoint,'Overlook campsite pose',(camp[0],camp[1],camp[2]+96),0)
marker.tags=[u.Name('Coastal.Expansion.Overlook campsite pose'),u.Name('Coastal.Campsite')]
sign('Campsite instructions','Campsite\nPause to warm your hands or rest',(camp[0],camp[1]-330,camp[2]+170),-90)
light=spawn(u.PointLight,'Campfire glow',(camp[0]+170,camp[1],camp[2]+55))
c=light.get_component_by_class(u.PointLightComponent)
c.set_mobility(u.ComponentMobility.MOVABLE)
c.set_light_color(u.LinearColor(1,.29,.045))
c.set_intensity(250)
c.set_attenuation_radius(700)
if not level.save_current_level():
    raise RuntimeError('Route/camp save failed')
print(json.dumps({'routes':len(SPEC['routes']),'camp_marker':marker.get_path_name(),'authored_actors':len(by_tag)}))
