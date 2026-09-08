"""Author the sheltered dock-basin approach; swimming zone is configured separately."""
import json
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
from m3_destination_authoring import Destination

OUT = Path('F:/coastline/local-evidence')
d = Destination('sheltered_swimming')
wood = '/Game/Coastal/M2/Materials/M_DockTimber'
before_ids = sorted(str(a.get_editor_property('world_id'))
                    for a in d.actors.get_all_level_actors() if isinstance(a, u.CoastalWorldObject))
# Open the single 300 cm west handrail segment between existing piles. The first
# native traversal exposed this blocker; retain the actor for reversible repair.
access_tag = 'Coastal.Swimming.AccessHandrail'
rails = [a for a in d.actors.get_all_level_actors()
         if (access_tag in [str(t) for t in a.tags]
             or (a.get_actor_label() == 'Pier handrail'
                 and (a.get_actor_location()-u.Vector(12335, -55, 337)).length() < 1))]
if len(rails) != 1 or not isinstance(rails[0], u.StaticMeshActor):
    raise RuntimeError('Expected the inspected west pier access handrail')
rail = rails[0]
rail.tags = list(rail.tags) + ([] if access_tag in [str(t) for t in rail.tags] else [u.Name(access_tag)])
rail.set_actor_hidden_in_game(True)
rail.static_mesh_component.set_collision_profile_name('NoCollision')
rail.set_actor_label('Pier handrail — swimming access opening (hidden)')
rock_moves = []
for rock in d.actors.get_all_level_actors():
    if not isinstance(rock, u.StaticMeshActor) or rock.get_actor_label() != 'Weathered coastal rock':
        continue
    center, extent = rock.get_actor_bounds(False)
    if not (center.x+extent.x > 11480 and center.x-extent.x < 12020
            and center.y+extent.y > -2600 and center.y-extent.y < 200):
        continue
    lowest_ramp_z = 250 + max(-2400, min(0, center.y-extent.y)) * 500 / 2400
    if center.z+extent.z < lowest_ramp_z-10:
        continue
    before = rock.get_actor_location()
    delta_x = 11000-(center.x+extent.x)
    rock.set_actor_location(before+u.Vector(delta_x, 0, 0), False, True)
    rock_moves.append({'actor': rock.get_path_name(), 'before': list(before.to_tuple()),
                       'after': list(rock.get_actor_location().to_tuple())})
# The old pier remains the approach. Both new sections sit over the existing
# basin, avoiding changes to original terrain, deck colliders or save objects.
d.path('Pier connection', [(12500, 0, 250), (11750, 0, 250)], wood, width=260)
d.path('Walk-out ramp', [(11750, 0, 250), (11750, -2400, -250)], wood, width=400)
d.box('Ramp landing', (11750, 0, 238), (440, 240, 24), wood)
d.sign('Swimming access', 'SHELTERED SWIMMING\nWalk down the ramp to enter\nReturn up the ramp to leave',
       (12100, 210, 410), yaw=-90)
# Floating markers describe the authored safe region without obstructing it.
for index, (x, y) in enumerate([(11150, -400), (11150, -1200), (11150, -2000),
                              (11150, -2800), (11150, -3400), (11750, -3400),
                              (12250, -3400), (12250, -2600)]):
    d.mesh('Swim marker ' + str(index), '/Engine/BasicShapes/Sphere',
           (x, y, -65), scale=(.65, .65, .65), collision=False,
           materials=['/Game/Coastal/M2/Materials/M_RadioDial'])
after_ids = sorted(str(a.get_editor_property('world_id'))
                   for a in d.actors.get_all_level_actors() if isinstance(a, u.CoastalWorldObject))
if before_ids != after_ids:
    raise RuntimeError('WorldObject IDs changed during swim approach authoring')
report = {'actors': d.save(), 'water_surface_z': -90,
          'opened_handrail': rail.get_path_name(),
          'shoreline_rocks_moved_beside_basin': rock_moves,
          'ramp': [[11750, 0, 250], [11750, -2400, -250]],
          'dry_start': [11750, 0, 348], 'world_ids': after_ids,
          'swimming_zone': 'configured separately after native checkpoint build'}
(OUT / 'm3-swimming-approach.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
