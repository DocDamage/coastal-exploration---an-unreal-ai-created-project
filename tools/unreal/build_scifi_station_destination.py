"""Assemble a walkable coastal monitoring station from owned Modular SciFi Station art."""
import json
import sys
from pathlib import Path
import unreal as u
HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
from m3_destination_authoring import Destination
ROOT = '/Game/ModularSciFiStation/Environment/'
ASSETS = {
    'floor': ROOT + 'Floor/SM_floor_400x400_01',
    'wall': ROOT + 'Walls/A/SM_wall_400_a_01',
    'panel': ROOT + 'Props/WallAttachments/SM_control_panel_01',
    'antenna': ROOT + 'Props/Radar/SM_antenna_01',
    'radar': ROOT + 'Props/Radar/SM_radar_sphere_01',
    'pillar': ROOT + 'Scaffold/SM_hydraulic_pillar_400_01',
    'vent': ROOT + 'Props/Ventilation/SM_vent_01',
}
loaded = {key: u.load_asset(path) for key, path in ASSETS.items()}
if any(not isinstance(asset, u.StaticMesh) for asset in loaded.values()):
    raise RuntimeError('Station content is incomplete or not loadable')
material = loaded['floor'].get_material(0).get_path_name()
d = Destination('station')
# A continuous collision deck keeps decorative module seams out of locomotion.
d.box('Deck collision', (33000, -29400, 582), (2400, 2400, 36), material, hidden=True)
for x in range(6):
    for y in range(6):
        d.fitted(f'Floor {x} {y}', ASSETS['floor'],
                 (32000 + x*400, -30400 + y*400, 599), (400, 400, 2))
for row in range(6):
    x, y = 32000 + row*400, -30400 + row*400
    for side in (-1, 1):
        d.fitted(f'Side wall {side} {row}', ASSETS['wall'],
                 (33000 + side*1200, y, 800), (30, 400, 400), yaw=180 if side>0 else 0)
        d.box(f'Side collision {side} {row}', (33000 + side*1200, y, 800),
              (24, 400, 400), material, hidden=True)
    d.fitted(f'Back wall {row}', ASSETS['wall'], (x, -30600, 800), (30,400,400),yaw=90)
    d.box(f'Back collision {row}', (x,-30600,800), (400,24,400), material, hidden=True)
    # An 800cm opening faces the access boardwalk.
    if row not in (2,3):
        d.fitted(f'Front wall {row}', ASSETS['wall'], (x,-28200,800), (30,400,400),yaw=90)
        d.box(f'Front collision {row}', (x,-28200,800), (400,24,400), material, hidden=True)
    for strip in range(3):
        d.fitted(f'Roof {row} {strip}', ASSETS['floor'],
                 (x,-30200+strip*800,1010), (400,800,20))
for side in (-1,1):
    for end in (-1,1):
        d.fitted(f'Deck support {side} {end}', ASSETS['pillar'],
                 (33000+side*1050,-29400+end*1050,280),(100,100,640))
        d.fitted(f'Vent {side} {end}', ASSETS['vent'],
                 (33000+side*1070,-29400+end*700,790),(100,130,180))
for i in range(3):
    d.fitted(f'Monitoring panel {i}', ASSETS['panel'],
             (32400+i*600,-30480,785),(180,45,180),yaw=180)
    lamp=d.actor(u.PointLight, f'Interior light {i}', (32400+i*600,-29400,940))
    light=lamp.get_component_by_class(u.PointLightComponent)
    light.set_mobility(u.ComponentMobility.MOVABLE)
    light.set_editor_property('intensity',2500)
    light.set_editor_property('attenuation_radius',1200)
    light.set_editor_property('light_color',u.Color(190,220,255,255))
    light.set_editor_property('cast_shadows',False)
d.fitted('Roof radar',ASSETS['radar'],(33700,-29700,1390),(500,500,700))
d.fitted('Roof antenna',ASSETS['antenna'],(32300,-29600,1430),(160,160,800))
route=[(38600,-14000,800),(36600,-17800,800),(34600,-22600,700),
       (33000,-26200,600),(33000,-28400,600)]
d.path('Platform access ',route,material,width=500,rails=True)
d.sign('Platform direction','Outer monitoring station',(38300,-14700,1020),yaw=90)
d.sign('Entrance identity','COASTAL MONITORING STATION',(33000,-28060,1030),yaw=90)
arrival=d.checkpoint('Modular SciFi Station',(33000,-28500,600))
actors=d.save()
report={'destination':'station','arrival':arrival,'route_ground':route,
        'interior_walk':[(33000,-29400,698),(32400,-30000,698),(33600,-30000,698)],
        'actors':actors,'runtime_verified':False,
        'scope':'Original compact monitoring room, supplied modular art, open entrance and sea-platform access.'}
(Path(__file__).resolve().parents[3]/'local-evidence/m3-station-placement.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'destination':'station','actors':len(actors),'arrival':arrival}))
