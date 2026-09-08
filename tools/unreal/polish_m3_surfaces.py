"""Project-owned ground shading and dock surface cleanup; no native build."""
import json
import shutil
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT.parent / 'local-evidence'
ART = '/Game/Coastal/M3/Materials'
TEX = '/Game/Fishermans_Cabin/Textures/Tiling_Textures/'
lib = u.MaterialEditingLibrary
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)


def load(path):
    value = u.load_asset(path)
    if not value:
        raise RuntimeError('Missing existing asset ' + path)
    return value


def node(mat, cls, **props):
    result = lib.create_material_expression(mat, cls)
    for name, value in props.items():
        result.set_editor_property(name, value)
    return result


def wire(a, output, b, pin):
    if not lib.connect_material_expressions(a, output, b, pin):
        raise RuntimeError('Cannot connect ' + pin)


def uv(mat, sx, sy, ox=0, oy=0):
    p = node(mat, u.MaterialExpressionWorldPosition)
    xy = node(mat, u.MaterialExpressionComponentMask, r=True, g=True, b=False, a=False)
    wire(p, '', xy, '')
    scale = node(mat, u.MaterialExpressionConstant2Vector, r=sx, g=sy)
    mul = node(mat, u.MaterialExpressionMultiply)
    wire(xy, '', mul, 'A'); wire(scale, '', mul, 'B')
    offset = node(mat, u.MaterialExpressionConstant2Vector, r=ox, g=oy)
    result = node(mat, u.MaterialExpressionAdd)
    wire(mul, '', result, 'A'); wire(offset, '', result, 'B')
    return result


def sample(mat, texture, coords, sampler=None):
    if texture == TEX + 'Rock/T_Rocks_N' and sampler == u.MaterialSamplerType.SAMPLERTYPE_NORMAL:
        normal_path = '/Game/Coastal/M3/Textures/T_RockNormal'
        if not u.EditorAssetLibrary.does_asset_exist(normal_path):
            if not u.EditorAssetLibrary.duplicate_asset(texture, normal_path):
                raise RuntimeError('Cannot create owned normal-map copy')
        normal_texture = load(normal_path)
        normal_texture.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_NORMALMAP)
        normal_texture.set_editor_property('srgb', False)
        u.EditorAssetLibrary.save_loaded_asset(normal_texture)
        texture = normal_path
    result = node(mat, u.MaterialExpressionTextureSample, texture=load(texture))
    if sampler is not None:
        result.set_editor_property('sampler_type', sampler)
    wire(coords, '', result, '')  # TextureSample's first exposed input is its UVs.
    return result


def blend(mat, a, b, mask, channel):
    result = node(mat, u.MaterialExpressionLinearInterpolate)
    wire(a, '', result, 'A'); wire(b, '', result, 'B'); wire(mask, channel, result, 'Alpha')
    return result


if level.is_in_play_in_editor() or list(u.EditorLoadingAndSavingUtils.get_dirty_map_packages()) or list(u.EditorLoadingAndSavingUtils.get_dirty_content_packages()):
    raise RuntimeError('Preserve unsaved work before surface polish')
if not level.load_level('/Game/Coastal/Maps/L_FirstSignal'):
    raise RuntimeError('Opening map missing')
all_actors = list(actors.get_all_level_actors())
ground = [a for a in all_actors if a.get_actor_label() == 'Coastal headland and walking trail']
boards = [a for a in all_actors if a.get_actor_label() == 'M3 Dock board surface']
if len(ground) != 1 or len(boards) != 1:
    raise RuntimeError('Complete M3 dressing before surface polish')
paths = ['Ground_Grass/T_Grass_D', 'Ground_Dirt/T_Ground_Dirt_D', 'Rock/T_Rocks_D',
         'Ground_Grass/T_Grass_N', 'Ground_Dirt/T_Ground_Dirt_N', 'Rock/T_Rocks_N']
for path in paths:
    load(TEX + path)
mask_path = '/Game/Coastal/M2/Textures/T_FirstSignalGroundMask'
load(mask_path)
backup = OUT / 'm3-surface-backup'
backup.mkdir(exist_ok=True)
map_file = ROOT.parent / 'LocalHost/CoastalExploration/Content/Coastal/Maps/L_FirstSignal.umap'
if not (backup / map_file.name).exists():
    shutil.copy2(map_file, backup / map_file.name)
material_path = ART + '/M_HeadlandGround'
if u.EditorAssetLibrary.does_asset_exist(material_path):
    raise RuntimeError('Surface material exists; inspect it rather than rebuilding over edits')
