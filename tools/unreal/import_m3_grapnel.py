"""Import the original three-prong grapnel and assign a steel material."""
import json
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[3]
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (ROOT/'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('End PIE before import')
folder='/Game/Coastal/Activities/Rope'
path=folder+'/SM_Grapnel'
tools=u.AssetToolsHelpers.get_asset_tools()
if u.EditorAssetLibrary.does_asset_exist(path):
    mesh=u.load_asset(path)
else:
    options=u.FbxImportUI()
    for key,value in dict(automated_import_should_detect_type=False,import_mesh=True,import_as_skeletal=False,
        import_animations=False,import_materials=False,import_textures=False,
        mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH).items():
        options.set_editor_property(key,value)
    options.static_mesh_import_data.set_editor_property('combine_meshes',True)
    options.static_mesh_import_data.set_editor_property('auto_generate_collision',False)
    task=u.AssetImportTask()
    for key,value in dict(filename=str(ROOT/'local-evidence/m3-grapnel-source/grapnel.obj'),destination_path=folder,
        destination_name='SM_Grapnel',automated=True,replace_existing=False,save=True,options=options).items():
        task.set_editor_property(key,value)
    tools.import_asset_tasks([task])
    meshes=[u.load_asset(p) for p in task.get_editor_property('imported_object_paths')]
    meshes=[m for m in meshes if isinstance(m,u.StaticMesh)]
    if len(meshes)!=1:
        raise RuntimeError('Expected one grapnel mesh')
    mesh=meshes[0]
    if mesh.get_path_name().split('.')[0]!=path:
        if not u.EditorAssetLibrary.rename_asset(mesh.get_path_name(),path):
            raise RuntimeError('Cannot normalize grapnel name')
material_path=folder+'/M_GrapnelSteel'
if u.EditorAssetLibrary.does_asset_exist(material_path):
    material=u.load_asset(material_path)
else:
    material=tools.create_asset('M_GrapnelSteel',folder,u.Material,u.MaterialFactoryNew())
    color=u.MaterialEditingLibrary.create_material_expression(material,u.MaterialExpressionConstant3Vector,-300,0)
    color.set_editor_property('constant',u.LinearColor(.18,.22,.25,1))
    u.MaterialEditingLibrary.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
    for prop,value,y in ((u.MaterialProperty.MP_METALLIC,.95,100),(u.MaterialProperty.MP_ROUGHNESS,.3,200)):
        expression=u.MaterialEditingLibrary.create_material_expression(material,u.MaterialExpressionConstant,-300,y)
        expression.set_editor_property('r',value)
        u.MaterialEditingLibrary.connect_material_property(expression,'',prop)
    u.MaterialEditingLibrary.recompile_material(material)
    u.EditorAssetLibrary.save_loaded_asset(material)
mesh.set_material(0,material)
u.EditorAssetLibrary.save_loaded_asset(mesh)
box=mesh.get_bounding_box()
report=dict(mesh=mesh.get_path_name(),material=material.get_path_name(),minimum=list(box.min.to_tuple()),maximum=list(box.max.to_tuple()))
(ROOT/'local-evidence/m3-grapnel-import.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
