import json
import math
from pathlib import Path
import unreal as u
root=Path(__file__).resolve().parents[3]
source=json.loads((root/'local-evidence/m3-prone-retarget.json').read_text(encoding='utf-8'))
rows=[]
for item in source:
    clip=u.load_asset(item['output'])
    if not clip or clip.get_editor_property('enable_root_motion') or not clip.get_editor_property('force_root_lock'):
        raise RuntimeError('Crawl clips must use movement-owned translation: '+item['cue'])
    frames=[]
    for fraction in (0,.25,.5,.75,1):
        seconds=clip.get_play_length()*fraction
        pose=u.AnimPoseExtensions.get_anim_pose_at_time(clip,seconds,u.AnimPoseEvaluationOptions())
        bones={b:list(u.AnimPoseExtensions.get_bone_pose(pose,b,u.AnimPoseSpaces.WORLD).translation.to_tuple())
               for b in ('root','pelvis','head','hand_l','hand_r','foot_l','foot_r')}
        if not all(math.isfinite(v) for xyz in bones.values() for v in xyz):
            raise RuntimeError('Invalid retarget pose: '+item['cue'])
        frames.append(dict(seconds=seconds,bones=bones))
    rows.append(dict(cue=item['cue'],clip=item['output'],length=clip.get_play_length(),frames=frames))
output=root/'local-evidence/m3-prone-pose-audit.json'
output.write_text(json.dumps(rows,indent=2),encoding='utf-8')
print(str(output))
