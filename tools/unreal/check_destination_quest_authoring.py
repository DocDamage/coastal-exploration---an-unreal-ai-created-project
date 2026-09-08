"""Read back the fixed destination manifest and its reachable dry-arrival placement."""
import json
from pathlib import Path
import unreal as u

PROJECT = Path(u.Paths.get_project_file_path()).resolve().parent
ALLOWED = {Path('F:/coastline/LocalHost/CoastalExploration').resolve(),
           Path('F:/coastline/LocalHost58/CoastalExploration').resolve()}
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if PROJECT not in ALLOWED or not world or world.get_name() != 'L_FirstSignal' or level.is_in_play_in_editor():
    raise RuntimeError('Expected an approved First Signal editor host outside PIE')
host_key = 'ue58' if PROJECT.parent.name == 'LocalHost58' else 'ue57'
spec = json.loads(Path('F:/coastline/CoastalExploration/data/m3_destination_quests.json').read_text(encoding='utf-8'))
expected = {step['world_id']: step for quest in spec['quests'] for step in quest['steps']}
required = {'world.test.radio', 'world.test.storage', 'world.test.door',
            'world.test.battery', 'world.test.fuse', 'world.test.note', 'world.test.postcard'}
arrivals = {
    'world.north_reach.harbour_log': (28200, 23000, 448),
    'world.north_reach.platform_signal': (38600, -14000, 898),
    'world.north_reach.powell_order': (25500, 4000, 398),
    'world.coastal_records.hallsands': (-6500, 22000, 192),
    'world.coastal_records.village': (-1600, 15300, 2298),
    'world.coastal_records.baelo': (8000, 20000, 526),
    'world.coastal_records.prison': (21600, 29000, 498),
}
objects = [a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
           if isinstance(a, u.CoastalWorldObject)]
ids = [str(a.world_id) for a in objects]
if len(ids) != 14 or len(set(ids)) != 14 or set(ids) != required | set(expected):
    raise RuntimeError('Expected exactly seven required and seven approved optional world records')
rows = []
for actor in objects:
    world_id = str(actor.world_id)
    if world_id not in expected:
        continue
    row = expected[world_id]
    if (actor.get_editor_property('kind') != u.CoastalObjectKind.DISCOVERY
            or str(actor.get_editor_property('journal_entry')) != row['journal_id']):
        raise RuntimeError('Destination record binding mismatch: ' + world_id)
    visual = actor.get_editor_property('visual_mesh')
    proxy = actor.get_editor_property('proxy_mesh')
    if not visual or not proxy or proxy.get_collision_enabled() == u.CollisionEnabled.NO_COLLISION:
        raise RuntimeError('Destination record lacks a visible collidable interaction prop: ' + world_id)
    distance = (actor.get_actor_location() - u.Vector(*arrivals[world_id])).length()
    if distance > 180:
        raise RuntimeError('Destination record is outside interaction reach of its verified dry arrival: ' + world_id)
    if actor.is_active():
        raise RuntimeError('Editor-authored destination record must be pristine: ' + world_id)
    rows.append({'world_id': world_id, 'journal_id': str(actor.get_editor_property('journal_entry')),
                 'actor': actor.get_path_name(), 'class': actor.get_class().get_path_name(),
                 'location': list(actor.get_actor_location().to_tuple()), 'arrival_distance_cm': distance})

report = {'passed': True, 'host': str(PROJECT), 'engine_target': host_key,
          'required_records': sorted(required), 'destination_records': sorted(rows, key=lambda x: x['world_id'])}
out = Path('F:/coastline/local-evidence') / f'm3-destination-quests-authoring-check-{host_key}.json'
out.write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps({'passed': True, 'world_records': len(ids), 'destination_records': len(rows)}))
