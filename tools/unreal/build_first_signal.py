"""Assemble the original persistent M2 coast in the local licensed host (full Editor).

Input inspection is local evidence from the owned cabin; raw vendor assets stay outside Git.
Refuses existing maps. Gameplay acceptance is deliberately separate from construction.
"""
import json
import math
import random
import sys
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).parent))
from m2_geometry import SPEC, PATHS, terrain, closest
EVIDENCE = ROOT.parent / 'local-evidence'
ART = '/Game/Coastal/M2'
VENDOR = '/Game/Fishermans_Cabin/Meshes/'
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
assets = u.AssetToolsHelpers.get_asset_tools()


def load(path):
    asset = u.load_asset(path)
    if not asset:
        raise RuntimeError('Missing authored asset: ' + path)
    return asset


def spawn(cls, name, position, rotation=(0, 0, 0)):
    actor = actors.spawn_actor_from_class(cls, u.Vector(*position), u.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    if not actor:
        raise RuntimeError('Cannot spawn ' + name)
    actor.set_actor_label(name)
    return actor


def mesh(name, path, position, rotation=(0, 0, 0), scale=(1, 1, 1), materials=(), collision=True):
    actor = spawn(u.StaticMeshActor, name, position, rotation)
    component = actor.get_component_by_class(u.StaticMeshComponent)
    component.set_static_mesh(load(path) if isinstance(path, str) else path)
    component.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    actor.set_actor_scale3d(u.Vector(*scale))
    for i, material in enumerate(materials):
        # Demo Blueprint-created dynamic instances are level objects, not reusable assets.
        # Their meshes retain their authored default material in this independent map.
        if isinstance(material, str) and ':PersistentLevel.' in material:
            continue
        if material:
            component.set_material(i, load(material) if isinstance(material, str) else material)
    return actor


def material(name, color, roughness=.85, metal=0):
    path = ART + '/Materials/' + name
    if u.EditorAssetLibrary.does_asset_exist(path):
        return load(path)
    mat = assets.create_asset(name, ART + '/Materials', u.Material, u.MaterialFactoryNew())
    lib = u.MaterialEditingLibrary
    rgb = lib.create_material_expression(mat, u.MaterialExpressionConstant3Vector, -240, 0)
    rgb.set_editor_property('constant', u.LinearColor(*color, 1))
    lib.connect_material_property(rgb, '', u.MaterialProperty.MP_BASE_COLOR)
    for prop, value in [(u.MaterialProperty.MP_ROUGHNESS, roughness), (u.MaterialProperty.MP_METALLIC, metal)]:
        node = lib.create_material_expression(mat, u.MaterialExpressionConstant, -240, 120)
        node.set_editor_property('r', value)
        lib.connect_material_property(node, '', prop)
    mat.set_editor_property('two_sided', True)
    lib.recompile_material(mat)
    u.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def box(name, position, dimensions, mat, rotation=(0, 0, 0), collision=True):
    return mesh(name, '/Engine/BasicShapes/Cube.Cube', position, rotation,
                tuple(v / 100 for v in dimensions), [mat], collision)


def sign(text, position, yaw=0):
    x, y, z = position
    box('Trail sign post', (x, y, z + 85), (12, 12, 170), wood)
    box('Trail sign board', (x, y, z + 150), (9, 155, 48), wood, (0, yaw, 0))
    a = spawn(u.TextRenderActor, 'Wayfinding — ' + text, (x + 7*math.cos(math.radians(yaw)), y + 7*math.sin(math.radians(yaw)), z + 150), (0, yaw, 0))
    c = a.get_component_by_class(u.TextRenderComponent)
    c.set_text(text)
    c.set_world_size(14)
    c.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
    c.set_text_render_color(u.Color(232, 218, 183, 255))


def world_object(key, kind, label, position, asset, rotation=(0, 0, 0), scale=(1, 1, 1), item=None):
    cls = u.load_class(None, '/Game/Coastal/Integration/BP_CoastalHyperWorldObject.BP_CoastalHyperWorldObject_C')
    a = spawn(cls, 'FirstSignal_' + key, position, rotation)
    a.set_editor_property('world_id', u.Name(SPEC['persistent_ids'][key]))
    a.set_editor_property('kind', getattr(u.CoastalObjectKind, kind))
    a.set_editor_property('display_label', label)
    a.set_editor_property('visual_mesh', load(asset) if isinstance(asset, str) else asset)
    a.set_editor_property('visual_transform', u.Transform(scale=u.Vector(*scale)))
    if item:
        requirement = u.CoastalItemRequirement()
        requirement.set_editor_property('item_id', u.Name(item))
        requirement.set_editor_property('quantity', 1)
        a.set_editor_property('pickup_item', requirement)
        a.set_editor_property('persistent_pickup_container', True)
    if kind == 'DISCOVERY':
        a.set_editor_property('journal_entry', u.Name('journal.first_signal.postcard'))
    a.refresh_development_proxy()
    return a


def safety(name, kind, center, extent, destination):
    a = spawn(u.load_class(None, '/Script/CoastalFoundation.CoastalSafetyVolume'), name, center)
    a.set_editor_property('kind', getattr(u.CoastalSafetyKind, kind))
    a.get_editor_property('bounds').set_box_extent(u.Vector(*extent), False)
    a.get_editor_property('return_point').set_world_location(u.Vector(*destination), False, False)


if level.is_in_play_in_editor() or list(u.EditorLoadingAndSavingUtils.get_dirty_map_packages()):
    raise RuntimeError('Preserve unsaved editor work before assembly.')
if u.EditorAssetLibrary.does_asset_exist(SPEC['map_path']):
    raise RuntimeError('M2 map already exists; inspect it instead of overwriting.')

palette = {'Ground':(.15,.21,.105), 'Trail':(.39,.32,.21), 'Shore':(.27,.29,.26),
           'RadioCase':(.06,.14,.12), 'RadioMetal':(.18,.20,.20),
           'RadioDial':(.75,.46,.12), 'RadioSpeaker':(.015,.02,.018)}
materials = {name: material('M_' + name, color) for name, color in palette.items()}
wood = material('M_WeatheredTimber', (.16,.115,.075))
steel = material('M_RustedRail', (.14,.105,.075), .65, .65)
water = material('M_CoveWater', (.018,.105,.14), .22, .35)
imported = {}
for name in ['SM_FirstSignalTerrain', 'SM_FirstSignalRadio']:
    path = ART + '/Geometry/' + name
    if not u.EditorAssetLibrary.does_asset_exist(path):
        task = u.AssetImportTask()
        task.set_editor_property('filename', str(EVIDENCE / 'm2-original-meshes' / (name + '.obj')))
        task.set_editor_property('destination_path', ART + '/Geometry')
        task.set_editor_property('destination_name', name)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', True)
        assets.import_asset_tasks([task])
    sm = load(path)
    for i, slot in enumerate(sm.get_editor_property('static_materials')):
        key = str(slot.get_editor_property('material_slot_name'))
        if key in materials:
            sm.set_material(i, materials[key])
    if name == 'SM_FirstSignalTerrain':
        sm.get_editor_property('body_setup').set_editor_property('collision_trace_flag', u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    else:
        u.get_editor_subsystem(u.StaticMeshEditorSubsystem).add_simple_collisions(sm, u.ScriptCollisionShapeType.BOX)
    u.EditorAssetLibrary.save_loaded_asset(sm)
    imported[name] = sm

if not level.new_level(SPEC['map_path'], False):
    raise RuntimeError('Cannot create M2 persistent map')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('force_no_precomputed_lighting', True)
world.get_world_settings().set_editor_property('default_game_mode', u.load_class(None, '/Game/Coastal/Integration/BP_CoastalGameMode.BP_CoastalGameMode_C'))
mesh('Coastal headland and walking trail', imported['SM_FirstSignalTerrain'], (0,0,0))
box('Cove and open sea', (5000,0,-100), (100000,100000,20), water, collision=False)
sun = spawn(u.DirectionalLight, 'Late afternoon sun', (0,0,6000), (-28,-55,0))
sun.get_component_by_class(u.DirectionalLightComponent).set_mobility(u.ComponentMobility.MOVABLE)
sun.get_component_by_class(u.DirectionalLightComponent).set_intensity(30000)
sun.get_component_by_class(u.DirectionalLightComponent).set_atmosphere_sun_light(True)
spawn(u.SkyAtmosphere, 'Coastal atmosphere', (0,0,0))
sky = spawn(u.SkyLight, 'Open sky fill', (0,0,3000))
sky.get_component_by_class(u.SkyLightComponent).set_mobility(u.ComponentMobility.MOVABLE)
sky.get_component_by_class(u.SkyLightComponent).set_editor_property('real_time_capture', True)
spawn(u.ExponentialHeightFog, 'Sea haze', (0,0,-600))

# Keep the full authored cabin furniture, without copying its demo landscape or foliage instances.
inspection = json.loads((EVIDENCE / 'm2-assets.json').read_text())
offset = (-700,100,121.0154851374)
cabin_count = 0
for row in inspection['prefab_actors']:
    x,y,z = row['position']
    if row['actor'] == 'InstancedFoliageActor' or any(s in row['mesh'] for s in ['/Landscape/', '/Foliage/', '/Rocks/', '/Water/']):
        continue
    if abs(x)>1300 or abs(y)>1400 or z < 100:
        continue
    # East doorway leaf/hardware is replaced by the existing native interaction actor.
    if row['actor'] == 'SM_Sideboard_01':
        storage_row = row
        continue
    if row['actor'] == 'BP_Door' and '/SM_Door_01.' not in row['mesh']:
        continue
    # These decorative meshes have unsuitable solid collision (including
    # phantom convex hits far outside the tray/box visuals). Keep walls/floors.
    no_collision = ('/SM_Plastic_Tray_04.', '/SM_Wooden_Box_Low_04.', '/SM_Cobwebs_01.')
    collision = not any(name in row['mesh'] for name in no_collision)
    decoration = mesh('Cabin_' + row['actor'], row['mesh'], tuple(v+d for v,d in zip(row['position'], offset)), row['rotation'], row['scale'], row['materials'], collision=collision)
    # The thin book on the cabinet's front lip blocks the storage sight line.
    # Retain its physical collision, but let interaction visibility reach the cabinet.
    if row['actor'] == 'SM_Books_175' and '/SM_Books_09.' in row['mesh']:
        decoration.get_component_by_class(u.StaticMeshComponent).set_collision_response_to_channel(
            u.CollisionChannel.ECC_VISIBILITY, u.CollisionResponseType.ECR_IGNORE)
    cabin_count += 1

world_object('door','DOOR','Cabin door',(-516.817386,140.038467,300),VENDOR+'Modules/SM_Door_03', (0,-90,0))
# Original radio and readable maintenance note share a deliberately clear desk by the entrance.
box('Radio desk top',(-760,-95,374),(100,55,8),wood)
for x in [-801,-719]:
    for y in [-115,-75]: box('Radio desk leg',(x,y,340),(6,6,64),wood)
world_object('radio','RADIO','Cabin radio',(-760,-95,378),imported['SM_FirstSignalRadio'],(0,90,0))
world_object('maintenance_note','MAINTENANCE_NOTE','Maintenance note',(-730,-78,379),VENDOR+'Notebook/SM_Notebook_01')
world_object('storage','STORAGE','Cabin storage',tuple(v+d for v,d in zip(storage_row['position'],offset)),storage_row['mesh'],storage_row['rotation'],storage_row['scale'])

# Broad dock landing, a continuous pier and separate guaranteed maintenance stores.
box('Dock landing',(12500,1450,239),(1100,1050,22),wood)
box('Old pier',(12500,-550,239),(360,3000,22),wood)
for y in range(-2000,1100,300):
    for x in [12335,12665]:
        box('Pier pile',(x,y,140),(26,26,480),wood)
        if y < 900: box('Pier handrail',(x,y+145,337),(12,300,12),wood)
world_object('battery_supply_box','PICKUP','Dock electrical supplies',(12120,1570,285),VENDOR+'Wooden_Crates/SM_Wooden_Crates_02',scale=(2,2,2),item='item.radio_battery')
world_object('fuse_supply_box','PICKUP','Pier maintenance supplies',(12830,1210,285),VENDOR+'Wooden_Crates/SM_Wooden_Crates_02',scale=(2,2,2),item='item.marine_fuse')
sign('ELECTRICAL',(12070,1730,250),90)
sign('MAINTENANCE',(12850,1390,250),90)
sign('OLD DOCK',(12200,2200,terrain(12200,2200)[0]),90)

# Optional abandoned railway with an uninterrupted view toward a distant headland.
for x in range(7100,9001,95): box('Railway sleeper',(x,8500,1170),(24,270,18),wood)
for y in [8420,8580]: box('Disused rail',(8070,y,1190),(2100,8,20),steel)
box('Overlook bench',(7530,7950,1185),(160,45,12),wood)
for x in [7470,7590]: box('Bench support',(x,7950,1168),(14,35,36),wood)
world_object('overlook_postcard','DISCOVERY','A postcard from North Reach',(7530,7950,1192),VENDOR+'Book/SM_Book_01',scale=(.65,.65,.12))
sign('NORTH REACH',(8050,8250,1150),-90)
sign('DOCK  >\n<  CABIN',(5680,1740,terrain(5680,1740)[0]),90)
sign('RAIL OVERLOOK',(6100,2580,terrain(6100,2580)[0]),-130)
sign('SHORELINE TRAIL',(400,980,300),-90)

rng = random.Random(47021)
for i in range(230):
    x,y = rng.uniform(-4200,17500),rng.uniform(-1800,11800)
    z,d = terrain(x,y)
    if d < 430 or any(math.hypot(x-a[0],y-a[1])<1550 for a in SPEC['areas'].values()):
        continue
    if y>2000 and i%3:
        scale = rng.uniform(.65,1.1)
        mesh('Coastal fir',VENDOR+'Foliage/Tree/SM_Fir_Tree_01',(x,y,z-30),(0,rng.uniform(0,360),0),(scale,)*3)
    else:
        scale = rng.uniform(1.8,4.5)
        mesh('Weathered coastal rock',VENDOR+'Rocks/SM_Rocks_02',(x,y,z),(0,rng.uniform(0,360),0),(scale,)*3)
for i in range(18):
    mesh('North Reach distant cliffs',VENDOR+'Rocks/SM_Rocks_02',(21000+i*800,-13000+math.sin(i)*1300,-600),(0,i*29,0),(12,12,18),collision=False)

spawn(u.PlayerStart,'FirstSignal_PlayerStart',(0,0,398),(0,180,0))
spawn(u.load_class(None,'/Script/CoastalFoundation.CoastalMissionDirector'),'FirstSignal_MissionDirector',(0,0,500))
for name, pos in [('Cove',(0,0,398)),('Dock',(12500,1800,348)),('Overlook',(7650,7850,1248))]:
    safety(name+' dry checkpoint','DRY_CHECKPOINT',pos,(180,180,110),pos)
safety('Deep sea','DEEP_WATER',(6000,3000,-2000),(45000,45000,1880),(0,0,398))
for name,center,extent in [('West',(-5800,3000,1500),(300,14000,5000)),('East',(18900,3000,1500),(300,14000,5000)),('North',(6000,13700,1500),(14000,300,5000)),('Below',(6000,3000,-1900),(14000,14000,200))]:
    safety(name+' boundary','OUT_OF_BOUNDS',center,extent,(0,0,398))
u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(u.Vector(1200,-1900,1300),u.Rotator(pitch=-20,yaw=140,roll=0))
if not level.save_current_level():
    raise RuntimeError('M2 map save failed')
report = {'map':SPEC['map_path'],'cabin_components':cabin_count,'actors':len(actors.get_all_level_actors()),'gameplay_testing':'deferred by user','assembly':'saved'}
(EVIDENCE/'m2-assembly.json').write_text(json.dumps(report,indent=2))
u.log('COASTAL_M2_ASSEMBLED '+json.dumps(report))
u.SystemLibrary.quit_editor()

