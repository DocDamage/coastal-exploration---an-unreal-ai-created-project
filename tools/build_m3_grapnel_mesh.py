"""Build original three-prong grapnel geometry in centimetres for the rope tip."""
import json
import math
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
destination=ROOT/'local-evidence/m3-grapnel-source/grapnel.obj'
destination.parent.mkdir(parents=True,exist_ok=True)
vertices,faces=[],[]

def add(a,b): return tuple(x+y for x,y in zip(a,b))
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def mul(a,s): return tuple(x*s for x in a)
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def unit(a): return mul(a,1/math.sqrt(sum(x*x for x in a)))

def tube(points,radii,sides=12):
    start=len(vertices)
    for i,point in enumerate(points):
        tangent=unit(sub(points[min(i+1,len(points)-1)],points[max(i-1,0)]))
        reference=(0,0,1) if abs(tangent[2])<.9 else (0,1,0)
        n=unit(cross(tangent,reference)); b=cross(tangent,n)
        for j in range(sides):
            a=2*math.pi*j/sides
            vertices.append(add(point,mul(add(mul(n,math.cos(a)),mul(b,math.sin(a))),radii[i])))
    for i in range(len(points)-1):
        for j in range(sides):
            a=start+i*sides+j; b=start+i*sides+(j+1)%sides
            c=b+sides; d=a+sides
            faces.extend([(a,b,c),(a,c,d)])
    for j in range(1,sides-1):
        faces.append((start,start+j+1,start+j))
        end=start+(len(points)-1)*sides
        faces.append((end,end+j,end+j+1))

tube([(0,0,0),(15,0,0),(20,0,0)],[.85,.85,.035],16)
for prong in range(3):
    az=2*math.pi*prong/3
    profile=[(13,0),(16,2),(17,4),(16,6),(13,8),(10,8.5),(7,7)]
    points=[(x,r*math.cos(az),r*math.sin(az)) for x,r in profile]
    tube(points,[.8,.8,.8,.75,.65,.45,.025])
ring=[(-2+2.2*math.cos(2*math.pi*i/32),2.2*math.sin(2*math.pi*i/32),0) for i in range(33)]
tube(ring,[.55]*len(ring))
text=['# Original Coastal three-prong grapnel. Units: centimetres. Forward: +X.','o CoastalGrapnel']
text += ['v '+' '.join(f'{x:.6f}' for x in v) for v in vertices]
text += ['f '+' '.join(str(i+1) for i in f) for f in faces]
destination.write_text('\n'.join(text)+'\n')
print(json.dumps(dict(path=str(destination),vertices=len(vertices),triangles=len(faces),tip=[20,0,0],rope=[-4.2,0,0])))
