"""Place a visible Dynamic Rope gantry on the clear dock route, backing up the map first."""
import hashlib
import json
import shutil
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[3]
project=ROOT/'LocalHost58/CoastalExploration'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve()!=(project/'CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
if level.is_in_play_in_editor():
    raise RuntimeError('End PIE before authoring')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if world.get_path_name()!='/Game/Coastal/Maps/L_FirstSignal.L_FirstSignal':
    raise RuntimeError('Expected FirstSignal')
if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
    raise RuntimeError('Inspect unsaved map changes first')
map_file=project/'Content/Coastal/Maps/L_FirstSignal.umap'
backup=ROOT/'local-evidence/m3-activities-asset-backups/L_FirstSignal-before-grapple.umap'
if not backup.exists():
    shutil.copy2(map_file,backup)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
existing={a.get_actor_label():a for a in actors.get_all_level_actors()}
label='coastal_rope - Grapple gantry'
anchor=existing.get(label)
if not anchor:
    anchor=actors.spawn_actor_from_class(u.CoastalGrappleAnchor,u.Vector(13200,1800,950),u.Rotator(roll=90))
    anchor.set_actor_label(label)
    anchor.set_actor_scale3d(u.Vector(.8,.8,4.2))
folder='/Game/Coastal/Activities/Rope'
material_path=folder+'/M_GrappleAnchor'
if u.EditorAssetLibrary.does_asset_exist(material_path):
    material=u.load_asset(material_path)
else:
    material=u.AssetToolsHelpers.get_asset_tools().create_asset('M_GrappleAnchor',folder,u.Material,u.MaterialFactoryNew())
    color=u.MaterialEditingLibrary.create_material_expression(material,u.MaterialExpressionConstant3Vector,-300,0)
    color.set_editor_property('constant',u.LinearColor(.8,.22,.025,1))
    u.MaterialEditingLibrary.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
    u.MaterialEditingLibrary.recompile_material(material)
    u.EditorAssetLibrary.save_loaded_asset(material)
anchor.anchor_mesh.set_material(0,material)
for side in (-1,1):
    post_label=label+' support '+str(side)
    post=existing.get(post_label)
    if not post:
        post=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(13200,1800+side*190,600))
        post.set_actor_label(post_label)
        post.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cylinder'))
        post.static_mesh_component.set_collision_profile_name('BlockAll')
        post.set_actor_scale3d(u.Vector(.4,.4,7.0))
        post.tags=['Coastal.RopeGantrySupport']
if not level.save_current_level():
    raise RuntimeError('Gantry map save failed')
report=dict(anchor=anchor.get_path_name(),location=list(anchor.get_actor_location().to_tuple()),map=str(map_file),backup=str(backup),
    before_sha256=hashlib.sha256(backup.read_bytes()).hexdigest(),after_sha256=hashlib.sha256(map_file.read_bytes()).hexdigest())
(ROOT/'local-evidence/m3-grapple-gantry.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
