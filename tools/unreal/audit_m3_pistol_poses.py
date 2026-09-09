"""Compare component-space hands in source and retargeted aiming clips."""
import json
from pathlib import Path
import unreal as u

rows = []
for direction in ('Front', 'Right', 'Back', 'Left'):
    for source in (True, False):
        path = ('/Game/MotoInteractionAnims/Animations/Combat/Pistol/AS_' if source else
                '/Game/Coastal/Character/Animations/CA_AS_') + 'Pistol_Point_' + direction
        clip = u.load_asset(path)
        for seconds in (0.0, 0.5, 0.99):
            pose = u.AnimPoseExtensions.get_anim_pose_at_time(clip, seconds, u.AnimPoseEvaluationOptions())
            row = dict(direction=direction, source=source, seconds=seconds)
            for bone in ('hand_r', 'hand_l', 'head'):
                transform = u.AnimPoseExtensions.get_bone_pose(pose, bone, u.AnimPoseSpaces.WORLD)
                row[bone] = dict(position=list(transform.translation.to_tuple()),rotation=str(transform.rotation))
            rows.append(row)
destination = Path(__file__).resolve().parents[3] / 'local-evidence/m3-pistol-pose-audit.json'
destination.write_text(json.dumps(rows, indent=2))
print(json.dumps(rows))
