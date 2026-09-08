"""Normalize the two owned survey scans for Unreal. Run with Blender in background.

Vendor originals remain untouched; derived FBX/textures belong in LocalVendor.
Both supplied scenes store elevation along negative world Y.
"""
import bpy
import json
import math
import sys
from mathutils import Matrix, Vector
from pathlib import Path

kind, source, destination = sys.argv[sys.argv.index('--') + 1:]
source, destination = Path(source), Path(destination)
destination.mkdir(parents=True, exist_ok=True)
if kind == 'hallsands':
    bpy.ops.wm.open_mainfile(filepath=str(source))
    textures = source.parent.parent / 'textures'
    mat = bpy.data.materials['matScan']
    mat.use_nodes = True
    mat.node_tree.nodes.clear()
    output = mat.node_tree.nodes.new('ShaderNodeOutputMaterial')
    surface = mat.node_tree.nodes.new('ShaderNodeBsdfPrincipled')
    surface.inputs['Roughness'].default_value = 0.95
    mat.node_tree.links.new(surface.outputs['BSDF'], output.inputs['Surface'])
    texture = mat.node_tree.nodes.new('ShaderNodeTexImage')
    texture.image = bpy.data.images.load(str(textures / '20190129_MAT_Hallsand_low_uv_repack_defaul.jpg'))
    mat.node_tree.links.new(texture.outputs['Color'], surface.inputs['Base Color'])
    normal = mat.node_tree.nodes.new('ShaderNodeTexImage')
    normal.image = bpy.data.images.load(str(textures / '20190129_MAT_Hallsands_8k_repack_normals.jpg'))
    normal.image.colorspace_settings.name = 'Non-Color'
    normalmap = mat.node_tree.nodes.new('ShaderNodeNormalMap')
    mat.node_tree.links.new(normal.outputs['Color'], normalmap.inputs['Color'])
    mat.node_tree.links.new(normalmap.outputs['Normal'], surface.inputs['Normal'])
    base = bpy.data.materials['matBase']
    base.use_nodes = True
    bsdf = base.node_tree.nodes.get('Principled BSDF')
    if bsdf:
        bsdf.inputs['Base Color'].default_value = (0.19, 0.17, 0.13, 1)
        bsdf.inputs['Roughness'].default_value = 1
else:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    for im in bpy.data.images:
        if im.packed_file:
            im.filepath_raw = str(destination / (im.name + '.jpg'))
            im.file_format = 'JPEG'
            im.save()
            im.unpack(method='REMOVE')
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
rotate = Matrix.Rotation(-math.pi / 2, 4, 'X')
for o in meshes:
    world = o.matrix_world.copy()
    o.parent = None
    o.matrix_world = rotate @ world
corners = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
low = Vector([min(c[i] for c in corners) for i in range(3)])
high = Vector([max(c[i] for c in corners) for i in range(3)])
origin = Vector(((low.x + high.x) / 2, (low.y + high.y) / 2, low.z))
for o in meshes:
    o.location -= origin
bpy.ops.object.select_all(action='DESELECT')
for o in meshes:
    o.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
bpy.ops.object.join()
combined = bpy.context.object
combined.name = 'SM_' + kind.title()
bpy.ops.export_scene.fbx(filepath=str(destination / (combined.name + '.fbx')),
    use_selection=True, object_types={'MESH'}, add_leaf_bones=False,
    path_mode='COPY', embed_textures=False, axis_forward='-Y', axis_up='Z')
report = {'asset': combined.name, 'source': str(source), 'dimensions_m': list(high-low),
          'normalization': 'world -Y elevation to +Z, XY centered, base at Z=0',
          'fbx': str(destination / (combined.name + '.fbx'))}
(destination / 'conversion.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
