"""Read collision at the new arrival points before adding recovery checkpoints."""
import json
from pathlib import Path
import unreal as u

w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
points=[]
for name,xs,ys in [('hallsands',[-5500,-6500,-7500], [21000,22000,23000]),
                   ('baelo',[7000,8000,9000],[17300,17900,18500])]:
    for x in xs:
        for y in ys:
            points.append((name,x,y))
points += [('powell',25500,4000),('harbour',28200,23000),('sea_platform',38600,-14000),('camp',7600,9100)]
rows=[]
for name,x,y in points:
    result=u.SystemLibrary.line_trace_single(w,u.Vector(x,y,15000),u.Vector(x,y,-2500),
        u.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],u.DrawDebugTrace.NONE)
    if result is None:
        rows.append({'destination':name,'xy':[x,y],'blocking':False})
        continue
    hit=result[0].to_tuple() if isinstance(result,tuple) else result.to_tuple()
    rows.append({'destination':name,'xy':[x,y],'blocking':hit[0],
        'impact':list(hit[5].to_tuple()),'normal':list(hit[7].to_tuple()),
        'actor':hit[9].get_actor_label() if hit[9] else None})
out=Path('F:/coastline/local-evidence/m3-expansion-arrival-traces.json')
out.write_text(json.dumps(rows,indent=2))
print(json.dumps(rows))
