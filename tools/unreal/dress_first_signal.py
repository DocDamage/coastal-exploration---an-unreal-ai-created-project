"""Apply M3 owned environment dressing to the existing coast; no gameplay execution."""
import json
import shutil
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT.parent / 'local-evidence'
SOURCE = OUT / 'm3-environment-source'
ART = '/Game/Coastal/M3'
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
assets = u.AssetToolsHelpers.get_asset_tools()


def load(path):
    asset = u.load_asset(path)
    if not asset:
        raise RuntimeError('Missing inspected asset ' + path)
    return asset


def decorate(label, asset, position, yaw=0, scale=1):
    actor = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(*position), u.Rotator(pitch=0, yaw=yaw, roll=0))
    if not actor:
        raise RuntimeError('Cannot place ' + label)
    actor.set_actor_label('M3 ' + label)
    actor.set_folder_path('Coastal/M3 dressing')
    actor.set_actor_scale3d(u.Vector(scale, scale, scale))
    comp = actor.get_component_by_class(u.StaticMeshComponent)
    comp.set_static_mesh(asset)
    comp.set_collision_profile_name('NoCollision')
    return actor, comp


if level.is_in_play_in_editor() or list(u.EditorLoadingAndSavingUtils.get_dirty_map_packages()) or list(u.EditorLoadingAndSavingUtils.get_dirty_content_packages()):
    raise RuntimeError('Preserve unsaved editor work before dressing')
if not level.load_level('/Game/Coastal/Maps/L_FirstSignal'):
    raise RuntimeError('M2 coast is missing')
existing = list(actors.get_all_level_actors())
if any(a.get_actor_label().startswith('M3 ') for a in existing):
    raise RuntimeError('M3 dressing already exists; inspect it rather than duplicating')
water_actors = [a for a in existing if a.get_actor_label() == 'Cove and open sea']
if len(water_actors) != 1:
    raise RuntimeError('Expected exactly one authored water actor')
placements = json.loads((SOURCE / 'placements.json').read_text())
inspection = json.loads((OUT / 'm3-environment-assets.json').read_text())
config = json.loads((ROOT / 'data/m3_environment.json').read_text())
rock = load('/Game/Fishermans_Cabin/Meshes/Rocks/SM_Rocks_01')
grass = load('/Game/Fishermans_Cabin/Meshes/Foliage/Grass/SM_Grass_03')
wood = load('/Game/Coastal/M2/Materials/M_DockTimber')
owned_water = load(inspection['water']['path'])
# Preserve the pre-dressing map once, including its previous water assignment.
backup = OUT / 'm3-environment-backup'
backup.mkdir(exist_ok=True)
map_file = ROOT.parent / 'LocalHost/CoastalExploration/Content/Coastal/Maps/L_FirstSignal.umap'
if not (backup / map_file.name).exists():
    shutil.copy2(map_file, backup / map_file.name)

water_path = ART + '/Materials/MI_CoveWater'
if not u.EditorAssetLibrary.does_asset_exist(water_path):
    if not u.EditorAssetLibrary.duplicate_asset(owned_water.get_path_name(), water_path):
        raise RuntimeError('Cannot create project-owned water instance')
water = load(water_path)
for name, value in config['water_parameters'].items():
    # UE 5.7's setter returns false even after writing (inspected engine source).
    # Verify the actual stored parameter instead of interpreting that return.
    u.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(water, name, value)
    if abs(u.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(water, name) - value) > .0001:
        raise RuntimeError('Cannot set inspected water parameter ' + name)
u.MaterialEditingLibrary.update_material_instance(water)
u.EditorAssetLibrary.save_loaded_asset(water)
board_path = ART + '/Geometry/SM_M3DockBoards'
if not u.EditorAssetLibrary.does_asset_exist(board_path):
    task = u.AssetImportTask()
    task.set_editor_property('filename', str(SOURCE / 'SM_M3DockBoards.obj'))
    task.set_editor_property('destination_path', ART + '/Geometry')
    task.set_editor_property('destination_name', 'SM_M3DockBoards')
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    assets.import_asset_tasks([task])
boards = load(board_path)
for i in range(len(boards.get_editor_property('static_materials'))):
    boards.set_material(i, wood)
u.EditorAssetLibrary.save_loaded_asset(boards)

world_class = u.load_class(None, '/Script/CoastalFoundation.CoastalWorldObject')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()


def persistent_state():
    return sorted((str(a.get_editor_property('world_id')), str(a.get_actor_transform()))
                  for a in u.GameplayStatics.get_all_actors_of_class(world, world_class))


before = persistent_state()
water_comp = water_actors[0].get_component_by_class(u.StaticMeshComponent)
previous_water = water_comp.get_material(0).get_path_name()
water_comp.set_material(0, water)
decorate('Dock board surface', boards, (0, 0, 0))
for group, asset in [('rocks', rock), ('grass', grass)]:
    for index, row in enumerate(placements[group]):
        # Inspected meshes have their base at min Z; bury only a small base edge.
        zmin = asset.get_bounding_box().min.z
        x, y, z = row['position']
        actor, comp = decorate(f'{group} {index:02}', asset,
            (x, y, z - zmin * row['scale'] - 4), row['yaw'], row['scale'])
        if group == 'grass':
            comp.set_cast_shadow(False)
            comp.set_cull_distance(8500)
if before != persistent_state():
    raise RuntimeError('Persistent object identities/transforms changed; refusing map save')
if not level.save_current_level():
    raise RuntimeError('M3 map save failed')
report = {'status': 'saved', 'decorative_actors': 81, 'previous_water': previous_water,
          'water': water.get_path_name(), 'persistent_objects_unchanged': True,
          'gameplay': 'not_run', 'native_build': 'not_run', 'backup': str(backup / map_file.name)}
(OUT / 'm3-environment-assembly.json').write_text(json.dumps(report, indent=2))
u.log('COASTAL_M3_ENVIRONMENT_SAVED')
if __name__ == '__main__':
    u.SystemLibrary.quit_editor()
