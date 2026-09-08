"""Small idempotent authoring helpers for the two September 8 destinations."""
import math
import unreal as u


class Destination:
    def __init__(self, key):
        self.level = u.get_editor_subsystem(u.LevelEditorSubsystem)
        self.editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
        self.actors = u.get_editor_subsystem(u.EditorActorSubsystem)
        expected_hosts = (
            'F:/coastline/LocalHost/CoastalExploration/CoastalExploration.uproject',
            'F:/coastline/LocalHost58/CoastalExploration/CoastalExploration.uproject',
        )
        if (u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path()).replace('\\', '/') not in expected_hosts
                or self.editor.get_editor_world().get_name() != 'L_FirstSignal'
                or self.level.is_in_play_in_editor()):
            raise RuntimeError('Expected the local First Signal editor outside PIE')
        self.prefix = 'Coastal.Expansion.' + key + '.'
        self.key = key
        self.known = {}
        for actor in self.actors.get_all_level_actors():
            for tag in actor.tags:
                if str(tag).startswith(self.prefix):
                    if str(tag) in self.known:
                        raise RuntimeError('Duplicate authored tag: ' + str(tag))
                    self.known[str(tag)] = actor
        self.written = []

    def actor(self, cls, key, location, yaw=0):
        tag = self.prefix + key
        actor = self.known.get(tag)
        if actor and not isinstance(actor, cls):
            raise RuntimeError('Authored actor class changed: ' + tag)
        if not actor:
            actor = self.actors.spawn_actor_from_class(cls, u.Vector(*location))
            if not actor:
                raise RuntimeError('Could not spawn ' + tag)
            actor.tags = [u.Name(tag)]
            self.known[tag] = actor
        actor.set_actor_location_and_rotation(u.Vector(*location), u.Rotator(yaw=yaw), False, True)
        actor.set_actor_label(self.key + ' — ' + key)
        actor.set_folder_path('Coastal Expansion/' + self.key)
        self.written.append(actor)
        return actor

    def mesh(self, key, asset, location, yaw=0, scale=(1, 1, 1), collision=True, materials=None):
        sm = u.load_asset(asset) if isinstance(asset, str) else asset
        if not isinstance(sm, u.StaticMesh):
            raise RuntimeError('Missing static mesh: ' + str(asset))
        actor = self.actor(u.StaticMeshActor, key, location, yaw)
        actor.set_actor_scale3d(u.Vector(*scale))
        component = actor.static_mesh_component
        component.set_static_mesh(sm)
        component.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
        for index, material in enumerate(materials or []):
            if material:
                component.set_material(index, u.load_asset(material))
        return actor

    def fitted(self, key, asset, center, size, yaw=0, collision=False):
        sm = u.load_asset(asset)
        if not isinstance(sm, u.StaticMesh):
            raise RuntimeError('Missing static mesh: ' + asset)
        dims = [v * 2 for v in sm.get_bounds().box_extent.to_tuple()]
        if any(v < .01 for v in dims):
            raise RuntimeError('Cannot fit degenerate mesh: ' + asset)
        actor = self.mesh(key, sm, (0, 0, 0), yaw,
                          tuple(s / d for s, d in zip(size, dims)), collision)
        actor.set_actor_location(u.Vector(*center) - actor.get_actor_bounds(False)[0], False, True)
        return actor

    def box(self, key, center, size, material, hidden=False):
        actor = self.mesh(key, '/Engine/BasicShapes/Cube', center,
                          scale=tuple(v / 100 for v in size), materials=[material])
        actor.set_actor_hidden_in_game(hidden)
        return actor

    def path(self, key, points, material, width=500, rails=False):
        for index, (a, b) in enumerate(zip(points, points[1:])):
            dx, dy, dz = [end - start for start, end in zip(a, b)]
            horizontal = math.hypot(dx, dy)
            if horizontal < 1 or abs(dz) / horizontal > .25:
                raise RuntimeError('Invalid walkable route grade')
            actor = self.box(key + str(index),
                             tuple((start + end) / 2 - (12 if axis == 2 else 0)
                                   for axis, (start, end) in enumerate(zip(a, b))),
                             (math.sqrt(horizontal ** 2 + dz ** 2) + 16, width, 24), material)
            actor.set_actor_rotation(u.Rotator(pitch=math.degrees(math.atan2(dz, horizontal)),
                                              yaw=math.degrees(math.atan2(dy, dx))), False)
            if rails:
                for side in (-1, 1):
                    nx, ny = -dy / horizontal * width * .48 * side, dx / horizontal * width * .48 * side
                    beam = self.box(f'{key}rail {index} {side}',
                                    ((a[0] + b[0]) / 2 + nx, (a[1] + b[1]) / 2 + ny,
                                     (a[2] + b[2]) / 2 + 95),
                                    (math.sqrt(horizontal ** 2 + dz ** 2), 10, 10), material)
                    beam.set_actor_rotation(actor.get_actor_rotation(), False)
                    count = math.ceil(horizontal / 400)
                    for post in range(count + 1):
                        t = post / count
                        self.box(f'{key}post {index} {side} {post}',
                                 (a[0] + dx * t + nx, a[1] + dy * t + ny, a[2] + dz * t + 48),
                                 (14, 14, 96), material)

    def sign(self, key, text, position, yaw=-90):
        actor = self.actor(u.TextRenderActor, key, position, yaw)
        component = actor.get_component_by_class(u.TextRenderComponent)
        component.set_text(text)
        component.set_world_size(32)
        component.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
        component.set_text_render_color(u.Color(235, 220, 180, 255))
        return actor

    def checkpoint(self, name, ground):
        center = (ground[0], ground[1], ground[2] + 98)
        actor = self.actor(u.CoastalSafetyVolume, 'Dry arrival', center)
        actor.set_editor_property('kind', u.CoastalSafetyKind.DRY_CHECKPOINT)
        actor.get_editor_property('bounds').set_box_extent(u.Vector(180, 180, 110), False)
        actor.get_editor_property('return_point').set_world_location(u.Vector(*center), False, False)
        self.sign('Arrival sign', name, (ground[0], ground[1] - 240, ground[2] + 180))
        return center

    def save(self):
        if not self.level.save_current_level():
            raise RuntimeError('Destination level save failed')
        return [{'name': a.get_actor_label(), 'path': a.get_path_name(),
                 'position': list(a.get_actor_location().to_tuple())} for a in self.written]
