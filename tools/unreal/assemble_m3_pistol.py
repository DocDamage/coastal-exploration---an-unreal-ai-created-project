"""Assemble the owned pistol parts in a separate authoring level, preserving the gameplay map."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if level.is_in_play_in_editor():
    raise RuntimeError('End PIE first')
if u.EditorLoadingAndSavingUtils.get_dirty_map_packages() or u.EditorLoadingAndSavingUtils.get_dirty_content_packages():
    raise RuntimeError('Save or inspect existing dirty packages before authoring')
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (ROOT/'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
output = '/Game/Coastal/Activities/Combat/SM_CoastalPistol'
prefixed = '/Game/Coastal/Activities/Combat/SM_SM_CoastalPistol'
if not u.EditorAssetLibrary.does_asset_exist(output) and u.EditorAssetLibrary.does_asset_exist(prefixed):
    if not u.EditorAssetLibrary.rename_asset(prefixed, output):
        raise RuntimeError('Cannot normalize merged mesh name')
original = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world().get_path_name().split('.')[0]
parts = ['SM_Pistol_Body', 'SM_Slide_Pistol', 'SM_Pistol_Barrel', 'SM_Pistol_Magazine']
if not u.EditorAssetLibrary.does_asset_exist(output):
    if not level.new_level('/Game/Coastal/Activities/Combat/L_PistolAuthoring'):
        raise RuntimeError('Cannot create isolated authoring level')
    actors = u.get_editor_subsystem(u.EditorActorSubsystem)
    sources = []
    for name in parts:
        mesh = u.load_asset('/Game/INVENTORY/Items/Resources/Pistol/' + name)
        if not mesh:
            raise RuntimeError('Missing part ' + name)
        actor = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector())
        actor.static_mesh_component.set_static_mesh(mesh)
        sources.append(actor)
    options = u.MergeStaticMeshActorsOptions()
    options.base_package_name = output
    options.destroy_source_actors = True
    options.spawn_merged_actor = True
    settings = u.MeshMergingSettings()
    settings.merge_materials = False
    settings.merge_physics_data = False
    settings.generate_light_map_uv = False
    options.mesh_merging_settings = settings
    merged = u.get_editor_subsystem(u.StaticMeshEditorSubsystem).merge_static_mesh_actors(sources, options)
    if not merged:
        raise RuntimeError('Mesh assembly failed')
    mesh = merged.static_mesh_component.static_mesh
    if mesh.get_path_name().split('.')[0] != output:
        if not u.EditorAssetLibrary.rename_asset(mesh.get_path_name(), output):
            raise RuntimeError('Cannot normalize merged mesh name')
    if not u.EditorAssetLibrary.save_loaded_asset(mesh):
        raise RuntimeError('Mesh save failed')
    level.save_current_level()
    if not level.load_level(original):
        raise RuntimeError('Return to gameplay map failed')
else:
    mesh = u.load_asset(output)
report = dict(mesh=mesh.get_path_name(), parts=parts, bounds=str(mesh.get_bounds()), original_map=original)
(ROOT/'local-evidence/m3-pistol-assembly.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
