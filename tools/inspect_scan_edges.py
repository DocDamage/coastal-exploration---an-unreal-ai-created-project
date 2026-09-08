"""Read-only geometry evidence for removing baked water and blending survey edges."""
import bpy
import json
from collections import defaultdict
from pathlib import Path

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath='F:/coastline/LocalVendor/Expansion/Hallsands/SM_Hallsands.fbx',
                         use_image_search=False)
rows = []
for obj in bpy.data.objects:
    if obj.type != 'MESH':
        continue
    vertices = [obj.matrix_world @ v.co for v in obj.data.vertices]
    bins = defaultdict(float)
    materials = defaultdict(lambda: {'polygons': 0, 'area_m2': 0, 'low_z': 1e9, 'high_z': -1e9})
    for p in obj.data.polygons:
        points = [vertices[i] for i in p.vertices]
        lo, hi = min(v.z for v in points), max(v.z for v in points)
        name = obj.data.materials[p.material_index].name
        m = materials[name]
        m['polygons'] += 1
        m['area_m2'] += p.area
        m['low_z'], m['high_z'] = min(m['low_z'], lo), max(m['high_z'], hi)
        if hi-lo < .08 and p.normal.z > .98:
            bins[round((lo+hi)/2, 1)] += p.area
    zs = sorted(v.z for v in vertices)
    rows.append({'object': obj.name, 'vertices': len(vertices), 'polygons': len(obj.data.polygons),
                 'materials': dict(materials), 'planar_elevations_m': sorted(bins.items(), key=lambda kv: -kv[1])[:18],
                 'z_percentiles': {str(p): zs[round((len(zs)-1)*p/100)] for p in (0, 10, 25, 50, 75, 90, 100)}})
Path('F:/coastline/local-evidence/m3-hallsands-edge-inspection.json').write_text(json.dumps(rows, indent=2))
print(json.dumps(rows))
