"""Real UI/bridge feedback, mixer capture and lifetime checks in disposable First Signal PIE."""
import hashlib
import json
import math
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
REPORT = ROOT / 'local-evidence/m3-interaction-audio-live.json'
SAVE = str(globals().get('save_set', 'coastal_test_m3_audio_0909a'))
if not SAVE.startswith('coastal_test_'):
    raise RuntimeError('Disposable save namespace required')
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (
        ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent UE5.8 host required')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pc = u.GameplayStatics.get_player_controller(world, 0)
pawn = u.GameplayStatics.get_player_character(world, 0)
ui = pc.get_component_by_class(u.CoastalUISessionComponent)
feedback = pc.get_component_by_class(u.CoastalActionAudio)
routing = pc.get_component_by_class(u.CoastalAudioOptionsComponent)
bridge = pawn.get_component_by_class(u.CoastalInteractionBridge)
move = pawn.get_component_by_class(u.CharacterMovementComponent)
owners = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name()]
if len(owners) != 1 or owners[0].has_active_campaign() or not feedback or not routing.is_audio_ready():
    raise RuntimeError('Start fresh PIE at the configured campaign menu')
saves = owners[0]
objects = {str(a.world_id): a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalWorldObject)}
save_dir = Path(u.Paths.project_saved_dir()) / 'SaveGames'
before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in save_dir.rglob('*.sav')}
rows, silenced = [], []
started = time.monotonic()
deadline = started + 240
next_time = 0
recording = False
app_volume_override = u.SystemLibrary.get_console_variable_int_value('au.DisableAppVolume')
REPORT.write_text(json.dumps({'passed': False, 'status': 'running', 'save_set': SAVE}))


def count():
    return feedback.get_editor_property('submission_count')


def check(label, condition, **detail):
    if not condition:
        raise RuntimeError(label + ': ' + str(detail))
    rows.append(dict(case=label, passed=True, **detail))


def command(name):
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport() and p.is_visible()]
    if len(panels) != 1:
        raise RuntimeError('Expected one visible panel for ' + name)
    panels[0].call_method('Execute', args=(name,))


def cue(label, old_count, expected):
    voice = feedback.get_feedback_voice()
    actual = str(feedback.get_editor_property('last_cue'))
    check(label, count() == old_count + 1 and actual == expected and voice is not None,
          submissions=count() - old_count, cue=actual,
          sound=voice.sound.get_path_name() if voice else None)
    check(label + ' effects route', voice.get_editor_property('sound_class_override') == routing.effects_class)
    live = [v for v in u.ObjectIterator(u.AudioComponent) if v.get_outer() == pc and v.sound
            and '/M3Interaction/' in v.sound.get_path_name() and v.is_playing()]
    check(label + ' one voice', len(live) == 1, voices=len(live))


def use(suffix):
    target = next(a for name, a in objects.items() if name.endswith('.' + suffix))
    move.stop_movement_immediately()
    point = target.get_interaction_point()
    # Probe real reach/visibility from a few nearby positions; do not bypass the bridge.
    for degrees in range(0, 360, 45):
        angle = math.radians(degrees)
        height = 60 if suffix == 'storage' else 25
        pawn.set_actor_location(point + u.Vector(120 * math.cos(angle), 120 * math.sin(angle), height), False, True)
        if bridge.preview_interaction(target).can_interact:
            return target, bridge.try_interact(target)
    raise RuntimeError('No valid scripted approach for ' + suffix)


def options_defaults():
    ui.open_pause()
    yield .3
    command('options')
    yield .3
    command('option_defaults')
    yield .15
    command('option_session')
    yield .3
    command('back')
    yield .15
    command('back')
    yield .8


def volume(field, steps):
    ui.open_pause()
    yield .25
    command('options')
    yield .25
    for _ in range(field):
        command('option_next')
        yield .10
    for _ in range(steps):
        command('option_decrease')
        yield .10
    command('option_session')
    yield .3
    command('back')
    yield .15
    command('back')
    yield .8


