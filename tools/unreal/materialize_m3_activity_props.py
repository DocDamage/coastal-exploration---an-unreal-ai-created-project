"""Bind the supplied texture sets to imported activity geometry, without modifying source art."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
expected = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != expected.resolve():
    raise RuntimeError('Independent host required')
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('End PIE before material authoring')
LIB = Path('D:/VaultCache/FabLibrary')
bike = LIB / 'Post_Apocalyptic_Motorcycle___Rigged_Off_Road_Enduro_Bike-2ba45f8e/fbx/sk_rsg_bike_reskin_fbx_extracted/Dirty'
ladder = LIB / 'Modular_Wooden_Ladder-e218c1c0/fbx/mid/modular_wooden_ladder_xb_extracted'
rod = LIB / 'Fishing_Rod-762d7400/fbx/fishing_rod_extracted/textures'
fish = LIB / 'Fish_-_Java_barb__Barbonymus_gonionotus_-cd4b7dd5/fbx/fish_java_barb_barbonymu_extracted/textures'
sets = {}
for number in (1,2):
    prefix = 'SK_RSG_Bike_Body_0' + str(number) + '_Dirty_'
    sets['Motorcycle/M_EnduroBody' + str(number)] = {key: bike/(prefix+suffix+'.png') for key,suffix in
        [('BaseColor','BC'),('Normal','N'),('Roughness','R'),('Metallic','M'),('AmbientOcclusion','AO')]}
prefix = 'Modular_Wooden_Ladder_xb4jcgtdw_Mid_2K_'
sets['Ladder/M_WoodenLadder'] = {key: ladder/(prefix+suffix+'.jpg') for key,suffix in
    [('BaseColor','BaseColor'),('Normal','Normal'),('Roughness','Roughness'),('AmbientOcclusion','AO'),('Specular','Specular')]}
sets['Fishing/M_FishingRod'] = {'BaseColor': rod/'Low_SurvivalKit_Part_2_DefaultMaterial_Bas.png'}
sets['Fish/M_JavaBarb'] = {channel:fish/('m_fish_'+suffix+'.png') for channel,suffix in
    [('BaseColor','BaseColor'),('Normal','Normal'),('Roughness','Roughness'),('Metallic','Metallic'),('OpacityMask','Alpha')]}
for sources in sets.values():
    for path in sources.values():
        if not path.is_file():
            raise RuntimeError('Missing supplied texture: ' + str(path))
props = dict(BaseColor=u.MaterialProperty.MP_BASE_COLOR,Normal=u.MaterialProperty.MP_NORMAL,
             Roughness=u.MaterialProperty.MP_ROUGHNESS,Metallic=u.MaterialProperty.MP_METALLIC,
             AmbientOcclusion=u.MaterialProperty.MP_AMBIENT_OCCLUSION,Specular=u.MaterialProperty.MP_SPECULAR,
             OpacityMask=u.MaterialProperty.MP_OPACITY_MASK)
materials,rows = {},[]
for key,sources in sets.items():
    folder,name = key.split('/')
    dest = '/Game/Coastal/Activities/' + folder
    path = dest+'/'+name
    material = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
    if material is None:
        material = u.AssetToolsHelpers.get_asset_tools().create_asset(name,dest,u.Material,u.MaterialFactoryNew())
        for channel,source in sources.items():
            texname = 'T_'+name[2:]+'_'+channel
            texpath = dest+'/Textures/'+texname
            texture = u.load_asset(texpath) if u.EditorAssetLibrary.does_asset_exist(texpath) else None
            if texture is None:
                task = u.AssetImportTask()
                for field,value in dict(filename=str(source),destination_path=dest+'/Textures',destination_name=texname,
                                        automated=True,replace_existing=False,save=True).items():
                    task.set_editor_property(field,value)
                u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
                texture = u.load_asset(texpath)
            if not isinstance(texture,u.Texture2D):
                raise RuntimeError('Texture import failed: '+str(source))
            texture.set_editor_property('srgb',channel=='BaseColor')
            if channel=='Normal':
                texture.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
            u.EditorAssetLibrary.save_loaded_asset(texture)
            expression = u.MaterialEditingLibrary.create_material_expression(material,u.MaterialExpressionTextureSample)
            expression.set_editor_property('texture',texture)
            expression.set_editor_property('sampler_type', u.MaterialSamplerType.SAMPLERTYPE_NORMAL if channel=='Normal'
                else u.MaterialSamplerType.SAMPLERTYPE_COLOR if channel=='BaseColor' else u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
            if not u.MaterialEditingLibrary.connect_material_property(expression,'RGB' if channel in ('BaseColor','Normal') else 'R',props[channel]):
                raise RuntimeError('Material binding failed: '+channel)
        if folder=='Motorcycle':
            material.set_editor_property('used_with_skeletal_mesh',True)
        if folder=='Fish':
            material.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED)
            material.set_editor_property('two_sided',True)
        u.MaterialEditingLibrary.recompile_material(material)
        if not u.EditorAssetLibrary.save_loaded_asset(material):
            raise RuntimeError('Material save failed: '+path)
    materials[key] = material
registry = u.AssetRegistryHelpers.get_asset_registry()
for folder in ('Motorcycle','Fishing','Ladder','Fish'):
    for data in registry.get_assets_by_path('/Game/Coastal/Activities/'+folder,True):
        if str(data.asset_class_path.asset_name) not in ('StaticMesh','SkeletalMesh'):
            continue
        mesh = data.get_asset()
        skeletal = isinstance(mesh,u.SkeletalMesh)
        slots = mesh.get_editor_property('materials' if skeletal else 'static_materials')
        for index,slot in enumerate(slots):
            name = str(slot.material_slot_name)
            key = 'Motorcycle/M_EnduroBody'+('1' if name=='body_1_pack' else '2') if folder=='Motorcycle' else {
                'Fishing':'Fishing/M_FishingRod','Ladder':'Ladder/M_WoodenLadder','Fish':'Fish/M_JavaBarb'}[folder]
            slot.set_editor_property('material_interface',materials[key])
        mesh.set_editor_property('materials' if skeletal else 'static_materials',slots)
        if not u.EditorAssetLibrary.save_loaded_asset(mesh):
            raise RuntimeError('Mesh material save failed: '+mesh.get_path_name())
        rows.append(dict(mesh=mesh.get_path_name(),materials=[s.material_interface.get_path_name() for s in slots]))
(ROOT/'local-evidence/m3-activity-prop-materials.json').write_text(json.dumps(rows,indent=2))
print(json.dumps(rows))
