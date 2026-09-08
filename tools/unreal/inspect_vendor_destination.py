"""Extract vendor scene static art transforms without installing its gameplay owners.

Supply scene_path and report_name globals through the editor Python bridge.
Loading a large source world may build its derived assets on first use.
"""
import json
from pathlib import Path
import unreal as u

vendor = u.load_asset(scene_path)
if not isinstance(vendor, u.World):
    raise RuntimeError('Vendor map missing: ' + scene_path)
rows = []
for actor in u.GameplayStatics.get_all_actors_of_class(vendor, u.Actor):
    for c in actor.get_components_by_class(u.StaticMeshComponent):
        sm = c.static_mesh
        if not sm:
            continue
        transforms = [c.get_world_transform()]
        if isinstance(c, u.InstancedStaticMeshComponent):
            transforms = [c.get_instance_transform(i, world_space=True) for i in range(c.get_instance_count())]
        for transform in transforms:
            p, r, s = transform.translation, transform.rotation.rotator(), transform.scale3d
            rows.append({'actor': actor.get_actor_label(), 'mesh': sm.get_path_name(),
                'position': list(p.to_tuple()), 'rotation': [r.pitch, r.yaw, r.roll], 'scale': list(s.to_tuple()),
                'materials': [m.get_path_name() if m else None for m in c.get_materials()]})
out = Path('F:/coastline/local-evidence') / report_name
out.write_text(json.dumps(rows, indent=2))
print(json.dumps({'source': scene_path, 'static_instances': len(rows), 'report': str(out)}))
