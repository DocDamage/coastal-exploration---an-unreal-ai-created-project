"""Place one presentation-only merchant beside the authored village register."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
EXPECTED = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != EXPECTED.resolve():
    raise RuntimeError('Expected independent LocalHost58 project')
if not u.SystemLibrary.get_engine_version().startswith('5.8.'):
    raise RuntimeError('Expected Unreal 5.8')
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if level.is_in_play_in_editor():
    raise RuntimeError('Stop PIE before authoring')
if u.EditorLoadingAndSavingUtils.get_dirty_map_packages() or u.EditorLoadingAndSavingUtils.get_dirty_content_packages():
    raise RuntimeError('Preserve existing unsaved work before authoring')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
registers = [a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalWorldObject)
             if str(a.world_id) == 'world.coastal_records.village']
existing = u.GameplayStatics.get_all_actors_of_class(world, u.CoastalMerchantPresenter)
if len(registers) != 1 or existing:
    raise RuntimeError('Expected one village register and no existing merchant presenter')
register = registers[0]
if (register.get_actor_location() - u.Vector(-1455, 15300, 2305)).length() > 100:
    raise RuntimeError('Village register moved; choose merchant placement again')

xy = u.Vector(-1455, 15630, 2600)
hit = u.SystemLibrary.line_trace_single(world, xy, u.Vector(xy.x, xy.y, 1800),
    u.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], u.DrawDebugTrace.NONE, True)
if not hit or hit.to_tuple()[7].z < .7:
    raise RuntimeError('Merchant placement has no walkable downward Visibility hit')
floor = hit.to_tuple()[4]
if (floor - u.Vector(-1600, 15300, 2200)).length() < 250:
    raise RuntimeError('Merchant would obstruct the village arrival')
actor = u.EditorLevelLibrary.spawn_actor_from_class(
    u.CoastalMerchantPresenter, floor, u.Rotator(pitch=0, yaw=-35, roll=0))
if not actor:
    raise RuntimeError('Could not spawn merchant presenter')
actor.set_actor_label('Coastal Village Merchant Presentation')
actor.tags = ['Coastal.Merchant', 'Coastal.Village']
if not u.EditorLoadingAndSavingUtils.save_current_level():
    raise RuntimeError('Failed to save merchant placement')
report = {'authored': True, 'actor': actor.get_path_name(),
          'location': list(actor.get_actor_location().to_tuple()),
          'floor_hit': list(floor.to_tuple()), 'register_distance_cm': (floor-register.get_actor_location()).length(),
          'arrival_distance_cm': (floor-u.Vector(-1600, 15300, 2200)).length(),
          'scope': 'One non-colliding presentation actor; existing village register remains authoritative'}
(ROOT / 'local-evidence/m3-merchant-authoring.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
