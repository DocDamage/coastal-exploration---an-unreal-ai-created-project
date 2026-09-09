"""Measure the assembled barrel direction in each retargeted final aiming pose."""
import json
import math
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[3]
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('Run authored-map bearing check outside PIE')
directors=u.GameplayStatics.get_all_actors_of_class(world,u.CoastalCombatDirector)
if len(directors)!=1:
    raise RuntimeError('Expected one authored director')
relative=directors[0].weapon_visual_transform
rows=[]
for direction,expected in [('Front',(0,1,0)),('Right',(-1,0,0)),('Back',(0,-1,0)),('Left',(1,0,0))]:
    clip=u.load_asset('/Game/Coastal/Character/Animations/CA_AS_Pistol_Point_'+direction)
    pose=u.AnimPoseExtensions.get_anim_pose_at_time(clip,clip.get_play_length(),u.AnimPoseEvaluationOptions())
    hand=u.AnimPoseExtensions.get_bone_pose(pose,'hand_l',u.AnimPoseSpaces.WORLD)
    mesh=u.MathLibrary.compose_transforms(relative,hand)
    forward=u.MathLibrary.transform_direction(mesh,u.Vector(1,0,0))
    dot=sum(a*b for a,b in zip(forward.to_tuple(),expected))
    angle=math.degrees(math.acos(max(-1,min(1,dot))))
    rows.append(dict(direction=direction,barrel=list(forward.to_tuple()),error_degrees=angle,passed=angle<5))
report=dict(passed=all(r['passed'] for r in rows),rows=rows,tolerance_degrees=5,
    scope='Final animation pose geometry; does not replace dynamic/contact visual review')
(ROOT/'local-evidence/m3-pistol-bearings.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
