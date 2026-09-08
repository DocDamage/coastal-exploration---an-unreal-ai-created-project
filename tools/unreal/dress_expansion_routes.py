"""Add deterministic, nonblocking planting and stones to the graded route shoulders."""
import json
import math
import random
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
from m3_destination_authoring import Destination

ROOT = Path(__file__).resolve().parents[2]
SPEC = json.loads((ROOT / 'data/m3_expansion.json').read_text(encoding='utf-8'))
d = Destination('route_dressing')
rng = random.Random(90826)
assets = {
    'grass': '/Game/Fishermans_Cabin/Meshes/Foliage/Grass/SM_Grass_03',
    'bush': '/Game/Fishermans_Cabin/Meshes/Foliage/Bush/SM_Bush_02',
    'stone': '/Game/Fishermans_Cabin/Meshes/Small_Rocks/SM_Small_Rocks_03',
}
for path in assets.values():
    if not isinstance(u.load_asset(path), u.StaticMesh):
        raise RuntimeError('Missing required dressing mesh: ' + path)

placements = []
for route in SPEC['routes']:
    if route.get('kind') == 'boardwalk':
        continue
    for segment, (a, b) in enumerate(zip(route['points'], route['points'][1:])):
        dx, dy, dz = [end - start for start, end in zip(a, b)]
        length = math.hypot(dx, dy)
        # Avoid junctions and destination approaches. The actual generated trail
        # has a 350 cm half-width and shoulders descending to -250 at 1400 cm.
        count = int((length - 1100) // 850)
        for index in range(max(0, count)):
            t = (600 + (index + .5) * (length - 1200) / count) / length
            for side in (-1, 1):
                offset = side * rng.uniform(590, 760)
                height = a[2] + dz * t
                ground = height + (-250 - height) * (abs(offset) - 350) / 1050
                if ground < 25:
                    continue
                kind = ('grass', 'stone', 'grass', 'bush')[(index + segment + (side > 0)) % 4]
                size = {'grass': (140, 130, 105), 'stone': (155, 115, 90),
                        'bush': (180, 170, 145)}[kind]
                x, y = a[0] + dx * t - dy / length * offset, a[1] + dy * t + dx / length * offset
                # Slight burial seats the irregular asset in the graded surface.
                burial = 35 if kind == 'stone' else 18
                center = (x, y, ground + size[2] / 2 - burial)
                key = f'{route["id"]}.{segment}.{index}.{side}.{kind}'
                actor = d.fitted(key, assets[kind], center, size, rng.uniform(0, 360), False)
                placements.append({'tag': key, 'kind': kind, 'ground_z': ground,
                                   'center': list(center), 'collision': 'NoCollision'})

rows = d.save()
report = {'actors_written': len(rows), 'placements': placements,
          'minimum_center_distance_from_route_axis_cm': 590,
          'central_walkway_half_width_cm': 350, 'visual_acceptance': 'pending'}
Path('F:/coastline/local-evidence/m3-route-dressing.json').write_text(
    json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps({'actors_written': len(rows), 'visual_acceptance': 'pending'}))
