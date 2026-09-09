"""Build dry Atlantis upper terraces and a partly submerged lower court from owned art."""
import json
import sys
from pathlib import Path
import unreal as u
HERE=Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0,str(HERE))
from m3_destination_authoring import Destination
ROOT='/Game/Atlantis_Ruins/Meshes/'
NAMES=['SM_Ceiling_00','SM_Arch_00','SM_Column_00','SM_Statue_00','SM_Statue_01',
       'SM_Wall_00','SM_Rocks_Large_02','SM_Coral_00']
loaded={name:u.load_asset(ROOT+name) for name in NAMES}
if any(not isinstance(a,u.StaticMesh) for a in loaded.values()):
    raise RuntimeError('Atlantis content is incomplete or not loadable')
material=loaded['SM_Ceiling_00'].get_material(0).get_path_name()
d=Destination('atlantis')
d.box('Upper terrace foundation',(-18000,6000,-20),(4200,3000,440),material)
for x in range(7):
    for y in range(5):
        d.fitted(f'Terrace paving {x} {y}',ROOT+'SM_Ceiling_00',
                 (-19800+x*600,4800+y*600,201),(600,600,2))
# Colonnades frame a broad central east-west route, with no required submerged interaction.
for i,x in enumerate((-19800,-18800,-17800,-16800)):
    for side in (-1,1):
        d.fitted(f'Colonnade {i} {side}',ROOT+'SM_Column_00',(x,6000+side*1300,700),(170,170,1000))
        d.box(f'Column collision {i} {side}',(x,6000+side*1300,700),(120,120,1000),material,hidden=True)
for i,x in enumerate((-17700,-19000)):
    # The supplied mesh is one half of an arch; pair its crown ends at the route center.
    d.fitted(f'Central arch {i}',ROOT+'SM_Arch_00',(x,6300,700),(600,200,1000),yaw=-90)
    d.fitted(f'Central arch opposite {i}',ROOT+'SM_Arch_00',(x,5700,700),(600,200,1000),yaw=90)
    for side in (-1,1):
        d.box(f'Arch pier collision {i} {side}',(x,6000+side*530,650),(140,140,900),material,hidden=True)
    d.box(f'Arch lintel collision {i}',(x,6000,1160),(180,1200,80),material,hidden=True)
for side in (-1,1):
    d.fitted(f'Upper statue {side}',ROOT+('SM_Statue_00' if side<0 else 'SM_Statue_01'),
             (-19500,6000+side*870,650),(280,280,900),yaw=90)
    d.box(f'Statue plinth {side}',(-19500,6000+side*870,290),(340,340,180),material)
    d.fitted(f'Upper side wall {side}',ROOT+'SM_Wall_00',(-18200,6000+side*1500,380),(110,3000,360),yaw=90)
    d.box(f'Upper side barrier {side}',(-18200,6000+side*1500,350),(3000,65,300),material,hidden=True)
d.fitted('West wall',ROOT+'SM_Wall_00',(-20100,6000,425),(100,2600,450))
d.box('West barrier',(-20100,6000,400),(60,2600,400),material,hidden=True)
# A lower court remains scenery visible across the water from the upper terrace.
d.box('Submerged lower court',(-22300,6000,-450),(4200,3800,100),material)
for i,x in enumerate((-21500,-22800,-23900)):
    for side in (-1,1):
        d.fitted(f'Drowned column {i} {side}',ROOT+'SM_Column_00',
                 (x,6000+side*1400,-100),(210,210,650))
        d.fitted(f'Coral {i} {side}',ROOT+'SM_Coral_00',
                 (x+200,6000+side*1100,-210),(360,360,350))
for i,(x,y) in enumerate(((-21000,4100),(-22300,8200),(-24100,5000))):
    d.fitted(f'Shore reef {i}',ROOT+'SM_Rocks_Large_02',(x,y,-80),(1000,800,700))
route=[(-6500,22000,94),(-8500,20000,94),(-11500,17000,160),
       (-14500,13000,200),(-16000,8500,200),(-16000,6200,200)]
d.path('Hallsands access ',route,material,width=500,rails=True)
d.sign('Hallsands direction','Atlantis upper terraces',(-6800,21600,320),yaw=45)
d.sign('Terrace identity','ATLANTIS RUINS',(-16100,6500,440),yaw=90)
arrival=d.checkpoint('Atlantis Ruins',(-16000,6000,200))
written={a.get_path_name() for a in d.written}
for actor in d.known.values():
    if actor.get_path_name() not in written:
        d.actors.destroy_actor(actor)
actors=d.save()
report={'destination':'atlantis','arrival':arrival,'route_ground':route,
        'interior_walk':[(-17000,6000,298),(-18300,6000,298),(-19800,6000,298),
                         (-18300,6000,298),(-18300,6900,298),(-17000,6900,298)],
        'actors':actors,'runtime_verified':False,
        'scope':'Dry upper terrace route and tide-survey arrival; lower court is partly submerged scenery, no diving required.'}
(Path(__file__).resolve().parents[3]/'local-evidence/m3-atlantis-placement.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'destination':'atlantis','actors':len(actors),'arrival':arrival}))
