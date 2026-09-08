"""Original authored M2 terrain/radio source. Run with ordinary Python before the Editor recipe."""
import json, math, struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SPEC = json.loads((ROOT / 'data/m2_opening.json').read_text())
PATHS = [SPEC['main_path'], SPEC['overlook_path']]

def closest(x, y, path):
    best = (1e9, 0)
    for a, b in zip(path, path[1:]):
        dx, dy = b[0]-a[0], b[1]-a[1]
        t = max(0, min(1, ((x-a[0])*dx+(y-a[1])*dy)/(dx*dx+dy*dy)))
        d = math.hypot(x-a[0]-t*dx, y-a[1]-t*dy)
        if d < best[0]: best = d, a[2]+t*(b[2]-a[2])
    return best

def terrain(x, y):
    d, z = min((closest(x,y,p) for p in PATHS), key=lambda v:v[0])
    # Broad traversable clearings at all three destinations.
    for ax, ay, az in SPEC['areas'].values():
        pad = 1200 if ax == 0 else 950
        candidate = max(0, math.hypot(x-ax,y-ay)-pad)
        if candidate < d: d,z = candidate,az
    blend = max(0, min(1, (d-220)/1450))
    blend = blend*blend*(3-2*blend)
    shore = -170 + (y+2200)*0.095 + 95*math.sin(x/2300) + 40*math.sin(y/880)
    ridge = 1200*math.exp(-((y-10200)/3300)**2)
    rough = 100*math.sin(x/620)*math.sin(y/1100)
    h = z*(1-blend)+(shore+ridge+rough)*blend
    return h, d

class Obj:
    def __init__(self): self.vertices=[]; self.faces=[]
    def vertex(self, point): self.vertices.append(point); return len(self.vertices)
    def face(self, ids, material): self.faces.append((ids,material))
    def box(self, center, size, material):
        x,y,z=center; a,b,c=[v/2 for v in size]
        ids=[self.vertex((x+dx*a,y+dy*b,z+dz*c)) for dx,dy,dz in
             [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
        for f in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]: self.face([ids[i] for i in f],material)
    def write(self, target):
        # Reflect Y for the UE OBJ basis and reverse triangle winding with it.
        # Two-sided materials hid the old down-facing terrain; physics did not.
        lines=['# Original Coastal Exploration mesh','mtllib '+target.with_suffix('.mtl').name]
        lines += [f'v {x:.4f} {-y:.4f} {z:.4f}' for x,y,z in self.vertices]
        lines += [f'vt {x/600:.5f} {y/600:.5f}' for x,y,z in self.vertices]
        current=None
        for face,material in self.faces:
            if material!=current: lines.append('usemtl '+material); current=material
            for i in range(1,len(face)-1): lines.append('f '+' '.join(f'{v}/{v}' for v in [face[0],face[i+1],face[i]]))
        target.write_text('\n'.join(lines))
        colors={'Ground':(.22,.27,.16),'Trail':(.45,.39,.28),'Shore':(.32,.30,.26),'RadioCase':(.09,.16,.15),'RadioMetal':(.14,.16,.17),'RadioDial':(.85,.58,.20),'RadioSpeaker':(.025,.03,.025)}
        target.with_suffix('.mtl').write_text('\n'.join(f'newmtl {m}\nKd {c[0]} {c[1]} {c[2]}\n' for m,c in colors.items()))

def generate(output):
    output.mkdir(parents=True,exist_ok=True)
    obj=Obj(); xmin,xmax,ymin,ymax=SPEC['world_bounds']; step=160
    xs=list(range(xmin,xmax+step,step)); ys=list(range(ymin,ymax+step,step))
    for y in ys:
        for x in xs: obj.vertex((x,y,terrain(x,y)[0]))
    for j in range(len(ys)-1):
        for i in range(len(xs)-1):
            h,d=terrain(xs[i]+step/2,ys[j]+step/2)
            mat='Trail' if d<210 else 'Shore' if h<100 else 'Ground'
            a=j*len(xs)+i+1; obj.face([a,a+1,a+1+len(xs),a+len(xs)],mat)
    obj.write(output/'SM_FirstSignalTerrain.obj')
    radio=Obj(); radio.box((0,0,15),(42,22,30),'RadioCase')
    radio.box((0,-11.4,16),(38,1,23),'RadioSpeaker')
    for x in range(-17,7,3): radio.box((x,-12.2,16),(1,1,21),'RadioMetal')
    radio.box((13,-12.4,21),(8,1.5,8),'RadioDial')
    radio.box((13,-12.4,9),(5,2,5),'RadioMetal')
    radio.box((0,0,36),(25,3,3),'RadioMetal')
    for x in [-12,12]: radio.box((x,0,32),(3,3,8),'RadioMetal')
    radio.box((-17,6,48),(0.7,0.7,42),'RadioMetal')
    radio.write(output/'SM_FirstSignalRadio.obj')
    # Linear shader-data mask: R = footpath, G = rocky shoreline. TGA stores BGR.
    resolution=512
    pixels=bytearray()
    for j in range(resolution):
        for i in range(resolution):
            x=xmin+(xmax-xmin)*(i+.5)/resolution
            y=ymin+(ys[-1]-ymin)*(j+.5)/resolution
            h,d=terrain(x,y)
            trail=max(0,min(1,(370-d)/180))
            trail=trail*trail*(3-2*trail)
            shore=max(0,min(1,(90-h)/150))
            pixels.extend((0,round(shore*255),round(trail*255)))
    header=struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,resolution,resolution,24,32)
    (output/'T_FirstSignalGroundMask.tga').write_bytes(header+pixels)
    length=lambda p:sum(math.dist(a,b) for a,b in zip(p,p[1:]))/100
    info={'main_one_way_m':length(PATHS[0]),'optional_branch_one_way_m':length(PATHS[1]),'round_trip_with_optional_m':2*sum(length(p) for p in PATHS)}
    (output/'layout.json').write_text(json.dumps(info,indent=2))
    print(info)

if __name__=='__main__':
    generate(ROOT.parent/'local-evidence/m2-original-meshes')
