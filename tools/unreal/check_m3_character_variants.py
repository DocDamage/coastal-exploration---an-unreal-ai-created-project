"""Change all exposed controls through the real creator, and verify generated parameters and grooms."""
import json
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn = u.GameplayStatics.get_player_character(world, 0)
ui = pawn.get_controller().get_component_by_class(u.CoastalUISessionComponent)
creator = pawn.get_component_by_class(u.CoastalMutableCharacterComponent)
saves = next(s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name())
if not str(saves.get_active_save_set()).startswith('coastal_test_') or not creator.is_appearance_ready():
    raise RuntimeError('Generated character in a disposable campaign required')
definition = u.load_asset('/Game/Coastal/Character/DA_CoastalCharacter')
controls = definition.get_editor_property('controls')
report = ROOT / 'local-evidence/m3-character-variants.json'
rows = []
started = time.monotonic()


def check(name, ok, **detail):
    if not ok:
        raise RuntimeError(name + ': ' + str(detail))
    rows.append(dict(case=name, passed=True, **detail))


def command(name):
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport() and p.is_visible()]
    check('one active panel', len(panels) == 1)
    panels[0].call_method('Execute', args=(name,))


def instance():
    usages = [x for x in u.ObjectIterator(u.CustomizableObjectInstanceUsage)
              if x.get_attach_parent() == creator.get_generated_body()]
    if len(usages) != 1:
        raise RuntimeError('One body usage required')
    return usages[0].get_customizable_object_instance()


def value(control):
    name = control.get_editor_property('parameter')
    if control.get_editor_property('choices'):
        return instance().get_enum_parameter_selected_option(name)
    return instance().get_float_parameter_selected_option(name)


def capture(name):
    target = next(c.texture_target for c in pawn.get_components_by_class(u.SceneCaptureComponent2D) if c.texture_target)
    u.RenderingLibrary.export_render_target(world, target, str(ROOT / 'local-evidence'), name + '.png')


def run():
    while ui.has_modal():
        command('back'); yield .35
    ui.open_pause(); yield .35
    command('character_creator'); yield 1
    for index, control in enumerate(controls):
        name = control.get_editor_property('parameter')
        before = value(control)
        body = creator.get_generated_body()
        command('character_increase'); yield .4
        deadline = time.monotonic() + 120
        while creator.is_generating() or creator.get_generated_body() == body:
            if time.monotonic() > deadline:
                raise RuntimeError('Generation timeout for ' + name + ': ' + creator.last_detail)
            yield .3
        after = value(control)
        check('generated option ' + name, after != before, before=before, after=after)
        linked = control.get_editor_property('linked_parameter')
        if linked:
            check('linked body shape matches', instance().get_enum_parameter_selected_option(linked) == after)
        if name == 'Head Accessories':
            grooms = pawn.get_components_by_class(u.GroomComponent)
            check('hair creates native groom components', len(grooms) > 0, count=len(grooms))
        if index in (3, 11, 15):
            yield 1
            capture('m3-character-variant-' + str(index))
        if index + 1 < len(controls):
            command('character_next'); yield .35
    command('character_save'); yield .35
    check('variant saved', 'applied and saved' in creator.last_detail.lower())
    expected = [value(c) for c in controls]
    command('back'); yield .4
    check('variant reload accepted', saves.load_campaign(str(saves.get_active_save_set())) == u.CoastalSaveResult.LOADED)
    yield 1
    while creator.is_generating() or not creator.is_appearance_ready():
        yield .3
    check('all variant values survive reload', [value(c) for c in controls] == expected, values=expected)
    check('grooms survive reload', len(pawn.get_components_by_class(u.GroomComponent)) > 0)


steps = run()
next_tick = 0.0
report.write_text(json.dumps(dict(status='running', passed=False)))


def tick(delta):
    global next_tick
    if time.monotonic() < next_tick:
        return
    error = None
    try:
        if time.monotonic() - started > 600:
            raise RuntimeError('Variant test timed out')
        next_tick = time.monotonic() + next(steps)
        return
    except StopIteration:
        pass
    except Exception as exc:
        error = str(exc)
    u.unregister_slate_post_tick_callback(handle)
    report.write_text(json.dumps(dict(status='finished', passed=error is None, error=error,
        cases=rows, seconds=time.monotonic()-started), indent=2))


handle = u.register_slate_post_tick_callback(tick)
