"""Exercise creator through the real existing UI in a fresh disposable PIE campaign."""
import hashlib
import json
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
SAVE = str(globals().get('save_set', 'coastal_test_m3_character_0909a'))
if not SAVE.startswith('coastal_test_'):
    raise RuntimeError('Disposable campaign namespace required')
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if not level.is_in_play_in_editor():
    raise RuntimeError('Start PIE before this check')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn = u.GameplayStatics.get_player_character(world, 0)
pc = u.GameplayStatics.get_player_controller(world, 0)
ui = pc.get_component_by_class(u.CoastalUISessionComponent)
bridge = pawn.get_component_by_class(u.CoastalInteractionBridge)
owners = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name()]
if len(owners) != 1:
    raise RuntimeError('Expected one PIE save coordinator')
saves = owners[0]
save_dir = Path(u.Paths.project_saved_dir()) / 'SaveGames'
if saves.has_active_campaign() or any(p.name.startswith('Coastal_' + SAVE + '_') for p in save_dir.rglob('*.sav')):
    raise RuntimeError('A fresh campaign-menu PIE and unused name are required')
before = {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in save_dir.rglob('*.sav')}
original_mesh = pawn.mesh.skeletal_mesh_asset
original_anim = pawn.mesh.anim_class
original_controller = pawn.get_controller()
original_movement = pawn.get_component_by_class(u.CharacterMovementComponent)
original_inventory = pawn.get_component_by_class(u.CoastalAGISAdapter)
output = ROOT / 'local-evidence/m3-character-smoke.json'
state, rows = {}, []
phase, next_time, started = 0, 0.0, time.monotonic()
output.write_text(json.dumps(dict(status='running', passed=False, save_set=SAVE)))


def check(name, value, **details):
    if not value:
        raise RuntimeError(name + ': ' + str(details))
    rows.append(dict(case=name, passed=True, **details))


def panel():
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport() and p.is_visible()]
    if len(panels) != 1:
        raise RuntimeError('Expected exactly one visible panel')
    return panels[0]


def command(name):
    panel().call_method('Execute', args=(name,))


def creator():
    return pawn.get_component_by_class(u.CoastalMutableCharacterComponent)


def body_type():
    body = creator().get_generated_body()
    usages = [x for x in u.ObjectIterator(u.CustomizableObjectInstanceUsage)
              if x.get_attach_parent() == body]
    if len(usages) != 1:
        raise RuntimeError('Expected one generated body usage')
    return usages[0].get_customizable_object_instance().get_enum_parameter_selected_option('Body Type 1')


def finish(error=None):
    u.unregister_slate_post_tick_callback(handle)
    preserved = all(Path(path).is_file() and hashlib.sha256(Path(path).read_bytes()).hexdigest() == sha for path,sha in before.items())
    output.write_text(json.dumps(dict(passed=error is None and preserved, error=error, cases=rows,
        status='finished', seconds=time.monotonic()-started, save_set=SAVE, preexisting_saves_preserved=preserved,
        character_detail=creator().last_detail if creator() else None), indent=2))


def tick(delta):
    global phase, next_time
    now = time.monotonic()
    if now < next_time:
        return
    next_time = now + 0.3
    try:
        if now-started > 300:
            raise RuntimeError('Character smoke timeout at phase ' + str(phase))
        if phase == 0:
            if not saves.is_configured():
                return
            check('fresh campaign', saves.start_new_campaign(SAVE,pawn.get_actor_transform()) == u.CoastalSaveResult.STARTED_NEW)
            phase = 1
        elif phase == 1:
            if not creator() or not creator().is_appearance_ready():
                return
            body = creator().get_generated_body()
            check('Mutable generated body', body is not None and '/Engine/Transient' in body.skeletal_mesh_asset.get_path_name())
            check('existing mesh and animation preserved', pawn.mesh.skeletal_mesh_asset == original_mesh and pawn.mesh.anim_class == original_anim)
            check('existing controller preserved', pawn.get_controller() == original_controller)
            check('existing movement preserved', pawn.get_component_by_class(u.CharacterMovementComponent) == original_movement)
            check('existing AGIS preserved', pawn.get_component_by_class(u.CoastalAGISAdapter) == original_inventory)
            check('generated mesh has no collision', body.get_collision_enabled() == u.CollisionEnabled.NO_COLLISION)
            state['body'] = body
            ui.open_pause(); phase = 2
        elif phase == 2:
            command('character_creator'); phase = 3
        elif phase == 3:
            captures = [c for c in pawn.get_components_by_class(u.SceneCaptureComponent2D) if c.texture_target]
            check('creator opened through pause menu', len(captures) == 1, count=len(captures))
            capture = captures[0]; state['capture'] = capture
            u.RenderingLibrary.export_render_target(world, capture.texture_target, str(ROOT / 'local-evidence'), 'm3-character-preview-fixed.png')
            command('character_increase'); phase = 4; next_time = now + 1.0
        elif phase == 4:
            if creator().is_generating() or creator().get_generated_body() == state['body']:
                return
            check('customization replaced generated body', creator().get_generated_body() != state['body'])
            check('body type parameter changed', body_type() == 'Slim', actual=body_type())
            command('character_save'); phase = 5
        elif phase == 5:
            check('appearance save verified', 'applied and saved' in creator().last_detail.lower(), detail=creator().last_detail)
            state['saved_body'] = creator().get_generated_body()
            command('character_increase'); phase = 6; next_time = now + 1.0
        elif phase == 6:
            if creator().is_generating() or creator().get_generated_body() == state['saved_body']:
                return
            command('back'); phase = 7; next_time = now + 1.0
        elif phase == 7:
            if creator().is_generating():
                return
            check('preview resources removed on back', not [c for c in pawn.get_components_by_class(u.SceneCaptureComponent2D) if c.texture_target])
            check('portrait lights removed on back', not pawn.get_components_by_class(u.DirectionalLightComponent))
            check('world lighting channel restored', creator().get_generated_body().lighting_channels.channel0
                  and not creator().get_generated_body().lighting_channels.channel1)
            check('back discarded unsaved parameter', body_type() == 'Slim', actual=body_type())
            check('campaign save succeeds', saves.save_now() == u.CoastalSaveResult.SAVED)
            check('campaign reload succeeds', saves.load_campaign(SAVE) == u.CoastalSaveResult.LOADED)
            phase = 8; next_time = now + 1.0
        elif phase == 8:
            if not creator().is_appearance_ready() or creator().is_generating():
                return
            check('saved appearance restored on reload', body_type() == 'Slim', actual=body_type(), detail=creator().last_detail)
            finish(); return
    except Exception as exc:
        finish(str(exc))


handle = u.register_slate_post_tick_callback(tick)
