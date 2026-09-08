"""Author one bounded, deterministic route patch using real PCGEx scheduling.

Run outside PIE in the independent UE5.8 trial. Existing grass remains until
the separate live check proves runtime generation and cleanup.
"""
import json
from pathlib import Path
import unreal as u

HOST = Path('F:/coastline/LocalHost58/CoastalExploration').resolve()
OUT = Path('F:/coastline/local-evidence/m3-pcg-route-authoring.json')
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.EditorActorSubsystem)
if (Path(u.Paths.get_project_file_path()).resolve().parent != HOST
        or level.is_in_play_in_editor()):
    raise RuntimeError('Only author in the isolated UE5.8 editor outside PIE')
actors = editor.get_all_level_actors()
prefix = 'Coastal.Expansion.route_dressing.north_trail.'
grass = sorted([a for a in actors if isinstance(a, u.StaticMeshActor)
                and any(str(t).startswith(prefix) and str(t).endswith('.grass') for t in a.tags)],
               key=lambda a: a.get_actor_label())[:6]
if len(grass) != 6:
    raise RuntimeError('Expected six existing north-trail grass placements')
mesh = grass[0].static_mesh_component.static_mesh
if any(a.static_mesh_component.static_mesh != mesh for a in grass):
    raise RuntimeError('Patch must use one instanced grass mesh')
transforms = [a.get_actor_transform() for a in grass]
positions = [t.translation for t in transforms]
minimum = [min(getattr(p, axis) for p in positions) - 250 for axis in ('x', 'y', 'z')]
maximum = [max(getattr(p, axis) for p in positions) + 250 for axis in ('x', 'y', 'z')]
center = u.Vector(*[(lo + hi) / 2 for lo, hi in zip(minimum, maximum)])
extent = u.Vector(*[(hi - lo) / 2 for lo, hi in zip(minimum, maximum)])
assets = u.AssetToolsHelpers.get_asset_tools()
folder = '/Game/Coastal/Expansion/Procedural'
graph_path = folder + '/PCG_NorthTrailGrass'
graph = u.load_asset(graph_path)
if not graph:
    graph = assets.create_asset('PCG_NorthTrailGrass', folder, u.PCGGraph, u.PCGGraphFactory())
if not isinstance(graph, u.PCGGraph):
    raise RuntimeError('Unexpected graph asset class')
for old_node in list(graph.nodes):
    graph.remove_node(old_node)
point_node, points = graph.add_node_of_type(u.PCGCreatePointsSettings)
points.coordinate_space = u.PCGCoordinateSpace.WORLD
points.cull_points_outside_volume = True
points.points_to_create = [u.PCGPoint(transform=t, density=1, seed=908260 + i)
                          for i, t in enumerate(transforms)]
mesh_node, spawner = graph.add_node_of_type(u.PCGStaticMeshSpawnerSettings)
spawner.set_mesh_selector_type(u.PCGMeshSelectorWeighted)
descriptor = u.PCGSoftISMComponentDescriptor()
for key, value in {'static_mesh': mesh, 'component_class': u.InstancedStaticMeshComponent,
                   'component_tags': [u.Name('Coastal.PCG.NorthTrailGrass')],
                   'can_ever_affect_navigation': False, 'generate_overlap_events': False,
                   'use_default_collision': False}.items():
    descriptor.set_editor_property(key, value)
body = descriptor.get_editor_property('body_instance')
body.set_editor_property('collision_profile_name', 'NoCollision')
body.set_editor_property('collision_enabled', u.CollisionEnabled.NO_COLLISION)
descriptor.set_editor_property('body_instance', body)
entry = u.PCGMeshSelectorWeightedEntry(weight=1)
entry.set_editor_property('descriptor', descriptor)
spawner.mesh_selector_parameters.mesh_entries = [entry]
graph.add_edge(point_node, 'Out', mesh_node, 'In')
graph.add_edge(mesh_node, 'Out', graph.get_output_node(), 'Out')

bp_path = folder + '/BP_NorthTrailGrassPatch'
bp = u.load_asset(bp_path)
if not bp:
    factory = u.BlueprintFactory()
    factory.set_editor_property('parent_class', u.Actor)
    bp = assets.create_asset('BP_NorthTrailGrassPatch', folder, u.Blueprint, factory)
