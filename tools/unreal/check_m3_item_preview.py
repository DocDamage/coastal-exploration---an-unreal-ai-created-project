"""Real AGIS pickup, existing detail UI, Slate rotation, cleanup and save/reload in fresh PIE."""
import hashlib
import json
import math
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
SAVE = str(globals().get('save_set', ''))
if not SAVE.startswith('coastal_test_'):
    raise RuntimeError('Supply a fresh disposable campaign name')
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (
        ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if not level.is_in_play_in_editor():
    raise RuntimeError('Start disposable PIE at the campaign menu')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pc = u.GameplayStatics.get_player_controller(world, 0)
pawn = u.GameplayStatics.get_player_character(world, 0)
if not world or not pc or not pawn:
    raise RuntimeError('Expected one local PIE player')
ui = pc.get_component_by_class(u.CoastalUISessionComponent)
bridge = pawn.get_component_by_class(u.CoastalInteractionBridge)
provider = pawn.get_component_by_class(u.CoastalAGISAdapter)
move = pawn.get_component_by_class(u.CharacterMovementComponent)
saves_owners = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name()]
if len(saves_owners) != 1 or not ui or not bridge or not provider or not move:
    raise RuntimeError('Expected configured local UI, bridge, AGIS provider, movement and one PIE coordinator')
saves = saves_owners[0]
save_dir = Path(u.Paths.project_saved_dir()) / 'SaveGames'
if saves.has_active_campaign() or any(p.name.startswith('Coastal_' + SAVE + '_') for p in save_dir.rglob('*.sav')):
    raise RuntimeError('Start fresh PIE at campaign menu')
before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
          for p in save_dir.rglob('*.sav')}
objects = {str(a.world_id): a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalWorldObject)}
transforms = {key: a.get_actor_transform() for key, a in objects.items()}
rows, next_time, started = [], 0, time.monotonic()
REPORT = ROOT / 'local-evidence/m3-item-preview-live.json'
REPORT.write_text(json.dumps(dict(passed=False, status='running', save_set=SAVE)))


def check(case, value, **details):
    if not value:
        raise RuntimeError(case + ': ' + str(details))
    rows.append(dict(case=case, passed=True, **details))


def panel():
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport() and p.is_visible()]
    if len(panels) != 1:
        raise RuntimeError('Expected exactly one visible existing UI panel')
    return panels[0]


def command(name):
    panel().call_method('Execute', args=(name,))


def preview():
    return pc.get_component_by_class(u.CoastalItemPreviewComponent)


def inventory():
    result = provider.read_container_view('container.player')
    values = result if isinstance(result, tuple) else (result,)
    view = next((x for x in values if isinstance(x, u.CoastalContainerView)), None)
    status = next((x for x in values if isinstance(x, u.CoastalProviderResult)), None)
    if status != u.CoastalProviderResult.READY or view is None:
        raise RuntimeError('AGIS player projection was not ready: ' + str(result))
    return [(x.instance_id.to_string(), str(x.item_id), x.quantity) for x in view.items]


def pose():
    capture = next(c for c in u.ObjectIterator(u.SceneCaptureComponent2D)
                   if c.texture_target == preview().get_preview_texture())
    mesh = next(c for c in capture.get_owner().get_components_by_class(u.StaticMeshComponent) if c.static_mesh
                and '/ItemPreview/' in c.static_mesh.get_path_name())
    return mesh.get_world_rotation(), capture


