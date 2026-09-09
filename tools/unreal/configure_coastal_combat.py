"""Author one bounded dry-land combat encounter after the UE5.8 combat build."""
import hashlib
import json
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
from m3_destination_authoring import Destination

OUT = (Path(__file__).resolve().parents[3] / 'local-evidence/m3-coastal-combat-authoring.json')
SAVE_DIR = Path(u.Paths.project_saved_dir()) / 'SaveGames'


def hashes():
    return {str(p.relative_to(SAVE_DIR)).replace('\\', '/'): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(SAVE_DIR.rglob('*')) if p.is_file()}


def floor(d, x, y):
    world = d.editor.get_editor_world()
    hit = u.SystemLibrary.line_trace_single(world, u.Vector(x, y, 3000), u.Vector(x, y, -1000),
        u.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], u.DrawDebugTrace.NONE, True)
    if not hit or hit.to_tuple()[7].z < .7:
        raise RuntimeError('No verified walkable floor at {}, {}'.format(x, y))
    return hit.to_tuple()[4].z


if not str(u.SystemLibrary.get_engine_version()).startswith('5.8.'):
    raise RuntimeError('Coastal combat authoring is UE5.8-only')
d = Destination('coastal_combat')
before_saves = hashes()
before_ids = sorted(str(a.world_id) for a in d.actors.get_all_level_actors()
                    if isinstance(a, u.CoastalWorldObject))
cube = u.load_asset('/Engine/BasicShapes/Cube.Cube')
cylinder = u.load_asset('/Engine/BasicShapes/Cylinder.Cylinder')
cone = u.load_asset('/Engine/BasicShapes/Cone.Cone')
if not cube or not cylinder or not cone:
    raise RuntimeError('Required rendered project/engine meshes are missing')

# Industrial Harbour is an existing verified dry destination. Floor traces keep
# encounter actors on its authored collision rather than trusting catalogue bounds.
points = {'player': (29200, 23000), 'clear': (30200, 23000),
          'blocked': (30200, 23800), 'sentry': (30800, 22300),
          'sentry_player': (30000, 22300)}
z = {name: floor(d, *xy) for name, xy in points.items()}
if max(z.values()) - min(z.values()) > 500:
    raise RuntimeError('Selected harbour encounter floor is not a coherent dry platform')

director = d.actor(u.CoastalCombatDirector, 'Harbour combat director',
                   (30000, 23000, max(z.values()) + 100))
director.set_editor_property('weapon_class', u.CoastalSidearmWeapon)
director.set_editor_property('weapon_attach_socket', 'hand_r')
director.set_editor_property('weapon_visual_asset', cube)
director.set_editor_property('tracer_asset', cylinder)
director.set_editor_property('max_health', 100.0)
director.set_editor_property('max_shield', 50.0)
director.set_editor_property('show_combat_hud', True)

volume = d.actor(u.CoastalCombatEncounterVolume, 'Harbour combat boundary',
                 (30000, 23000, min(z.values()) + 250))
volume.bounds.set_box_extent(u.Vector(1800, 1400, 450), True)
if not volume.is_authored_correctly():
    raise RuntimeError('Combat encounter volume collision is invalid')

clear = d.actor(u.CoastalCombatTarget, 'Clear training target',
                (*points['clear'], z['clear'] + 75))
clear.set_editor_property('presentation_asset', cube)
clear.set_actor_scale3d(u.Vector(.65, .65, 1.5))
blocked = d.actor(u.CoastalCombatTarget, 'Occluded training target',
                  (*points['blocked'], z['blocked'] + 75))
blocked.set_editor_property('presentation_asset', cube)
blocked.set_actor_scale3d(u.Vector(.65, .65, 1.5))
sentry = d.actor(u.CoastalCombatSentry, 'Harbour sentry',
                 (*points['sentry'], z['sentry'] + 75))
sentry.set_editor_property('presentation_asset', cone)
sentry.set_editor_property('tracer_asset', cylinder)
sentry.set_editor_property('attack_interval_seconds', 2.0)
sentry.set_editor_property('attack_damage', 8.0)
sentry.set_editor_property('engagement_range_cm', 1400.0)
sentry.set_editor_property('encounter_radius_cm', 1800.0)
sentry.set_editor_property('require_recently_rendered', True)

# This saved collision wall provides a deterministic LOS rejection case.
wall_z = max(z['player'], z['blocked']) + 110
wall = d.mesh('Combat LOS wall', '/Engine/BasicShapes/Cube', (29700, 23400, wall_z),
              scale=(.25, 2.5, 2.2), collision=True)
rows = d.save()
after_ids = sorted(str(a.world_id) for a in d.actors.get_all_level_actors()
                   if isinstance(a, u.CoastalWorldObject))
if after_ids != before_ids or hashes() != before_saves:
    raise RuntimeError('Combat authoring changed saved WorldObject IDs or campaign files')
report = {
    'passed': True, 'engine': u.SystemLibrary.get_engine_version(),
    'destination': 'industrial_harbour', 'test_player_location': [*points['player'], z['player'] + 98],
    'sentry_test_location': [*points['sentry_player'], z['sentry_player'] + 98],
    'floor_z': z, 'actors': rows, 'world_object_ids': after_ids,
    'assets': {'weapon': cube.get_path_name(), 'tracer': cylinder.get_path_name(),
               'sentry': cone.get_path_name()},
    'policy': ('Transient encounter ammo/health only; no AGIS items, save schema, offscreen attacks, '
               'or combat outside the authored overlap volume.'),
    'runtime_verified': False,
}
OUT.write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report))