def capture(label):
    global recording
    # Reload may have created a new ambience component. Isolate each recording
    # again without changing class gains or any saved preference.
    for voice in u.ObjectIterator(u.AudioComponent):
        if 'UEDPIE_' in voice.get_path_name() and voice.sound and '/M3Interaction/' not in voice.sound.get_path_name():
            silenced.append((voice, voice.get_editor_property('volume_multiplier')))
            voice.set_volume_multiplier(0)
    u.AudioMixerLibrary.start_recording_output(world, 8.0)
    recording = True
    yield .4
    for _ in range(3):
        ui.open_pause()
        yield .8
        command('back')
        yield .8
    u.AudioMixerLibrary.stop_recording_output(world, u.AudioRecordingExportType.WAV_FILE,
        'm3-feedback-' + label, str(ROOT / 'local-evidence'))
    recording = False
    rows.append({'case': 'mixer capture ' + label, 'status': 'captured_pending_signal_analysis'})
    yield .4


def run():
    # The editor can mute the PIE device and all unfocused application audio.
    # Bypass only those editor conditions; retain real Coastal Effects/Master mixes.
    u.SystemLibrary.execute_console_command(world, 'au.DisableAppVolume 1', pc)
    u.SystemLibrary.execute_console_command(world, 'au.Debug.SoloAudio', pc)
    result = saves.start_new_campaign(SAVE, pawn.get_actor_transform())
    check('fresh disposable campaign', result == u.CoastalSaveResult.STARTED_NEW, result=str(result))
    yield 1.2
    check('campaign UI settled', not ui.has_modal() and saves.has_active_campaign())
    for voice in u.ObjectIterator(u.AudioComponent):
        if 'UEDPIE_' in voice.get_path_name() and voice.sound and '/M3Interaction/' not in voice.sound.get_path_name():
            silenced.append((voice, voice.get_editor_property('volume_multiplier')))
            voice.set_volume_multiplier(0)
    yield from options_defaults()
    old = count()
    ui.open_pause()
    cue('pause open', old, 'confirm')
    ui.open_pause()
    check('repeated pause does not duplicate', count() == old + 1)
    yield .3
    old = count()
    command('unimplemented_command')
    check('invalid UI command is quiet', count() == old)
    yield .2
    command('back')
    cue('explicit back', old, 'cancel')
    yield .8
    old = count()
    battery, result = use('battery')
    check('real AGIS pickup applied', result == u.CoastalActionResult.APPLIED, result=str(result))
    cue('pickup', old, 'pickup')
    check('same-frame repeat suppressed', bridge.try_interact(battery) == u.CoastalActionResult.SUPPRESSED_INPUT)
    yield .8
    old = count()
    check('already collected stays quiet', bridge.try_interact(battery) == u.CoastalActionResult.ALREADY_APPLIED and count() == old)
    old = count()
    _, result = use('fuse')
    # The previous repeat claimed this frame; a fresh input is required.
    check('one input owner per frame', result == u.CoastalActionResult.SUPPRESSED_INPUT and count() == old)
    yield .3
    _, result = use('fuse')
    check('second real pickup applied', result == u.CoastalActionResult.APPLIED)
    cue('second pickup', old, 'pickup')
    yield .8
    old = count()
    _, result = use('door')
    check('door operation applied', result == u.CoastalActionResult.APPLIED)
    cue('door opens', old, 'door_open')
    voice = feedback.get_feedback_voice()
    check('door spatial attenuation configured', voice.get_editor_property('allow_spatialization') and
          voice.get_editor_property('override_attenuation'))
    yield .3
    ui.open_pause()
    check('pause replaces world cue', str(feedback.last_cue) == 'confirm')
    yield .3
    command('back')
    yield .6
    old = count()
    _, result = use('door')
    check('door closes', result == u.CoastalActionResult.APPLIED)
    cue('door close sound', old, 'door_close')
    yield 1.2
    old = count()
    _, result = use('storage')
    check('storage opened through bridge', result == u.CoastalActionResult.APPLIED)
    cue('storage cue', old, 'storage')
    yield .8
    check('automatic storage panel has no second cue', count() == old + 1 and ui.has_modal(),
          submissions=count() - old, modal=ui.has_modal())
    old = count()
    command('transfer')
    cue('real inventory transfer', old, 'pickup')
    yield .8
    command('back')
    yield .6
    old = count()
    _, result = use('note')
    check('note applied', result == u.CoastalActionResult.APPLIED)
    cue('note paper', old, 'paper')
    yield .25
    loaded = saves.load_campaign(SAVE)
    check('campaign reload', loaded == u.CoastalSaveResult.LOADED, result=str(loaded))
    yield .8
    check('reload retires old cue', feedback.get_feedback_voice() is None)
    old = count()
    ui.open_journal()
    cue('journal opened', old, 'journal')
    yield .6
    command('back')
    yield .8
    old = count()
    record, result = use('postcard')
    check('discovery recorded', result == u.CoastalActionResult.APPLIED)
    cue('discovery journal cue', old, 'journal')
    yield .8
    check('automatic journal has no second cue', ui.has_modal() and count() == old + 1)
    command('back')
    yield .6
    old = count()
    check('record reread is idempotent', bridge.try_interact(record) == u.CoastalActionResult.ALREADY_APPLIED)
    yield .6
    check('reread opens journal without new reward cue', ui.has_modal() and count() == old)
    command('back')
    yield .6
    old = count()
    _, result = use('note')
    cue('paper before recovery', old, 'paper')
    recovery = pawn.get_component_by_class(u.CoastalPlayerRecoveryComponent)
    check('real recovery started', recovery.request_defeat_return())
    yield .2
    check('recovery retires playing cue', feedback.get_feedback_voice() is None)
    yield 2.0
    check('recovery does not replay cue', not recovery.is_returning() and count() == old + 1)
    # Force a real TooFar refusal against a still-live door, with no world mutation.
    door = next(a for name, a in objects.items() if name.endswith('.door'))
    pawn.set_actor_location(saves.get_dry_checkpoint().translation, False, True)
    if (pawn.get_actor_location() - door.get_interaction_point()).length() < 500:
        pawn.set_actor_location(door.get_interaction_point() + u.Vector(600, 0, 0), False, True)
    old = count()
    result = bridge.try_interact(door)
    check('distant operation refused', result == u.CoastalActionResult.TOO_FAR, result=str(result))
    cue('distinct error', old, 'error')
    yield 1.3
    check('finished cue cleaned up without retry', feedback.get_feedback_voice() is None and count() == old + 1)
    yield from capture('baseline')
    yield from volume(8, 20)
    yield from capture('effects-muted')
    yield from options_defaults()
    yield from volume(6, 20)
    yield from capture('master-muted')
    yield from options_defaults()
    yield from volume(8, 10)
    yield from capture('effects-half')
    yield from options_defaults()
    ui.open_pause()
    check('voice before routing teardown', feedback.get_feedback_voice() is not None)
    routing.destroy_component(pc)
    check('routing release stops voice synchronously', feedback.get_feedback_voice() is None)
    yield .3
    old = count()
    command('back')
    check('released route does not rebind or replay', count() == old)


