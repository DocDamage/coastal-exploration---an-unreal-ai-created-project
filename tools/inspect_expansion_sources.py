"""Inspect private source art with Blender, without changing vendor originals."""
import bpy
import json
import sys
from pathlib import Path
from mathutils import Vector

args = sys.argv[sys.argv.index('--') + 1:]
source, output = map(Path, args[:2])
if source.suffix == '.blend':
    bpy.ops.wm.open_mainfile(filepath=str(source))
else:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
meshes = [o for o in bpy.data.objects if o.type == 'MESH']
report = {'source': str(source), 'unit_scale': bpy.context.scene.unit_settings.scale_length,
          'objects': [], 'images': []}
for o in meshes:
    corners = [o.matrix_world @ Vector(c) for c in o.bound_box]
    report['objects'].append({'name': o.name, 'vertices': len(o.data.vertices),
        'polygons': len(o.data.polygons), 'dimensions': list(o.dimensions),
        'min': [min(c[i] for c in corners) for i in range(3)],
        'max': [max(c[i] for c in corners) for i in range(3)],
        'materials': [s.material.name if s.material else None for s in o.material_slots]})
for im in bpy.data.images:
    report['images'].append({'name': im.name, 'path': im.filepath, 'size': list(im.size), 'packed': bool(im.packed_file)})
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, indent=2))
print(json.dumps(report))
