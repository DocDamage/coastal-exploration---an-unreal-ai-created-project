"""Generate continuous, graded walking surfaces for the destination expansion."""
import json
import math
from pathlib import Path
from m2_geometry import Obj

ROOT = Path(__file__).resolve().parents[2]
SPEC = json.loads((ROOT / 'data/m3_expansion.json').read_text())


def generate(destination):
    destination.mkdir(parents=True, exist_ok=True)
    report = []
    for route in SPEC['routes']:
        mesh = Obj()
        width = 220 if route.get('kind') == 'boardwalk' else 350
        max_grade = 0
        for a, b in zip(route['points'], route['points'][1:]):
            dx, dy, dz = [y-x for x, y in zip(a, b)]
            length = math.hypot(dx, dy)
            max_grade = max(max_grade, abs(dz)/length)
            nx, ny = -dy/length, dx/length
            sections = max(1, math.ceil(length/180))
            previous = None
            offsets = [-width, width] if route.get('kind') == 'boardwalk' else [-1400, -width, width, 1400]
            for i in range(sections+1):
                t = i/sections
                row = []
                for off in offsets:
                    height = a[2]+dz*t
                    if abs(off) == 1400:
                        height = -250
                    row.append(mesh.vertex((a[0]+dx*t+nx*off, a[1]+dy*t+ny*off, height)))
                if previous:
                    for k in range(len(row)-1):
                        mesh.face([previous[k], row[k], row[k+1], previous[k+1]], 'Trail' if len(row)==2 or k==1 else 'Ground')
                previous = row
        mesh.write(destination / ('SM_Expansion_' + route['id'] + '.obj'))
        report.append({'route': route['id'], 'maximum_grade': max_grade,
                       'length_m': sum(math.dist(a,b) for a,b in zip(route['points'],route['points'][1:]))/100})
    (destination / 'routes.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report))


if __name__ == '__main__':
    generate(ROOT.parent / 'local-evidence/m3-expansion-source')
