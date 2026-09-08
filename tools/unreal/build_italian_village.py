"""Place three authored vendor houses around a connected village terrace.

Inputs are the static-art reports exported from the vendor's three house maps.
Vendor gameplay, lighting, weather and world settings are not copied.
"""
import json
import sys
from pathlib import Path
import unreal as u

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
from m3_destination_authoring import Destination

OUT = Path('F:/coastline/local-evidence')
ROOT = '/Game/ItalianMedievalTown/'
scenes = []
cache = {}
for number in (1, 2, 3):
    rows = json.loads((OUT / f'm3-omitted-house{number}-scene.json').read_text())
    if not rows or len(rows) > 450:
        raise RuntimeError('Expected a bounded authored house scene')
    corners = []
    for row in rows:
        path = row['mesh']
        if not path.startswith(ROOT):
            raise RuntimeError('Unexpected house dependency outside vendor root: ' + path)
        if path not in cache:
            cache[path] = u.load_asset(path)
        sm = cache[path]
        if not isinstance(sm, u.StaticMesh):
            raise RuntimeError('Missing house mesh: ' + path)
        rotation = u.Rotator(pitch=row['rotation'][0], yaw=row['rotation'][1], roll=row['rotation'][2])
        transform = u.Transform(location=u.Vector(*row['position']), rotation=rotation,
                                scale=u.Vector(*row['scale']))
        bounds = sm.get_bounds()
        for x in (-1, 1):
            for y in (-1, 1):
                for z in (-1, 1):
                    corner = bounds.origin + u.Vector(bounds.box_extent.x * x,
                                                       bounds.box_extent.y * y,
                                                       bounds.box_extent.z * z)
                    corners.append(u.MathLibrary.transform_location(transform, corner).to_tuple())
    lower = [min(c[i] for c in corners) for i in range(3)]
    upper = [max(c[i] for c in corners) for i in range(3)]
    size = [b - a for a, b in zip(lower, upper)]
    if size[0] > 2000 or size[1] > 2200:
        raise RuntimeError('Authored house exceeds terrace layout; inspect bounds before placement: ' + str(size))
    scenes.append((number, rows, lower, upper))

floor = u.load_asset(ROOT + 'Meshes/Floor/SM_Floor_01a')
if not isinstance(floor, u.StaticMesh) or not floor.get_material(0):
    raise RuntimeError('Village floor material is unavailable')
material = floor.get_material(0).get_path_name()
d = Destination('medieval_italian_village')
retaining = '/Game/Coastal/M3/Materials/M_VillageRetainingStone'
if not u.EditorAssetLibrary.does_asset_exist(retaining):
    retaining = ROOT + 'Materials/MI_Stones_01a'
d.box('Masonry terrace', (-4500, 14600, 1050), (6800, 4200, 2260),
      retaining)
d.box('Courtyard walking surface', (-4500, 14600, 2190), (6800, 4200, 20), material, hidden=True)
for x in range(-7, 8):
    for y in range(-4, 5):
        d.fitted(f'Courtyard paving {x} {y}', ROOT + 'Meshes/Floor/SM_Floor_01a',
                 (-4500 + x * 450, 14600 + y * 450, 2199), (450, 450, 2))

for number, rows, lower, upper in scenes:
    center = (-6800 + (number - 1) * 2300, 13800, 2200)
    shift = (center[0] - (lower[0] + upper[0]) / 2,
             center[1] - (lower[1] + upper[1]) / 2,
             center[2] - lower[2])
    for index, row in enumerate(rows):
        actor = d.mesh(f'House {number} art {index}', cache[row['mesh']],
                       tuple(v + off for v, off in zip(row['position'], shift)),
                       row['rotation'][1], row['scale'], True, row['materials'])
        actor.set_actor_rotation(u.Rotator(pitch=row['rotation'][0], yaw=row['rotation'][1],
                                          roll=row['rotation'][2]), False)

d.mesh('Village well', ROOT + 'Meshes/Props/SM_Well_01a', (-4500, 15350, 2200))
for index, x in enumerate((-5700, -3300)):
    d.mesh('Market table ' + str(index), ROOT + 'Meshes/Props/SM_Table_01a', (x, 15800, 2200))
    d.mesh('Market barrel ' + str(index), ROOT + 'Meshes/Props/SM_Barrel_01a', (x - 160, 15920, 2200))
d.path('Village approach ', [(2000, 15000, 1600), (-1000, 15300, 2200), (-1600, 15300, 2200)],
       '/Game/Coastal/M2/Materials/M_WeatheredTimber', width=520, rails=True)
arrival = d.checkpoint('Medieval Italian Village', (-1600, 15300, 2200))
d.sign('Trail direction', 'Medieval Italian Village', (1800, 14620, 1830))
actors = d.save()
report = {'destination': 'medieval_italian_village', 'arrival': arrival, 'actors': actors,
          'houses': [{'number': n, 'source_min': low, 'source_max': high, 'pieces': len(rows)}
                     for n, rows, low, high in scenes],
          'scope': 'Three vendor-authored house exteriors and courtyard; traversal and visual acceptance separate'}
(OUT / 'm3-omitted-village-placement.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps({'destination': report['destination'], 'actors': len(actors), 'arrival': arrival}))