subsystem = u.get_engine_subsystem(u.SubobjectDataSubsystem)
library = u.SubobjectDataBlueprintFunctionLibrary
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)

def component(cls, name):
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        obj = library.get_object_for_blueprint(library.get_data(handle), bp)
        if isinstance(obj, cls):
            return obj
    handle, reason = subsystem.add_new_subobject(u.AddNewSubobjectParams(
        parent_handle=handles[0], new_class=cls, blueprint_context=bp))
    obj = library.get_object_for_blueprint(library.get_data(handle), bp)
    if not obj:
        raise RuntimeError('Could not add ' + name + ': ' + str(reason))
    subsystem.rename_subobject(handle, name)
    return obj

box = component(u.BoxComponent, 'PatchBounds')
box.set_box_extent(extent, False)
box.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
box.set_hidden_in_game(True)
pcg = component(u.PCGComponent, 'RouteGrassPCG')
pcg.set_graph(graph)
pcg.set_editor_property('generation_trigger', u.PCGComponentGenerationTrigger.GENERATE_AT_RUNTIME)
pcg.set_editor_property('seed', 90826)
pcg.set_editor_property('is_component_partitioned', False)
pcg.set_editor_property('override_generation_radii', True)
radii = pcg.generation_radii
radii.radius_unbounded = u.PerQualityLevelFloat(default=4500)
radii.cleanup_radius_scalar = u.PerQualityLevelFloat(default=1.3)
pcg.generation_radii = radii
pcg.set_editor_property('scheduling_policy_class', u.PCGExSchedulingPolicy)
policy = pcg.get_editor_property('scheduling_policy')
if not isinstance(policy, u.PCGExSchedulingPolicy):
    policy = u.new_object(u.PCGExSchedulingPolicy, outer=pcg)
    pcg.set_editor_property('scheduling_policy', policy)
policy.filter_by_channels = False
# Author the instanced constraint on the level instance below. Python-created
# nested Blueprint subobjects lack the compiler's archetype flags and cannot
# safely serve as private cross-package archetypes for a placed actor.
policy.set_editor_property('constraints', [])
u.BlueprintEditorLibrary.compile_blueprint(bp)
for asset in (graph, bp):
    if not u.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError('Could not save procedural asset')
existing = [a for a in actors if 'Coastal.PCG.NorthTrailPatch' in [str(t) for t in a.tags]]
if len(existing) > 1:
    raise RuntimeError('Duplicate procedural patch actors')
if existing:
    editor.destroy_actor(existing[0])
actor = editor.spawn_actor_from_class(bp.generated_class(), center)
actor.set_actor_location(center, False, True)
actor.tags = [u.Name('Coastal.PCG.NorthTrailPatch')]
actor.set_actor_label('North Trail — scheduled grass')
actor.set_folder_path('Coastal Expansion/Procedural')
instance_pcg = actor.get_component_by_class(u.PCGComponent)
instance_policy = instance_pcg.get_editor_property('scheduling_policy')
if not isinstance(instance_policy, u.PCGExSchedulingPolicy):
    raise RuntimeError('Placed patch did not inherit the PCGEx scheduling policy')
sphere = u.new_object(u.PCGExSchedulingConstraintSphere, outer=instance_policy)
sphere.radius = 4000
sphere.cleanup_scale = 1.3
instance_policy.set_editor_property('constraints', [sphere])
if not level.save_current_level():
    raise RuntimeError('Could not save procedural map authoring')
OUT.write_text(json.dumps({
    'authored': True, 'runtime_verified': False, 'graph': graph_path, 'blueprint': bp_path,
    'seed': 90826, 'point_count': 6, 'mesh': mesh.get_path_name(),
    'center': list(center.to_tuple()), 'extent': list(extent.to_tuple()),
    'generation_radius_cm': 4500, 'constraint_radius_cm': 4000, 'cleanup_multiplier': 1.3,
    'original_actor_tags': [[str(t) for t in a.tags] for a in grass],
    'originals_retained_until_runtime_verification': True}, indent=2), encoding='utf-8')
print('Authored bounded PCGEx route patch; runtime check required before replacing original grass')
