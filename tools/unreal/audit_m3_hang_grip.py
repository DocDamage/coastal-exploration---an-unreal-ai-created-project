"""Record the installed hang clip's hand order and curled-finger grip positions."""
import json
from pathlib import Path
import unreal as u

rows=[]
for path in ('/Game/FreeAnimationSet/Animations/Manny/Hold_Rope_Idle_Anim',
             '/Game/Coastal/Character/Animations/CA_Hold_Rope_Idle_Anim'):
    clip=u.load_asset(path)
    for seconds in (0.0,1.0,2.0,3.0):
        pose=u.AnimPoseExtensions.get_anim_pose_at_time(clip,seconds,u.AnimPoseEvaluationOptions())
        row=dict(clip=path,seconds=seconds)
        for bone in ('hand_r','hand_l','middle_01_r','middle_03_r','middle_01_l','middle_03_l'):
            row[bone]=list(u.AnimPoseExtensions.get_bone_pose(pose,bone,u.AnimPoseSpaces.WORLD).translation.to_tuple())
        rows.append(row)
output=Path(__file__).resolve().parents[3]/'local-evidence/m3-hang-grip-audit.json'
output.write_text(json.dumps(rows,indent=2))
print(str(output))
