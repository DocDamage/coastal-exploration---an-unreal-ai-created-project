"""Apply authored surface textures and placement corrections to the generated M2 map."""
import math
import sys
from pathlib import Path
import unreal as u
sys.path.insert(0,str(Path(__file__).parent))
from m2_geometry import terrain

ROOT=Path(__file__).resolve().parents[2]
ART='/Game/Coastal/M2'
TEX='/Game/Fishermans_Cabin/Textures/Tiling_Textures/'
lib=u.MaterialEditingLibrary
assets=u.AssetToolsHelpers.get_asset_tools()
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
if level.is_in_play_in_editor() or list(u.EditorLoadingAndSavingUtils.get_dirty_map_packages()):
    raise RuntimeError('Preserve unsaved work before M2 surface refinement')
if not level.load_level('/Game/Coastal/Maps/L_FirstSignal'):
    raise RuntimeError('Assemble M2 first')


def load(path):
    result=u.load_asset(path)
    if not result: raise RuntimeError('Missing owned asset '+path)
    return result


def expression(mat, cls, **props):
    node=lib.create_material_expression(mat,cls)
    for key,value in props.items(): node.set_editor_property(key,value)
    return node


def connect(a, output, b, pin):
    if not lib.connect_material_expressions(a,output,b,pin):
        raise RuntimeError('Cannot connect material pin '+pin)


def planar(mat, sx, sy, ox=0, oy=0):
    position=expression(mat,u.MaterialExpressionWorldPosition)
    xy=expression(mat,u.MaterialExpressionComponentMask,r=True,g=True,b=False,a=False)
    connect(position,'',xy,'')
    scale=expression(mat,u.MaterialExpressionConstant2Vector,r=sx,g=sy)
    mul=expression(mat,u.MaterialExpressionMultiply)
    connect(xy,'',mul,'A'); connect(scale,'',mul,'B')
    offset=expression(mat,u.MaterialExpressionConstant2Vector,r=ox,g=oy)
    add=expression(mat,u.MaterialExpressionAdd)
    connect(mul,'',add,'A'); connect(offset,'',add,'B')
    return add


def sample(mat,path,uv,linear=False):
    node=expression(mat,u.MaterialExpressionTextureSample,texture=load(path))
    if linear: node.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    connect(uv,'',node,'')
    return node


def new_material(name):
    path=ART+'/Materials/'+name
    if u.EditorAssetLibrary.does_asset_exist(path):
        return load(path),False
    return assets.create_asset(name,ART+'/Materials',u.Material,u.MaterialFactoryNew()),True


mask_path=ART+'/Textures/T_FirstSignalGroundMask'
if not u.EditorAssetLibrary.does_asset_exist(mask_path):
    task=u.AssetImportTask()
    task.set_editor_property('filename',str(ROOT.parent/'local-evidence/m2-original-meshes/T_FirstSignalGroundMask.tga'))
    task.set_editor_property('destination_path',ART+'/Textures')
    task.set_editor_property('automated',True)
    task.set_editor_property('save',True)
    assets.import_asset_tasks([task])
mask=load(mask_path)
mask.set_editor_property('srgb',False)
u.EditorAssetLibrary.save_loaded_asset(mask)

ground,created=new_material('M_CoastalGround')
if created:
    uv=planar(ground,1/250,1/250)
    grass=sample(ground,TEX+'Ground_Grass/T_Grass_D',uv)
    dirt=sample(ground,TEX+'Ground_Dirt/T_Ground_Dirt_D',uv)
    rock=sample(ground,TEX+'Rock/T_Rocks_D',uv)
    weights=sample(ground,mask_path,planar(ground,1/24000,1/20640,5500/24000,7000/20640),True)
    path=expression(ground,u.MaterialExpressionLinearInterpolate)
    connect(grass,'',path,'A'); connect(dirt,'',path,'B'); connect(weights,'R',path,'Alpha')
    shore=expression(ground,u.MaterialExpressionLinearInterpolate)
    connect(path,'',shore,'A'); connect(rock,'',shore,'B'); connect(weights,'G',shore,'Alpha')
    lib.connect_material_property(shore,'',u.MaterialProperty.MP_BASE_COLOR)
    rough=expression(ground,u.MaterialExpressionConstant,r=.95)
    lib.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
    ground.set_editor_property('two_sided',True)
    lib.recompile_material(ground)
    u.EditorAssetLibrary.save_loaded_asset(ground)

wood,created=new_material('M_DockTimber')
if created:
    tex=sample(wood,TEX+'Wood_Raw/T_Wood_Raw_DO',planar(wood,1/180,1/180))
    lib.connect_material_property(tex,'',u.MaterialProperty.MP_BASE_COLOR)
    rough=expression(wood,u.MaterialExpressionConstant,r=.9)
    lib.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(wood)
    u.EditorAssetLibrary.save_loaded_asset(wood)

cliff_index=0
for actor in actors.get_all_level_actors():
    label=actor.get_actor_label()
    p=actor.get_actor_location()
    component=actor.get_component_by_class(u.StaticMeshComponent)
    if label=='Cove and open sea':
        actor.set_actor_location(u.Vector(5000,0,-100),False,False)
        actor.set_actor_scale3d(u.Vector(1000,1000,.2))
    elif label=='Deep sea':
        actor.set_actor_location(u.Vector(6000,3000,-2000),False,False)
        actor.get_editor_property('bounds').set_box_extent(u.Vector(45000,45000,1880),False)
        actor.get_editor_property('return_point').set_world_location(u.Vector(0,0,398),False,False)
    elif label.startswith('Coastal fir'):
        actor.set_actor_location(u.Vector(p.x,p.y,terrain(p.x,p.y)[0]-30),False,False)
    elif label.startswith('North Reach distant cliffs'):
        actor.set_actor_location(u.Vector(21000+cliff_index*800,-13000+math.sin(cliff_index)*1300,-600),False,False)
        cliff_index+=1
    elif label.startswith('Pier pile'):
        actor.set_actor_location(u.Vector(p.x,p.y,140),False,False)
        actor.set_actor_scale3d(u.Vector(.26,.26,4.8))
    if component:
        if label=='Coastal headland and walking trail':
            for i in range(component.get_num_materials()): component.set_material(i,ground)
        elif not label.startswith('Cabin_'):
            for i in range(component.get_num_materials()):
                mat=component.get_material(i)
                if mat and mat.get_name()=='M_WeatheredTimber': component.set_material(i,wood)

if not any(a.get_actor_label()=='Cabin reading light' for a in actors.get_all_level_actors()):
    lamp=actors.spawn_actor_from_class(u.PointLight,u.Vector(-780,-100,495))
    lamp.set_actor_label('Cabin reading light')
    light=lamp.get_component_by_class(u.PointLightComponent)
    light.set_mobility(u.ComponentMobility.MOVABLE)
    light.set_intensity(100)
    light.set_light_color(u.LinearColor(1,.78,.55,1))
    light.set_attenuation_radius(600)
if not level.save_current_level(): raise RuntimeError('M2 refinement save failed')
u.log('COASTAL_M2_SURFACES_SAVED')
if __name__=='__main__': u.SystemLibrary.quit_editor()
