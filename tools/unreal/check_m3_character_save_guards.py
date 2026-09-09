"""Verify campaign isolation and preserved invalid appearance files in a disposable campaign."""
import hashlib
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
original_set = str(saves.get_active_save_set())
other_set = str(globals().get('other_save_set', 'coastal_test_m3_character_guards_0909a'))
save_dir = Path(u.Paths.project_saved_dir()) / 'SaveGames'
if not original_set.startswith('coastal_test_') or not other_set.startswith('coastal_test_'):
    raise RuntimeError('Disposable campaigns required')
if any(p.name.startswith('Coastal_'+other_set+'_') for p in save_dir.rglob('*.sav')):
    raise RuntimeError('Unused guard campaign name required')
report = ROOT / 'local-evidence/m3-character-save-guards.json'
rows, originals = [], {}
started = time.monotonic()


def check(name, ok, **details):
    if not ok:
        raise RuntimeError(name + ': ' + str(details))
    rows.append(dict(case=name, passed=True, **details))


def command(name):
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport() and p.is_visible()]
    check('one active panel', len(panels) == 1)
    panels[0].call_method('Execute', args=(name,))


def body_type():
    body = creator.get_generated_body()
    usage = next(x for x in u.ObjectIterator(u.CustomizableObjectInstanceUsage) if x.get_attach_parent() == body)
    return usage.get_customizable_object_instance().get_enum_parameter_selected_option('Body Type 1')


def ready():
    return creator.is_appearance_ready() and not creator.is_generating()


def run():
    while ui.has_modal():
        command('back'); yield .35
    expected = body_type()
    existing = set(save_dir.glob('CoastalAppearance_*.sav'))
    check('second campaign starts', saves.start_new_campaign(other_set,pawn.get_actor_transform()) == u.CoastalSaveResult.STARTED_NEW)
    yield 1
    while not ready(): yield .3
    check('new campaign uses default appearance', body_type() == 'Regular')
    ui.open_pause(); yield .35
    command('character_creator'); yield .4
    command('character_save'); yield .35
    command('character_save'); yield .35
    command('back'); yield .35
    paths = sorted(set(save_dir.glob('CoastalAppearance_*.sav'))-existing)
    check('two independent sidecar generations', len(paths) == 2)
    originals.update({p:p.read_bytes() for p in paths})
    check('original campaign reloads', saves.load_campaign(original_set) == u.CoastalSaveResult.LOADED)
    yield 1
    while not ready(): yield .3
    check('campaign switch restores original appearance', body_type() == expected, expected=expected)

    latest = next(p for p in paths if p.stem.endswith('_0'))
    previous = next(p for p in paths if p.stem.endswith('_1'))
    for field in ('future_version', 'wrong_save_class'):
        for p, data in originals.items(): p.write_bytes(data)
        if field == 'future_version':
            data = latest.read_bytes()
            check('one serialized appearance version', data.count(b'coastal.human.v1') == 1)
            latest.write_bytes(data.replace(b'coastal.human.v1', b'coastal.human.v9'))
        else:
            saved = u.GameplayStatics.create_save_game_object(u.CoastalCampaignSave)
            check('disposable wrong-class slot created', u.GameplayStatics.save_game_to_slot(saved,latest.stem,0))
        invalid_bytes = latest.read_bytes()
        check('campaign loads despite cosmetic damage', saves.load_campaign(other_set) == u.CoastalSaveResult.LOADED)
        yield 1
        while not ready(): yield .3
        check('valid earlier appearance restored ' + field, body_type() == 'Regular')
        check('read-only fallback explained ' + field, 'saving is disabled' in creator.last_detail.lower(), detail=creator.last_detail)
        ui.open_pause(); yield .35
        command('character_creator'); yield .35
        check('creator explains disabled saving ' + field, 'saving is disabled' in creator.last_detail.lower())
        command('character_save'); yield .35
        check('invalid appearance bytes preserved ' + field, latest.read_bytes() == invalid_bytes)
        command('back'); yield .35
    damaged = u.GameplayStatics.create_save_game_object(u.CoastalCampaignSave)
    check('second disposable invalid slot created', u.GameplayStatics.save_game_to_slot(damaged,previous.stem,0))
    invalid = {p:p.read_bytes() for p in paths}
    check('campaign loads with both appearance slots invalid', saves.load_campaign(other_set) == u.CoastalSaveResult.LOADED)
    yield 1
    check('invalid appearance falls back to native player', not creator.is_appearance_ready() and not pawn.mesh.get_editor_property('hidden_in_game'))
    check('unreadable appearance explained', 'could not be read' in creator.last_detail.lower())
    check('both invalid files preserved', all(p.read_bytes()==data for p,data in invalid.items()))
    for p,data in originals.items(): p.write_bytes(data)
    check('original campaign restored at end', saves.load_campaign(original_set) == u.CoastalSaveResult.LOADED)
    yield 1
    while not ready(): yield .3
    check('original appearance retained after guards', body_type() == expected)


steps = run()
next_tick = 0.0
report.write_text(json.dumps(dict(status='running', passed=False)))


def tick(delta):
    global next_tick
    if time.monotonic() < next_tick: return
    error = None
    try:
        if time.monotonic()-started > 180: raise RuntimeError('Save guard test timed out')
        next_tick=time.monotonic()+next(steps)
        return
    except StopIteration: pass
    except Exception as exc: error=str(exc)
    u.unregister_slate_post_tick_callback(handle)
    for p,data in originals.items(): p.write_bytes(data)
    report.write_text(json.dumps(dict(status='finished', passed=error is None, error=error,
        cases=rows, seconds=time.monotonic()-started, damaged_test_files_restored=True), indent=2))


handle = u.register_slate_post_tick_callback(tick)
