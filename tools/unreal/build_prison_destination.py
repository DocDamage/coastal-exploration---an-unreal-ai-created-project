"""Build a compact prison cell block and harbour approach using installed art."""
import json
import sys
from pathlib import Path
import unreal as u

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
from m3_destination_authoring import Destination

ROOT = '/Game/HAUNTED_PRISON/Meshes/'
REQUIRED = ['Interior/SM_Floor', 'Interior/SM_Wall_Int', 'Interior/SM_Wall_Door_Prison',
            'Interior/SM_PrisonGrid', 'Assets/SM_Bed', 'Assets/SM_Toilet',
            'Exterior/Moduls/SM_WallExt_A', 'Exterior/Moduls/SM_Roof']
loaded = {key: u.load_asset(ROOT + key) for key in REQUIRED}
if any(not isinstance(sm, u.StaticMesh) for sm in loaded.values()):
    raise RuntimeError('Prison content is not completely installed or loadable')
floor_material = loaded['Interior/SM_Floor'].get_material(0).get_path_name()
wall_material = loaded['Interior/SM_Wall_Int'].get_material(0).get_path_name()
d = Destination('haunted_prison')
d.box('Island foundation', (20500, 29000, 120), (3400, 3400, 540), wall_material)
d.box('Courtyard collision', (20500, 29000, 382), (3400, 3400, 36), floor_material, hidden=True)
# Separate thin art tiles retain their original material scale and a level walk.
for x in range(-3, 4):
    for y in range(-3, 4):
        d.fitted(f'Courtyard paving {x} {y}', ROOT + 'Interior/SM_Floor',
                 (20500 + x * 480, 29000 + y * 480, 399), (480, 480, 2))

# Six open cells face a generous central gallery. These are architectural
# openings; no unimplemented lock/door interaction or persistent state is added.
for row in (-1, 1):
    for cell in range(3):
        x = 19900 + cell * 600
        y = 29000 + row * 650
        d.fitted(f'Cell back {row} {cell}', ROOT + 'Interior/SM_Wall_Int',
                 (x, y + row * 350, 600), (600, 30, 400))
        d.box(f'Cell back collision {row} {cell}', (x, y + row * 350, 600),
              (600, 25, 400), wall_material, hidden=True)
        # Original doorway wall is 7.5m tall; shrinking it also shrinks the
        # doorway. Two full-height bar sections leave a measured 2.2m opening.
        for side in (-1, 1):
            d.fitted(f'Cell bars {row} {cell} {side}', ROOT + 'Interior/SM_PrisonGrid',
                     (x + side * 205, y - row * 350, 565), (190, 12, 330))
            d.box(f'Bar side collision {row} {cell} {side}',
                  (x + side * 205, y - row * 350, 600), (190, 22, 400), wall_material, hidden=True)
        d.box(f'Cell lintel {row} {cell}', (x, y - row * 350, 775), (600, 30, 50), wall_material)
        for side in (-1, 1):
            if side == -1 or cell == 2:
                d.fitted(f'Cell partition {row} {cell} {side}', ROOT + 'Interior/SM_Wall_Int',
                         (x + side * 300, y, 600), (700, 30, 400), yaw=90)
                d.box(f'Partition collision {row} {cell} {side}',
                      (x + side * 300, y, 600), (25, 700, 400), wall_material, hidden=True)
        d.fitted(f'Cell bed {row} {cell}', ROOT + 'Assets/SM_Bed',
                 (x - 140, y + row * 50, 423), (102, 216, 46), collision=True)
        d.fitted(f'Cell washroom {row} {cell}', ROOT + 'Assets/SM_Toilet',
                 (x + 170, y + row * 200, 440), (65, 75, 80), collision=True)
        # Roof leaves the gallery open to the existing world lighting.
        d.fitted(f'Cell roof {row} {cell}', ROOT + 'Exterior/Moduls/SM_Roof',
                 (x, y, 975), (610, 750, 350))

d.path('Harbour approach ', [(28200, 23000, 350), (26400, 24900, 350),
                            (24400, 26600, 400), (22500, 27800, 400),
                            (21600, 29000, 400)],
       '/Game/Coastal/M2/Materials/M_WeatheredTimber', width=520, rails=True)
d.sign('Harbour direction', 'Haunted Prison', (27900, 23900, 580), yaw=-45)
arrival = d.checkpoint('Haunted Prison', (21600, 29000, 400))
rows = d.save()
report = {'destination': 'haunted_prison', 'arrival': arrival, 'actors': rows,
          'scope': 'Six-cell modular assembly; walking and presentation acceptance recorded separately'}
Path('F:/coastline/local-evidence/m3-omitted-prison-placement.json').write_text(
    json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps({'destination': 'haunted_prison', 'actors': len(rows), 'arrival': arrival}))
