"""Build original zipline trolley geometry matching the installed two-hand pose."""
import json
import math
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
destination=ROOT/'local-evidence/m3-zipline-source/trolley.obj'
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

# Cable runs along +X through the origin. Hand grips sit 20cm below it.
tube([(0,-28,-20),(0,28,-20)],[1.5,1.5],20)
for x in (-6,6):
    ring=[(x+4.2*math.cos(2*math.pi*i/64),0,5.6+4.2*math.sin(2*math.pi*i/64)) for i in range(65)]
    tube(ring,[.8]*len(ring),12)
    tube([(x,-3,5.6),(x,3,5.6)],[1,1],12)
    for y in (-2.5,2.5):
        tube([(x,y,5.6),(x,y,-8),(0,y,-20)],[.9,.9,.9],12)
text=['# Original Coastal zipline trolley. Units: cm. Cable direction: +X.','o CoastalZiplineTrolley']
text += ['v '+' '.join(f'{x:.6f}' for x in v) for v in vertices]
text += ['f '+' '.join(str(i+1) for i in f) for f in faces]
destination.write_text('\n'.join(text)+'\n')
print(json.dumps(dict(path=str(destination),vertices=len(vertices),triangles=len(faces),left_grip=[0,-25.6,-20],right_grip=[0,25.6,-20])))
