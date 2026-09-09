"""Author the nine journal-backed investigation records beside verified dry arrivals."""
import json
import shutil
import sys
from pathlib import Path
import unreal as u

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
from m3_destination_authoring import Destination

PROJECT = Path(u.Paths.get_project_file_path()).resolve().parent
WORKSPACE = Path(__file__).resolve().parents[3]
ALLOWED = {(WORKSPACE / 'LocalHost58/CoastalExploration').resolve()}
if PROJECT not in ALLOWED:
    raise RuntimeError('Destination quest authoring is restricted to an explicit local host')
HOST_KEY = 'ue58' if PROJECT.name == 'CoastalExploration' and PROJECT.parent.name == 'LocalHost58' else 'ue57'
EVIDENCE = WORKSPACE / 'local-evidence'
backup = EVIDENCE / f'm3-destination-quests-before-L_FirstSignal-{HOST_KEY}.umap'
if not backup.exists():
    shutil.copy2(PROJECT / 'Content/Coastal/Maps/L_FirstSignal.umap', backup)

d = Destination('destination_quests')
world_class = u.EditorAssetLibrary.load_blueprint_class(
    '/Game/Coastal/Integration/BP_CoastalHyperWorldObject')
if not world_class:
    raise RuntimeError('Expected the existing Hyper-bound Coastal world-object class')
cube = u.load_asset('/Engine/BasicShapes/Cube')
material = u.load_asset('/Game/Coastal/M2/Materials/M_RadioCase')
if not isinstance(cube, u.StaticMesh) or material is None:
    raise RuntimeError('Required project-owned investigation presentation is unavailable')

# Object locations are within 150 cm of dry capsule-centre arrivals already checked
# in PIE. The plaques remain off the centre line so they do not obstruct recovery.
records = [
    ('Harbour inspection log', 'world.north_reach.harbour_log',
     'journal.north_reach.harbour_log', (28200, 23145, 455), (28200, 23000, 448),
     'READ: HARBOUR INSPECTION LOG'),
    ('Platform signal station', 'world.north_reach.platform_signal',
     'journal.north_reach.platform_signal', (38745, -14000, 905), (38600, -14000, 898),
     'CHECK: SIGNAL STATION'),
    ('Powell work order', 'world.north_reach.powell_order',
     'journal.north_reach.powell_order', (25645, 4000, 405), (25500, 4000, 398),
     'READ: POWELL WORK ORDER'),
    ('Hallsands evacuation marker', 'world.coastal_records.hallsands',
     'journal.coastal_records.hallsands', (-6355, 22000, 200), (-6500, 22000, 192),
     'INSPECT: EVACUATION MARKER'),
    ('Village register', 'world.coastal_records.village',
     'journal.coastal_records.village', (-1455, 15300, 2305), (-1600, 15300, 2298),
     'READ: VILLAGE REGISTER'),
    ('Baelo survey tablet', 'world.coastal_records.baelo',
     'journal.coastal_records.baelo', (8145, 20000, 535), (8000, 20000, 526),
     'INSPECT: SURVEY TABLET'),
    ('Prison duty record', 'world.coastal_records.prison',
     'journal.coastal_records.prison', (21455, 29000, 505), (21600, 29000, 498),
     'READ: PRISON DUTY RECORD'),
    ('Atlantis tide survey', 'world.outer_coast.atlantis',
     'journal.outer_coast.atlantis', (-15855, 6000, 305), (-16000, 6000, 298),
     'INSPECT: TIDE SURVEY'),
    ('Station monitoring log', 'world.outer_coast.station',
     'journal.outer_coast.station', (33145, -28500, 705), (33000, -28500, 698),
     'READ: STATION MONITORING LOG'),
]
required = {'world.test.radio', 'world.test.storage', 'world.test.door',
            'world.test.battery', 'world.test.fuse', 'world.test.note',
            'world.test.postcard'}
before_ids = {str(a.world_id) for a in d.actors.get_all_level_actors()
              if isinstance(a, u.CoastalWorldObject)}
if not required.issubset(before_ids):
    raise RuntimeError('Required original First Signal records are missing before authoring')

written = []
for key, world_id, journal_id, location, arrival, sign_text in records:
    tag = d.prefix + key
    actor = d.known.get(tag)
    if actor and not isinstance(actor, u.CoastalWorldObject):
        raise RuntimeError('Authored quest tag belongs to another actor: ' + tag)
    if not actor:
        actor = d.actors.spawn_actor_from_class(world_class, u.Vector(*location))
        if not actor:
            raise RuntimeError('Could not spawn destination record: ' + world_id)
        actor.tags = [u.Name(tag)]
        d.known[tag] = actor
    actor.set_actor_location_and_rotation(u.Vector(*location), u.Rotator(yaw=-90), False, True)
    actor.set_actor_label(key)
    actor.set_folder_path('Coastal Expansion/Destination Quests')
    actor.set_editor_property('world_id', u.Name(world_id))
    actor.set_editor_property('kind', u.CoastalObjectKind.DISCOVERY)
    actor.set_editor_property('journal_entry', u.Name(journal_id))
    actor.set_editor_property('display_label', key)
    actor.set_editor_property('visual_mesh', cube)
    actor.set_editor_property('visual_transform', u.Transform(scale=u.Vector(.55, .08, .35)))
    actor.set_editor_property('proxy_size_cm', u.Vector(55, 8, 35))
    actor.set_editor_property('pickup_item', u.CoastalItemRequirement())
    actor.set_editor_property('persistent_pickup_container', False)
    actor.refresh_development_proxy()
    proxy = actor.get_editor_property('proxy_mesh')
    proxy.set_material(0, material)
    proxy.set_collision_profile_name('BlockAll')
    d.written.append(actor)
    outer = world_id.startswith('world.outer_coast.')
    prompt = d.sign(key + ' prompt', sign_text, (location[0], location[1], location[2] + 85),
                    yaw=180 if outer else -90)
    if outer:
        prompt.get_component_by_class(u.TextRenderComponent).set_world_size(18)
    written.append({'world_id': world_id, 'journal_id': journal_id,
                    'actor': actor.get_path_name(), 'location': list(location),
                    'dry_arrival': list(arrival),
                    'distance_from_arrival_cm': (u.Vector(*location) - u.Vector(*arrival)).length()})

all_ids = [str(a.world_id) for a in d.actors.get_all_level_actors()
           if isinstance(a, u.CoastalWorldObject)]
expected = required | {row[1] for row in records}
if len(all_ids) != len(set(all_ids)) or set(all_ids) != expected:
    raise RuntimeError('First Signal world-record manifest is not the expected 7 required + 9 destination records')
d.save()
report = {'host': str(PROJECT), 'engine_target': HOST_KEY, 'backup': str(backup),
          'required_original_ids': sorted(required), 'optional_destination_records': written,
          'world_record_count': len(all_ids), 'runtime_verified': False}
(EVIDENCE / f'm3-destination-quests-authoring-{HOST_KEY}.json').write_text(
    json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps({'authored': True, 'host': HOST_KEY, 'world_records': len(all_ids)}))
