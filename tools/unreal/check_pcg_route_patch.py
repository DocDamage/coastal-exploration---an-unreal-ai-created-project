"""Exercise PCGEx generation, cleanup and deterministic regeneration in PIE."""
import json
import time
from pathlib import Path
import unreal as u

OUT = Path('F:/coastline/local-evidence/m3-pcg-route-live.json')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
patches = u.GameplayStatics.get_all_actors_with_tag(world, 'Coastal.PCG.NorthTrailPatch')
saves = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
         if 'UEDPIE_' in s.get_path_name() and s.has_active_campaign()]
if len(patches) != 1 or len(saves) != 1 or not str(saves[0].get_active_save_set()).startswith('coastal_test_'):
    raise RuntimeError('Requires one authored patch in an active disposable PIE campaign')
patch = patches[0]
pcg = patch.get_component_by_class(u.PCGComponent)
policy = pcg.scheduling_policy
if not isinstance(policy, u.PCGExSchedulingPolicy) or len(policy.constraints) != 1:
    raise RuntimeError('Missing actual PCGEx sphere constraint')
pawn = u.GameplayStatics.get_player_character(world, 0)
movement = pawn.get_component_by_class(u.CharacterMovementComponent)
near, far = u.Vector(8200, 12000, 1518), u.Vector(0, 0, 400)
for location in (near, far):
    if not u.CoastalPlacementLibrary.is_dry_destination(pawn, u.Transform(location=location)):
        raise RuntimeError('Generation probe is not a valid dry destination')
def place(location):
    movement.stop_movement_immediately()
    pawn.consume_movement_input_vector()
    pawn.set_actor_location(location, False, True)
def instances():
    components = patch.get_components_by_class(u.InstancedStaticMeshComponent)
    result = []
    for component in components:
        if component.get_collision_enabled() != u.CollisionEnabled.NO_COLLISION:
            raise RuntimeError('Generated grass unexpectedly blocks the route')
        for index in range(component.get_instance_count()):
            transform = component.get_instance_transform(index, True)
            result.append([round(v, 4) for v in (*transform.translation.to_tuple(),
                          *transform.rotation.to_tuple(), *transform.scale3d.to_tuple())])
    return sorted(result)
rows = []
phase = 0
original = []
began = time.monotonic()
deadline = began + 25
done = False
place(near)
def finish(error=None):
    global done
    if done:
        return
    done = True
    u.unregister_slate_post_tick_callback(handle)
    OUT.write_text(json.dumps({'passed': error is None, 'error': error, 'rows': rows,
                              'elapsed_seconds': time.monotonic() - began,
                              'policy': policy.get_class().get_name()}, indent=2), encoding='utf-8')
def tick(delta):
    global phase, deadline, original
    try:
        now = time.monotonic()
        if now > deadline:
            raise RuntimeError('Runtime PCG phase timed out: ' + str(phase) + ', instances=' + str(len(instances())))
        current = instances()
        if phase == 0 and len(current) == 6:
            original = current
            rows.append({'case': 'nearby_runtime_generation', 'instance_count': 6})
            place(far); phase, deadline = 1, now + 25
        elif phase == 1 and not current:
            rows.append({'case': 'distance_cleanup', 'instance_count': 0})
            place(near); phase, deadline = 2, now + 25
        elif phase == 2 and len(current) == 6:
            if current != original:
                raise RuntimeError('Regeneration changed instance placement')
            rows.append({'case': 'deterministic_regeneration', 'instance_count': 6,
                         'transforms': current})
            finish()
    except Exception as exc:
        finish(str(exc))
OUT.write_text(json.dumps({'passed': False, 'status': 'running'}), encoding='utf-8')
handle = u.register_slate_post_tick_callback(tick)
print('Runtime PCGEx generation/cleanup check running')