def preview_pixels(texture, index):
    width, height = texture.get_editor_property('size_x'), texture.get_editor_property('size_y')
    check('preview target dimensions ' + str(index), width == 384 and height == 384,
          width=width, height=height)
    # Read a small centered grid: per-pixel calls are intentionally expensive, so
    # this proves the captured art has color without treating it as an image diff.
    samples = []
    for y_fraction in range(1, 6):
        for x_fraction in range(1, 6):
            color = u.RenderingLibrary.read_render_target_pixel(
                world, texture, width // 2 + (x_fraction - 3) * width // 12,
                height // 2 + (y_fraction - 3) * height // 12)
            samples.append((int(color.r), int(color.g), int(color.b), int(color.a)))
    # Both original objects contain dark teal. White shader fallbacks, bloom and
    # a flat clear target must not be mistaken for the actual palette.
    colored = [rgba for rgba in samples if 8 <= max(rgba[:3]) < 245
               and min(rgba[:3]) < max(rgba[:3]) * .65]
    check('preview capture contains colored art ' + str(index), len(colored) >= 4,
          colored_samples=len(colored), samples=samples)
    check('preview capture is not a flat clear target ' + str(index), len(set(samples)) >= 2,
          distinct_samples=len(set(samples)))
    evidence = ROOT / 'local-evidence' / ('m3-item-preview-' + str(index) + '.png')
    u.RenderingLibrary.export_render_target(world, texture, str(evidence.parent), evidence.name)
    check('preview capture export ' + str(index), evidence.is_file() and evidence.stat().st_size > 64,
          path=str(evidence), bytes=evidence.stat().st_size if evidence.exists() else 0)


def use(suffix):
    target = next(a for name, a in objects.items() if name.endswith('.' + suffix))
    move.stop_movement_immediately()
    point = target.get_interaction_point()
    for degrees in range(0, 360, 45):
        angle = math.radians(degrees)
        pawn.set_actor_location(point + u.Vector(120*math.cos(angle),120*math.sin(angle),25), False, True)
        if bridge.preview_interaction(target).can_interact:
            return target, bridge.try_interact(target)
    raise RuntimeError('No valid real pickup approach')


def run():
    check('fresh campaign', saves.start_new_campaign(SAVE, pawn.get_actor_transform()) == u.CoastalSaveResult.STARTED_NEW)
    yield 1
    for suffix in ('battery', 'fuse'):
        target = next(a for name, a in objects.items() if name.endswith('.' + suffix))
        pawn.set_actor_location(target.get_interaction_point() + u.Vector(1000,0,50), False, True)
        check(suffix + ' distant pickup refused', bridge.try_interact(target) == u.CoastalActionResult.TOO_FAR)
        yield .2
        target, result = use(suffix)
        check(suffix + ' authoritative pickup', result == u.CoastalActionResult.APPLIED)
        yield .4
        check(suffix + ' repeat pickup idempotent', bridge.try_interact(target) == u.CoastalActionResult.ALREADY_APPLIED)
        yield .2
    original_inventory = inventory()
    check('two conserved AGIS instances', len(original_inventory) == 2 and all(row[2] == 1 for row in original_inventory))
    ui.open_inventory()
    yield .5
    for index in range(2):
        command('inspect')
        yield .5
        owner = preview()
        check('preview opens ' + str(index), owner is not None and owner.is_preview_open())
        texture = owner.get_preview_texture()
        check('preview texture exists ' + str(index), texture is not None)
        # View the sides as well as the default end-on metal contact of the
        # fuse, using the same buttons available to a player.
        for _ in range(3 if index == 0 else 6):
            command('rotate_right')
            yield .15
        yield .3
        art_deadline = time.monotonic() + 30
        while True:
            color = u.RenderingLibrary.read_render_target_pixel(world, texture, 192, 192)
            rgb = (int(color.r), int(color.g), int(color.b))
            # The fuse's center is a neutral metal contact; palette coverage is
            # checked across the grid below, rather than at this one pixel.
            if 8 <= max(rgb) < 245:
                break
            if time.monotonic() > art_deadline:
                raise RuntimeError('Preview did not refresh its temporary material fallback: ' + str(rgb))
            yield .25
        preview_pixels(texture, index)
        original_transform = pawn.get_actor_transform()
        control = pc.get_control_rotation()
        prior, _ = pose()
        command('rotate_right')
        yield .2
        changed, _ = pose()
        check('button rotates mesh ' + str(index), changed != prior)
        prior = changed
        check('Slate keyboard routed', u.CoastalVendorInspection.submit_item_preview_test_input('D', 1))
        u.CoastalVendorInspection.submit_item_preview_test_input('D', 0)
        yield .2
        check('keyboard rotates preview', pose()[0] != prior)
        prior = pose()[0]
        check('Slate gamepad routed', u.CoastalVendorInspection.submit_item_preview_test_input('Gamepad_RightX', 1))
        yield .2
        check('gamepad rotates preview', pose()[0] != prior)
        u.CoastalVendorInspection.submit_item_preview_test_input('Gamepad_RightX', 0)
        check('stale instance refuses rotation', not owner.rotate_preview(u.Guid(), owner.previewed_revision(),15,0))
        check('stale revision refuses rotation', not owner.rotate_preview(owner.previewed_instance(),owner.previewed_revision()+1,15,0))
        check('no camera/control movement', pc.get_control_rotation() == control and pawn.get_actor_transform() == original_transform)
        check('AGIS unchanged by inspection', inventory() == original_inventory)
        command('back')
        yield .3
        check('close releases capture target', not owner.is_preview_open() and owner.get_preview_texture() is None)
        command('next_item')
        yield .2
    command('inspect')
    yield .4
    owner = preview()
    check('preview before recovery', owner is not None and owner.is_preview_open())
    # The existing recovery owner correctly refuses a request while any modal owns
    # UI input. Close only through the actual panel flow before requesting it.
    command('back')
    yield .2
    command('back')
    yield .3
    check('modal released for recovery', not ui.has_modal() and bridge.allows_world_input())
    recovery = pawn.get_component_by_class(u.CoastalPlayerRecoveryComponent)
    check('real recovery accepted after detail cleanup', recovery is not None and recovery.request_defeat_return())
    recovery_deadline = time.monotonic() + 10
    while recovery.is_returning():
        if time.monotonic() > recovery_deadline:
            raise RuntimeError('Recovery did not finish after preview cleanup')
        yield .1
    yield .3
    check('recovery retains no preview resource', not owner.is_preview_open() and owner.get_preview_texture() is None)
    check('recovery preserves AGIS instances', inventory() == original_inventory)
    check('recovery restores gameplay input', bridge.allows_world_input()
          and not pc.is_move_input_ignored() and not pc.is_look_input_ignored())
    ui.open_inventory()
    yield .4
    command('inspect')
    yield .4
    check('preview reopens after recovery', preview().is_preview_open())
    # A coherent save/reload invalidates the preview's captured instance revision.
    check('save succeeds', saves.save_now() == u.CoastalSaveResult.SAVED)
    check('reload succeeds', saves.load_campaign(SAVE) == u.CoastalSaveResult.LOADED)
    yield .6
    check('epoch cleanup', not preview().is_preview_open() and not ui.has_modal())
    check('same item GUIDs after reload', inventory() == original_inventory)
    check('world object transforms unchanged', all(a.get_actor_transform() == transforms[key] for key,a in objects.items()))
    check('gameplay input restored', bridge.allows_world_input() and not pc.is_move_input_ignored() and not pc.is_look_input_ignored())


iterator = run()


def finish(error=None):
    u.unregister_slate_post_tick_callback(handle)
    if preview():
        preview().close_preview()
    unchanged = all(Path(p).exists() and hashlib.sha256(Path(p).read_bytes()).hexdigest() == h for p,h in before.items())
    REPORT.write_text(json.dumps(dict(passed=error is None and unchanged, error=error, rows=rows,
        save_set=SAVE, preexisting_saves_preserved=unchanged, elapsed_seconds=time.monotonic()-started), indent=2))


def tick(_delta):
    global next_time
    if time.monotonic() < next_time:
        return
    try:
        if time.monotonic()-started > 150:
            raise RuntimeError('Item preview acceptance timeout')
        next_time = time.monotonic() + next(iterator)
    except StopIteration:
        finish()
    except Exception as exc:
        finish(str(exc))


handle = u.register_slate_post_tick_callback(tick)
print('Item preview acceptance queued: ' + SAVE)
