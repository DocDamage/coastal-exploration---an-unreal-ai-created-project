"""Author swimming for the continuous First Signal water plane inside world limits."""
import hashlib
import json
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
from m3_destination_authoring import Destination

OUT = Path('F:/coastline/local-evidence')
SAVE_DIR = Path(u.Paths.project_saved_dir()) / 'SaveGames'
WATER_MATERIAL = '/Game/Coastal/M3/Materials/MI_CoveWater'
SURFACE_Z = -90.0
DEPTH = 600.0


def save_hashes():
    return {str(path.relative_to(SAVE_DIR)).replace('\\', '/'): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(SAVE_DIR.rglob('*')) if path.is_file()}


def bounds(actor):
    center, extent = actor.get_actor_bounds(False)
    return center, extent


def close(actual, expected, tolerance=5.0):
    return abs(actual - expected) <= tolerance


d = Destination('world_swimming')
actors = d.actors.get_all_level_actors()
before_saves = save_hashes()
before_ids = sorted(str(a.get_editor_property('world_id')) for a in actors
                    if isinstance(a, u.CoastalWorldObject))
required_ids = {'world.test.radio', 'world.test.storage', 'world.test.door',
                'world.test.battery', 'world.test.fuse', 'world.test.note',
                'world.test.postcard'}
if len(before_ids) != len(set(before_ids)) or not required_ids.issubset(before_ids):
    raise RuntimeError('Expected the seven required First Signal WorldObject IDs without duplicates')

water = []
for actor in actors:
    if not isinstance(actor, u.StaticMeshActor):
        continue
    materials = [m for m in actor.static_mesh_component.get_materials() if m]
    if any(m.get_path_name().split('.')[0] == WATER_MATERIAL for m in materials):
        water.append(actor)
if len(water) != 1:
    raise RuntimeError('Expected exactly one authored First Signal water plane')
water = water[0]
water_center, water_extent = bounds(water)
if (water.get_actor_label() != 'Cove and open sea'
        or not close(water_center.z + water_extent.z, SURFACE_Z)
        or water_extent.x < 49000 or water_extent.y < 49000):
    raise RuntimeError('Authored water geometry differs from the inspected continuous plane')

safety = {a.get_actor_label(): a for a in actors if isinstance(a, u.CoastalSafetyVolume)}
required = ['West boundary', 'East boundary', 'North boundary', 'Below boundary', 'Deep sea']
if any(name not in safety for name in required):
    raise RuntimeError('First Signal safety perimeter is incomplete')
west_c, west_e = bounds(safety['West boundary'])
east_c, east_e = bounds(safety['East boundary'])
north_c, north_e = bounds(safety['North boundary'])
deep_c, deep_e = bounds(safety['Deep sea'])
west = west_c.x + west_e.x
east = east_c.x - east_e.x
north = north_c.y - north_e.y
# Leave one boundary-thickness strip inside Deep sea so a southward departure
# loses its exemption while still overlapping the existing recovery hazard.
south_transition = west_e.x
south = deep_c.y - deep_e.y + south_transition
if not (close(west, -34700) and close(east, 45700) and close(north, 34700)
        and close(deep_c.y - deep_e.y, -42000) and west < east and south < north):
    raise RuntimeError('Safety perimeter differs from the measured First Signal limits')
if (west < water_center.x - water_extent.x or east > water_center.x + water_extent.x
        or south < water_center.y - water_extent.y or north > water_center.y + water_extent.y):
    raise RuntimeError('Playable perimeter extends beyond the authored water plane')

basins = [a for a in actors if isinstance(a, u.CoastalSwimmingZone)
          and 'Coastal.Expansion.sheltered_swimming.Sheltered basin' in [str(t) for t in a.tags]]
if len(basins) != 1:
    raise RuntimeError('Expected the higher-priority sheltered swimming zone')
basin = basins[0]
basin.set_editor_property('water_surface_z', SURFACE_Z)
basin.set_editor_property('max_recovery_exemption_depth_cm', 220.0)
basin.set_editor_property('water_volume', True)
basin.set_editor_property('physics_on_contact', True)
basin.set_editor_property('priority', 10)
if not u.CoastalVendorInspection.build_swimming_zone_box(basin, u.Vector(1200, 3700, 360)):
    raise RuntimeError('Sheltered swimming brush collision could not be hardened')