mat = u.AssetToolsHelpers.get_asset_tools().create_asset('M_HeadlandGround', ART, u.Material, u.MaterialFactoryNew())
coords = uv(mat, 1/250, 1/250)
mask = sample(mat, mask_path, uv(mat, 1/24000, 1/20640, 5500/24000, 7000/20640), u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
colors = []
for path, tint in zip(paths[:3], [(0.30, .48, .23), (.78, .66, .51), (.65, .69, .72)]):
    texture = sample(mat, TEX + path, coords)
    color = node(mat, u.MaterialExpressionConstant3Vector, constant=u.LinearColor(*tint, 1))
    tinted = node(mat, u.MaterialExpressionMultiply)
    wire(texture, '', tinted, 'A'); wire(color, '', tinted, 'B')
    colors.append(tinted)
base = blend(mat, blend(mat, colors[0], colors[1], mask, 'R'), colors[2], mask, 'G')
normals = [sample(mat, TEX + path, coords, u.MaterialSamplerType.SAMPLERTYPE_NORMAL) for path in paths[3:]]
normal = blend(mat, blend(mat, normals[0], normals[1], mask, 'R'), normals[2], mask, 'G')
rough = node(mat, u.MaterialExpressionConstant, r=.94)
for expr, prop in [(base, u.MaterialProperty.MP_BASE_COLOR), (normal, u.MaterialProperty.MP_NORMAL), (rough, u.MaterialProperty.MP_ROUGHNESS)]:
    if not lib.connect_material_property(expr, '', prop):
        raise RuntimeError('Cannot connect ground material output')
mat.set_editor_property('two_sided', True)
lib.recompile_material(mat)
u.EditorAssetLibrary.save_loaded_asset(mat)
component = ground[0].get_component_by_class(u.StaticMeshComponent)
previous = [component.get_material(i).get_path_name() for i in range(component.get_num_materials())]
for i in range(component.get_num_materials()):
    component.set_material(i, mat)
ballast_path = ART + '/M_RailBallast'
if u.EditorAssetLibrary.does_asset_exist(ballast_path):
    raise RuntimeError('Rail ballast material already exists; preserve it')
ballast = u.AssetToolsHelpers.get_asset_tools().create_asset('M_RailBallast', ART, u.Material, u.MaterialFactoryNew())
stone_uv = uv(ballast, 1/65, 1/65)
stone = sample(ballast, TEX + 'Rock/T_Rocks_D', stone_uv)
tint = node(ballast, u.MaterialExpressionConstant3Vector, constant=u.LinearColor(.28, .29, .27, 1))
color = node(ballast, u.MaterialExpressionMultiply)
wire(stone, '', color, 'A'); wire(tint, '', color, 'B')
stone_normal = sample(ballast, TEX + 'Rock/T_Rocks_N', stone_uv, u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
stone_rough = node(ballast, u.MaterialExpressionConstant, r=.96)
for expr, prop in [(color, u.MaterialProperty.MP_BASE_COLOR), (stone_normal, u.MaterialProperty.MP_NORMAL), (stone_rough, u.MaterialProperty.MP_ROUGHNESS)]:
    if not lib.connect_material_property(expr, '', prop):
        raise RuntimeError('Cannot connect ballast output')
lib.recompile_material(ballast)
u.EditorAssetLibrary.save_loaded_asset(ballast)
rail = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(8070, 8500, 1153))
rail.set_actor_label('M3 Rail ballast')
rail.set_folder_path('Coastal/M3 dressing')
rail.set_actor_scale3d(u.Vector(23, 3.1, .16))
rail_comp = rail.get_component_by_class(u.StaticMeshComponent)
rail_comp.set_static_mesh(load('/Engine/BasicShapes/Cube'))
rail_comp.set_material(0, ballast)
rail_comp.set_collision_profile_name('NoCollision')
# M2's solid deck overlapped terrain at the landing. Keep its continuous
# collider but render the M3 plank mesh instead of two coincident surfaces.
hidden = []
for actor in all_actors:
    if actor.get_actor_label() in ('Dock landing', 'Old pier'):
        actor.get_component_by_class(u.StaticMeshComponent).set_visibility(False)
        hidden.append(actor.get_actor_label())
if len(hidden) != 2:
    raise RuntimeError('Expected two original deck collision surfaces')
if not level.save_current_level():
    raise RuntimeError('Surface map save failed')
(OUT/'m3-surfaces.json').write_text(json.dumps({'status':'saved','ground_material':material_path,
    'previous_materials':previous,'hidden_visuals_collision_retained':hidden,'rail_ballast':ballast_path,'gameplay':'not_run'},indent=2))
u.log('COASTAL_M3_SURFACES_SAVED')
if __name__ == '__main__':
    u.SystemLibrary.quit_editor()
