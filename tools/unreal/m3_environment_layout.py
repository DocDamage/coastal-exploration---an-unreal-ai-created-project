"""Produce deterministic original dock geometry and offline dressing placements."""
import json
import math
import random
from pathlib import Path
from m2_geometry import Obj, terrain, SPEC

ROOT = Path(__file__).resolve().parents[2]
CONFIG = json.loads((ROOT / 'data/m3_environment.json').read_text())


def basin_height(x, y):
    height = terrain(x, y)[0]
    basin = CONFIG['dock_basin']
    side = max(0, min(1, (basin['half_width'] - abs(x-basin['center_x'])) / basin['side_fade']))
    end = max(0, min(1, (basin['landward_y']-y) / basin['landward_fade']))
    blend = (side*side*(3-2*side)) * (end*end*(3-2*end))
    return height + (min(height, basin['bed_height'])-height)*blend


def generate(output):
    output.mkdir(parents=True, exist_ok=True)
    dock = Obj()
    # Cosmetic board tops are 2cm above the existing continuous collision deck.
    for y in range(-2035, 945, 24):
        dock.box((12500, y, 251), (358, 22, 2), 'Trail')
    for y in range(937, 1963, 26):
        dock.box((12500, y, 251), (1098, 24, 2), 'Trail')
    dock.write(output / 'SM_M3DockBoards.obj')
    ground = Obj()
    xmin, xmax, ymin, ymax = SPEC['world_bounds']
    xs, ys = list(range(xmin, xmax+160, 160)), list(range(ymin, ymax+160, 160))
    for y in ys:
        for x in xs:
            ground.vertex((x, y, basin_height(x, y)))
    for j in range(len(ys)-1):
        for i in range(len(xs)-1):
            a = j*len(xs)+i+1
            ground.face([a, a+1, a+1+len(xs), a+len(xs)], 'Ground')
    ground.write(output / 'SM_M3CoastalTerrain.obj')
    rng = random.Random(CONFIG['seed'])
    result = {'rocks': [], 'grass': []}
    for _ in range(20000):
        x, y = rng.uniform(-4200, 17400), rng.uniform(-4500, 11900)
        z, distance = terrain(x, y)
        if distance < CONFIG['minimum_route_clearance_cm']:
            continue
        if min(math.hypot(x-a[0], y-a[1]) for a in SPEC['areas'].values()) < CONFIG['minimum_area_clearance_cm']:
            continue
        category = 'rocks' if -130 < z < 70 else 'grass' if 110 < z < 950 else None
        if category is None or len(result[category]) >= CONFIG['shore_rock_count' if category == 'rocks' else 'grass_patch_count']:
            continue
        if category == 'grass' and max(abs(terrain(x+60, y)[0]-z), abs(terrain(x, y+60)[0]-z)) > 32:
            continue
        if any(math.hypot(x-p['position'][0], y-p['position'][1]) < 240 for p in result[category]):
            continue
        result[category].append({'position': [round(x, 2), round(y, 2), round(z, 2)],
                                 'yaw': rng.uniform(0, 360), 'scale': rng.uniform(.65, 1.1)})
    if len(result['rocks']) != CONFIG['shore_rock_count'] or len(result['grass']) != CONFIG['grass_patch_count']:
        raise RuntimeError('Dressing budget could not be placed within the clearance rules')
    result['route_clearance_cm'] = min(terrain(*p['position'][:2])[1] for group in result.values() for p in group)
    (output / 'placements.json').write_text(json.dumps(result, indent=2))
    print({k: len(result[k]) for k in ('rocks', 'grass')}, 'minimum route clearance:', result['route_clearance_cm'])


if __name__ == '__main__':
    generate(ROOT.parent / 'local-evidence/m3-environment-source')