iterator = run()


def finish(error=None):
    global recording
    u.unregister_slate_post_tick_callback(handle)
    if recording:
        u.AudioMixerLibrary.stop_recording_output(world, u.AudioRecordingExportType.WAV_FILE,
            'm3-feedback-interrupted', str(ROOT / 'local-evidence'))
        recording = False
    for voice, gain in reversed(silenced):
        try:
            voice.set_volume_multiplier(gain)
        except Exception:
            pass
    u.SystemLibrary.execute_console_command(world, 'au.DisableAppVolume ' + str(app_volume_override), pc)
    u.SystemLibrary.execute_console_command(world, 'au.Debug.ClearSoloAudio', pc)
    unchanged = all(Path(p).is_file() and hashlib.sha256(Path(p).read_bytes()).hexdigest() == digest
                    for p, digest in before.items())
    result = {'passed': error is None and unchanged, 'error': error, 'rows': rows,
              'save_set': SAVE, 'preexisting_save_files_preserved': unchanged,
              'elapsed_seconds': time.monotonic() - started,
              'capture_setup': 'Temporarily bypass app-focus mute and solo PIE device; restored after capture',
              'scope': 'Scripted real UI/bridge/AGIS and mixer capture; no physical-device or listening acceptance'}
    REPORT.write_text(json.dumps(result, indent=2))
    print(json.dumps(result))


def tick(_delta):
    global next_time
    if time.monotonic() < next_time:
        return
    try:
        if time.monotonic() > deadline:
            raise RuntimeError('Live feedback test timeout')
        next_time = time.monotonic() + next(iterator)
    except StopIteration:
        finish()
    except Exception as exc:
        finish(str(exc))


handle = u.register_slate_post_tick_callback(tick)
print('Interaction audio gameplay/mixer checks queued for ' + SAVE)