size = (east - west, north - south, DEPTH)
center = ((west + east) / 2, (south + north) / 2, SURFACE_Z - DEPTH / 2)
zone = d.actor(u.CoastalSwimmingZone, 'First Signal coastal waters', center)
zone.set_editor_property('water_surface_z', SURFACE_Z)
zone.set_editor_property('max_recovery_exemption_depth_cm', 220.0)
zone.set_editor_property('water_volume', True)
zone.set_editor_property('physics_on_contact', True)
zone.set_editor_property('priority', 5)
if not u.CoastalVendorInspection.build_swimming_zone_box(zone, u.Vector(*size)):
    raise RuntimeError('Native world-water brush creation or validation failed')
zone_center, zone_extent = bounds(zone)
brush = zone.brush_component
if (not zone.is_authored_correctly()
        or str(brush.get_collision_profile_name()) != 'OverlapAllDynamic'
        or brush.get_collision_enabled() != u.CollisionEnabled.QUERY_ONLY
        or not brush.get_editor_property('generate_overlap_events')
        or brush.get_collision_response_to_channel(u.CollisionChannel.ECC_PAWN) != u.CollisionResponseType.ECR_OVERLAP
        or any(not close(a, b) for a, b in zip(
        zone_center.to_tuple(), center)) or any(not close(a * 2, b) for a, b in zip(
            zone_extent.to_tuple(), size))):
    raise RuntimeError('World-water brush bounds did not read back exactly')

# Record two low-shore probes for live walking-to-swimming validation. The dry
# point has terrain above the water; the wet point has terrain below it.
world = d.editor.get_editor_world()
shore = []
for x in range(-1000, 5001, 250):
    for y in range(-6000, 1001, 250):
        hit = u.SystemLibrary.line_trace_single(world, u.Vector(x, y, 1000),
            u.Vector(x, y, -1000), u.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
            u.DrawDebugTrace.NONE, True)
        if not hit:
            continue
        data = hit.to_tuple()
        actor = data[9]
        if actor and actor.get_actor_label() == 'Coastal headland and walking trail' and data[7].z > .9:
            shore.append((x, y, data[4].z))
dry = min((p for p in shore if -80 <= p[2] <= 40), key=lambda p: abs(p[2] + 45), default=None)
wet = min((p for p in shore if -260 <= p[2] <= -190), key=lambda p: abs(p[2] + 220), default=None)
if not dry or not wet:
    raise RuntimeError('Could not resolve inspected low-shore probes around the cove')

rows = d.save()
after_ids = sorted(str(a.get_editor_property('world_id')) for a in d.actors.get_all_level_actors()
                   if isinstance(a, u.CoastalWorldObject))
after_saves = save_hashes()
if after_ids != before_ids:
    raise RuntimeError('WorldObject IDs changed during all-water authoring')
if after_saves != before_saves:
    raise RuntimeError('Campaign save files changed during all-water authoring')

report = {
    'passed': True,
    'authored_water': {'actor': water.get_path_name(), 'label': water.get_actor_label(),
                       'center': list(water_center.to_tuple()), 'extent': list(water_extent.to_tuple()),
                       'surface_z': SURFACE_Z, 'material': WATER_MATERIAL},
    'playable_swimming_coverage': {'center': center, 'size': size, 'priority': 5,
        'bounds': {'west': west, 'east': east, 'south': south, 'north': north,
                   'top': SURFACE_Z, 'bottom': SURFACE_Z - DEPTH}},
    'retained_sheltered_zone': basin.get_path_name(),
    'required_brush_collision': {'profile': 'OverlapAllDynamic', 'mode': 'QueryOnly',
                                 'generate_overlap_events': True, 'pawn_response': 'Overlap'},
    'south_recovery_transition_cm': south_transition,
    'low_shore_probes': {'dry_floor': dry, 'submerged_floor': wet},
    'excluded_water': {
        'policy': 'Water beyond the safety perimeter remains visible scenery and retains boundary/fall recovery.',
        'west_scenery_cm': west - (water_center.x - water_extent.x),
        'east_scenery_cm': (water_center.x + water_extent.x) - east,
        'north_scenery_cm': (water_center.y + water_extent.y) - north,
        'south_scenery_and_transition_cm': south - (water_center.y - water_extent.y)},
    'future_content_policy': ('Any later underwater ruins placed beneath this authored water plane and inside '
                              'the measured perimeter inherit swimming; water outside it requires an explicit '
                              'new bounded volume and safety review.'),
    'zone': rows, 'world_ids': after_ids, 'save_file_count': len(after_saves),
    'scope': 'Saved editor configuration; scripted PIE and package acceptance are separate.'}
(OUT / 'm3-all-waters-configuration.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report))
