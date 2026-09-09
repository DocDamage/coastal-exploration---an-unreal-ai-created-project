"""Author two original provisional item models matching the existing coastal icons."""
import hashlib
import json
import math
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
DEST = '/Game/Coastal/M3/ItemPreview'
OUT = ROOT / 'local-evidence/m3-item-preview-source'
EXPECTED = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != EXPECTED.resolve():
    raise RuntimeError('Expected independent UE5.8 host')
if not u.SystemLibrary.get_engine_version().startswith('5.8.'):
    raise RuntimeError('Expected Unreal 5.8')
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('Stop PIE first')
if u.EditorLoadingAndSavingUtils.get_dirty_map_packages() or u.EditorLoadingAndSavingUtils.get_dirty_content_packages():
    raise RuntimeError('Preserve unsaved editor work first')
if u.EditorAssetLibrary.does_directory_exist(DEST):
    raise RuntimeError('Refusing to replace existing item preview art')
OUT.mkdir(parents=True, exist_ok=True)
PALETTE = {'teal': (.055, .20, .23), 'cream': (.72, .70, .52),
           'metal': (.52, .67, .63), 'gold': (.85, .45, .13)}
materials = {}
for name, color in PALETTE.items():
    for shade, gain in enumerate((.65, .85, 1.0)):
        key = name + str(shade)
        mat = u.AssetToolsHelpers.get_asset_tools().create_asset('M_' + key, DEST, u.Material, u.MaterialFactoryNew())
        mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
        expression = u.MaterialEditingLibrary.create_material_expression(mat, u.MaterialExpressionConstant3Vector)
        expression.set_editor_property('constant', u.LinearColor(*(c * gain for c in color), 1))
        u.MaterialEditingLibrary.connect_material_property(expression, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
        u.MaterialEditingLibrary.recompile_material(mat)
        u.EditorAssetLibrary.save_loaded_asset(mat)
        materials[key] = mat


class Model:
    def __init__(self):
        self.vertices, self.faces = [], []

    def face(self, points, material):
        start = len(self.vertices) + 1
        self.vertices.extend(points)
        self.faces.append((material, list(range(start, start + len(points)))))

    def box(self, low, high, material):
        x, y, z = low
        X, Y, Z = high
        vertices = [(x,y,z),(X,y,z),(X,Y,z),(x,Y,z),(x,y,Z),(X,y,Z),(X,Y,Z),(x,Y,Z)]
        for i, face in enumerate(((0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7))):
            self.face([vertices[n] for n in face], material + str(i % 3))

    def cylinder(self, start, end, radius, material):
        # Horizontal fuse cylinder along X; 24-sided opaque model, no transparency claim.
        rings = [[(x, radius * math.cos(i * math.tau / 24), radius * math.sin(i * math.tau / 24))
                  for i in range(24)] for x in (start, end)]
        self.face(list(reversed(rings[0])), material + '0')
        self.face(rings[1], material + '2')
        for i in range(24):
            j = (i + 1) % 24
            self.face([rings[0][i], rings[0][j], rings[1][j], rings[1][i]], material + str((i // 4) % 3))

    def write(self, name):
        path = OUT / (name + '.obj')
        # OBJ is Y-up. Unreal's standard OBJ importer converts it back to Z-up.
        lines = ['mtllib previews.mtl', 'o ' + name]
        lines += [f'v {x} {z} {-y}' for x,y,z in self.vertices]
        for material, face in self.faces:
            lines += ['usemtl ' + material, 'f ' + ' '.join(map(str, face))]
        path.write_text('\n'.join(lines) + '\n')
        return path


(OUT / 'previews.mtl').write_text('\n'.join('newmtl ' + name + '\nKd 0.5 0.5 0.5' for name in materials))
battery = Model()
battery.box((-3,-1.8,-4.5), (3,1.8,3.5), 'teal')
battery.box((-3.1,-1.9,3.5), (3.1,1.9,4.2), 'cream')
battery.box((-2.2,-.65,4.2), (-1,.65,5), 'metal')
battery.box((1,-.65,4.2), (2.2,.65,4.8), 'metal')
battery.box((-2,-1.86,-2.4), (2,-1.81,1.4), 'cream')
battery.box((-.25,-1.92,-1), (.25,-1.87,1), 'gold')
battery.box((-1,-1.92,-.25), (1,-1.87,.25), 'gold')
fuse = Model()
fuse.cylinder(-3.2,3.2,.85,'teal')
fuse.cylinder(-4,-2.6,1.02,'metal')
fuse.cylinder(2.6,4,1.02,'metal')
fuse.box((-2.5,-.9,-.12), (2.5,-.86,.12), 'gold')
rows = []
for name, model in [('SM_RadioBatteryPreview', battery), ('SM_MarineFusePreview', fuse)]:
    path = model.write(name)
    options = u.FbxImportUI()
    options.set_editor_property('import_materials', False)
    options.set_editor_property('import_textures', False)
    options.static_mesh_import_data.set_editor_property('combine_meshes', True)
    options.static_mesh_import_data.set_editor_property('auto_generate_collision', False)
    task = u.AssetImportTask()
    for key, value in dict(filename=str(path), destination_path=DEST, destination_name=name,
                           automated=True, replace_existing=False, save=False, options=options).items():
        task.set_editor_property(key, value)
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = u.load_asset(DEST + '/' + name)
    if not isinstance(mesh, u.StaticMesh):
        raise RuntimeError('Original item mesh import failed: ' + name)
    slots = []
    for index, slot in enumerate(mesh.get_editor_property('static_materials')):
        key = str(slot.get_editor_property('imported_material_slot_name'))
        if key not in materials:
            raise RuntimeError('Unexpected OBJ material slot: ' + key)
        mesh.set_material(index, materials[key])
        slots.append(key)
    if not u.EditorAssetLibrary.save_loaded_asset(mesh):
        raise RuntimeError('Failed to save original preview mesh')
    rows.append(dict(asset=mesh.get_path_name(), slots=slots, source_sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
(ROOT / 'local-evidence/m3-item-preview-art.json').write_text(json.dumps(dict(models=rows,
    scope='Original provisional battery/fuse art; no vendor mesh or persistent pickup changes'), indent=2))
print(json.dumps(rows))
# Native optional art lookup must also survive cooking. Preserve all other rules.
config = EXPECTED.parent / 'Config/DefaultGame.ini'
rule = '+DirectoriesToAlwaysCook=(Path="' + DEST + '")'
text = config.read_text()
if rule not in text:
    if '[/Script/UnrealEd.ProjectPackagingSettings]' not in text:
        raise RuntimeError('Expected existing packaging settings section')
    text = text.replace('[/Script/UnrealEd.ProjectPackagingSettings]',
                        '[/Script/UnrealEd.ProjectPackagingSettings]\n' + rule, 1)
    config.write_text(text)
