"""Persist the visually reviewed left-hand pistol presentation; back up the map."""
import hashlib
import json
import shutil
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
project = ROOT/'LocalHost58/CoastalExploration'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (project/'CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if level.is_in_play_in_editor():
    raise RuntimeError('End PIE before binding')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if world.get_path_name() != '/Game/Coastal/Maps/L_FirstSignal.L_FirstSignal':
    raise RuntimeError('Expected gameplay map')
if u.EditorLoadingAndSavingUtils.get_dirty_map_packages():
    raise RuntimeError('Inspect existing map edits first')
map_file = project/'Content/Coastal/Maps/L_FirstSignal.umap'
backup = ROOT/'local-evidence/m3-activities-asset-backups/L_FirstSignal-before-pistol.umap'
backup.parent.mkdir(parents=True,exist_ok=True)
if not backup.exists():
    shutil.copy2(map_file,backup)
mesh = u.load_asset('/Game/Coastal/Activities/Combat/SM_CoastalPistol')
clip = u.load_asset('/Game/Coastal/Character/Animations/CA_AS_Pistol_Point_Front')
pose = u.AnimPoseExtensions.get_anim_pose_at_time(clip,.99,u.AnimPoseEvaluationOptions())
hand = u.AnimPoseExtensions.get_bone_pose(pose,'hand_l',u.AnimPoseSpaces.WORLD)
desired = u.Transform(location=hand.translation+u.Vector(0,6,3),rotation=u.Rotator(yaw=90))
relative = u.MathLibrary.make_relative_transform(desired,hand)
muzzle = u.MathLibrary.transform_location(relative,u.Vector(14.581,0,4.45))
directors = u.GameplayStatics.get_all_actors_of_class(world,u.CoastalCombatDirector)
if len(directors)!=1 or not mesh:
    raise RuntimeError('Expected one director and assembled mesh')
actor = directors[0]
actor.set_editor_property('weapon_attach_socket','hand_l')
actor.set_editor_property('weapon_visual_asset',mesh)
actor.set_editor_property('weapon_visual_transform',relative)
actor.set_editor_property('muzzle_offset',muzzle)
if not u.EditorAssetLibrary.save_loaded_asset(mesh) or not level.save_current_level():
    raise RuntimeError('Presentation save failed')
baseline = json.loads((ROOT/'local-evidence/m3-activities-preservation-baseline.json').read_text())
other_changes = []
for name, expected in baseline.items():
    path = Path(name)
    if path.suffix not in ('.sav','.umap','.uproject') or path.resolve()==map_file.resolve():
        continue
    if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest()!=expected:
        other_changes.append(name)
report = dict(mesh=mesh.get_path_name(),socket=str(actor.weapon_attach_socket),
    transform=str(relative),muzzle=str(muzzle),map=str(map_file),backup=str(backup),
    before_sha256=hashlib.sha256(backup.read_bytes()).hexdigest(),
    after_sha256=hashlib.sha256(map_file.read_bytes()).hexdigest(),
    other_protected_changes=other_changes,passed=not other_changes)
(ROOT/'local-evidence/m3-pistol-binding.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report))
if other_changes:
    raise RuntimeError('Unexpected protected-file changes')
