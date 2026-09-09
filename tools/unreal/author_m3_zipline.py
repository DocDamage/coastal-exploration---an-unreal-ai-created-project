"""Author the training zipline and its real wrap endpoint; preserve the map chain."""
import hashlib
import json
import shutil
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[3]
project=ROOT/'LocalHost58/CoastalExploration'
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve()!=(project/'CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
if level.is_in_play_in_editor() or u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
    raise RuntimeError('End PIE and inspect dirty map changes before authoring')
if u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_path_name()!='/Game/Coastal/Maps/L_FirstSignal.L_FirstSignal':
    raise RuntimeError('Expected FirstSignal')
map_file=project/'Content/Coastal/Maps/L_FirstSignal.umap'
report_file=ROOT/'local-evidence/m3-zipline-authoring.json'
prior=json.loads((ROOT/'local-evidence/m3-grapple-gantry.json').read_text())
existing_report=json.loads(report_file.read_text()) if report_file.exists() else None
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
expected=existing_report['after_sha256'] if existing_report else prior['after_sha256']
if sha(map_file)!=expected:
    raise RuntimeError('Map changed outside this authoring pass')
backup=ROOT/'local-evidence/m3-activities-asset-backups/L_FirstSignal-before-zipline.umap'
if not backup.exists(): shutil.copy2(map_file,backup)
if sha(backup)!=prior['after_sha256']: raise RuntimeError('Unexpected zipline backup')
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
existing={a.get_actor_label():a for a in actors.get_all_level_actors()}
def actor(label,cls,location):
    item=existing.get(label)
    if not item:
        item=actors.spawn_actor_from_class(cls,u.Vector(*location))
        item.set_actor_label(label)
    else:
        item.set_actor_location(u.Vector(*location),False,True)
    return item
start=actor('coastal_zipline - Dock start',u.CoastalZipline,(12500,1700,600))
end=actor('coastal_zipline - Dock finish',u.CoastalGrappleAnchor,(14500,1700,490))
start.set_editor_property('end_anchor',end)
folder='/Game/Coastal/Activities/Rope'
path=folder+'/M_ZiplineAnchor'
material=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
if not material:
    material=u.AssetToolsHelpers.get_asset_tools().create_asset('M_ZiplineAnchor',folder,u.Material,u.MaterialFactoryNew())
    color=u.MaterialEditingLibrary.create_material_expression(material,u.MaterialExpressionConstant3Vector,-250,0)
    color.set_editor_property('constant',u.LinearColor(.025,.26,.65,1))
    u.MaterialEditingLibrary.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
    u.MaterialEditingLibrary.recompile_material(material)
    u.EditorAssetLibrary.save_loaded_asset(material)
start.start_anchor.set_material(0,material)
start.cable.set_material(0,u.load_asset(folder+'/M_GrapnelSteel'))
end.anchor_mesh.set_material(0,material)
for label,x,height in [('start',12500,350),('finish',14500,240)]:
    post=actor('coastal_zipline - '+label+' support',u.StaticMeshActor,(x,1700,250+height/2))
    post.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cylinder'))
    post.static_mesh_component.set_collision_profile_name('BlockAll')
    post.set_actor_scale3d(u.Vector(.25,.25,height/100))
if not level.save_current_level(): raise RuntimeError('Zipline map save failed')
report=dict(start=start.get_path_name(),end=end.get_path_name(),map=str(map_file),backup=str(backup),
    before_sha256=sha(backup),after_sha256=sha(map_file))
report_file.write_text(json.dumps(report,indent=2))
print(json.dumps(report))
