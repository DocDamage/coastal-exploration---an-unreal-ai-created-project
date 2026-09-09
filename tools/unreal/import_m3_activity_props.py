"""Import supplied motorcycle, rod and modular ladder into private project-owned paths."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
expected = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != expected.resolve():
    raise RuntimeError('Independent host required')
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('End PIE before import')
library = Path('D:/VaultCache/FabLibrary')
items = [
    dict(name='SK_Enduro', folder='Motorcycle', skeletal=True,
         source=library / 'Post_Apocalyptic_Motorcycle___Rigged_Off_Road_Enduro_Bike-2ba45f8e/fbx/sk_rsg_bike_reskin_fbx_extracted/SK_RSG_Bike.fbx'),
    dict(name='SM_FishingRod', folder='Fishing', skeletal=False,
         source=library / 'Fishing_Rod-762d7400/fbx/fishing_rod_extracted/source/FishingRod.fbx'),
    dict(name='SM_WoodenLadder', folder='Ladder', skeletal=False,
         source=library / 'Modular_Wooden_Ladder-e218c1c0/fbx/mid/modular_wooden_ladder_xb_extracted/Modular_Wooden_Ladder_xb4jcgtdw_Mid.fbx'),
    dict(name='SM_JavaBarb', folder='Fish', skeletal=False,
         source=library / 'Fish_-_Java_barb__Barbonymus_gonionotus_-cd4b7dd5/fbx/fish_java_barb_barbonymu_extracted/source/Ikann.fbx'),
]
for item in items:
    if not item['source'].is_file():
        raise RuntimeError('Missing supplied prop: ' + str(item['source']))
report = []
for item in items:
    destination = '/Game/Coastal/Activities/' + item['folder']
    path = destination + '/' + item['name']
    expected_type = u.SkeletalMesh if item['skeletal'] else u.StaticMesh
    meshes = [d.get_asset() for d in u.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(destination,True)
              if str(d.asset_class_path.asset_name) == ('SkeletalMesh' if item['skeletal'] else 'StaticMesh')]
    if not meshes:
        options = u.FbxImportUI()
        for key, value in dict(automated_import_should_detect_type=False, import_mesh=True,
                              import_as_skeletal=item['skeletal'], import_animations=False,
                              import_materials=False, import_textures=False,
                              mesh_type_to_import=u.FBXImportType.FBXIT_SKELETAL_MESH if item['skeletal'] else u.FBXImportType.FBXIT_STATIC_MESH).items():
            options.set_editor_property(key, value)
        if not item['skeletal']:
            options.static_mesh_import_data.set_editor_property('combine_meshes',True)
            options.static_mesh_import_data.set_editor_property('auto_generate_collision',True)
        task = u.AssetImportTask()
        for key, value in dict(filename=str(item['source']), destination_path=destination,
                              destination_name=item['name'], automated=True, replace_existing=False,
                              save=True, options=options).items():
            task.set_editor_property(key,value)
        u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        meshes = [u.load_asset(p) for p in task.get_editor_property('imported_object_paths')]
        meshes = [m for m in meshes if isinstance(m,expected_type)]
    if not meshes:
        raise RuntimeError('Prop import failed: ' + path)
    for mesh in meshes:
        slots = mesh.get_editor_property('materials' if item['skeletal'] else 'static_materials')
        row = dict(source=str(item['source']), sha256=hashlib.sha256(item['source'].read_bytes()).hexdigest(),
                   asset=mesh.get_path_name(), kind=mesh.get_class().get_name(),
                   material_slots=[str(s.material_slot_name) for s in slots],
                   materials='pending source texture binding', gameplay='not_run')
        if item['skeletal']:
            row['skeleton'] = mesh.get_editor_property('skeleton').get_path_name()
        else:
            box = mesh.get_bounding_box()
            row['bounds_min'] = list(box.min.to_tuple())
            row['bounds_max'] = list(box.max.to_tuple())
        report.append(row)
    (ROOT / 'local-evidence/m3-activity-prop-import.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
