"""Disposable PIE acceptance for destination score, real routing, cover and lifecycle."""
import hashlib
import json
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
REPORT = ROOT / 'local-evidence/m3-soundscape-live.json'
SAVE = str(globals().get('save_set', 'coastal_test_m3_soundscape_0909e'))
if not SAVE.startswith('coastal_test_'):
    raise RuntimeError('Disposable save required')
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (
        ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pc = u.GameplayStatics.get_player_controller(world, 0)
pawn = u.GameplayStatics.get_player_character(world, 0)
ui = pc.get_component_by_class(u.CoastalUISessionComponent)
soundscape = pc.get_component_by_class(u.CoastalSoundscape)
routing = pc.get_component_by_class(u.CoastalAudioOptionsComponent)
move = pawn.get_component_by_class(u.CharacterMovementComponent)
saves = next(s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name())
if not soundscape or not soundscape.ready or saves.has_active_campaign():
    raise RuntimeError('Fresh PIE with ready soundscape required')
before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
          for p in (Path(u.Paths.project_saved_dir()) / 'SaveGames').rglob('*.sav')}
objects = {str(a.world_id): a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalWorldObject)}
rows, silenced = [], []
started = time.monotonic()
next_tick = 0
recording = False
roof = None
roof_state = None
submix = None
original_mode = move.movement_mode
app_override = u.SystemLibrary.get_console_variable_int_value('au.DisableAppVolume')
REPORT.write_text(json.dumps({'passed': False, 'status': 'running', 'save_set': SAVE}))


def check(label, condition, **details):
    if not condition:
        raise RuntimeError(label + ': ' + str(details))
    rows.append(dict(case=label, passed=True, **details))
    REPORT.write_text(json.dumps(dict(passed=False, status='running', rows=rows, save_set=SAVE), indent=2))


def until(predicate, seconds=25):
    deadline = time.monotonic() + seconds
    while not predicate():
        if time.monotonic() > deadline:
            raise RuntimeError('Condition timeout')
        yield .1


def game_seconds(seconds):
    target = u.GameplayStatics.get_time_seconds(world) + seconds
    yield from until(lambda: u.GameplayStatics.get_time_seconds(world) >= target, seconds * 4 + 10)


def command(name):
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport() and p.is_visible()]
    if len(panels) != 1:
        raise RuntimeError('Expected one visible panel: ' + name)
    panels[0].call_method('Execute', args=(name,))


def options(field=None, steps=0, extra=None):
    ui.open_pause()
    yield .25
    command('options')
    yield .25
    command('option_defaults')
    yield .15
    if field is not None:
        for _ in range(field):
            command('option_next')
            yield .05
        for _ in range(steps):
            command('option_decrease')
            yield .05
    if extra is not None:
        for _ in range((extra - field) % 10):
            command('option_next')
            yield .05
        for _ in range(20):
            command('option_decrease')
            yield .05
    command('option_session')
    yield .3
    command('back')
    yield .15
    command('back')
    yield .8


def place(location):
    move.stop_movement_immediately()
    move.set_movement_mode(u.MovementMode.MOVE_FLYING)
    pawn.set_actor_location(u.Vector(*location), False, True)


def score(expected):
    yield from until(lambda: str(soundscape.current_score) == expected and soundscape.music_gain >= .99)
    voice = soundscape.get_music_voice()
    check('score ' + expected, voice is not None and voice.is_playing(), sound=voice.sound.get_path_name() if voice else None)
    check('score routed ' + expected, voice.get_editor_property('sound_class_override') == routing.ambience_class)
    voices = [v for v in u.ObjectIterator(u.AudioComponent) if v.get_outer() == pc and v.sound
              and '/M3Soundscape/' in v.sound.get_path_name() and 'thunder' not in v.sound.get_name() and v.is_playing()]
    check('single score ' + expected, len(voices) == 1, count=len(voices))


def run():
    global roof, roof_state, submix, recording
    check('campaign menu silent', soundscape.get_music_voice() is None and soundscape.get_thunder_voice() is None)
    result = saves.start_new_campaign(SAVE, pawn.get_actor_transform())
    check('new disposable campaign', result == u.CoastalSaveResult.STARTED_NEW, result=str(result))
    yield from until(lambda: not ui.has_modal())
    yield from options()
    places = [('coast', (-6500, 22000, 192)), ('village', (-1600, 15300, 2298)),
              ('workshop', (25500, 4000, 398)), ('prison', (21600, 29000, 498)),
              ('ruins', (-16000, 6000, 298)), ('ruins', (8000, 20000, 526)),
              ('prison', (33000, -28500, 698)), ('camp', (7600, 9100, 1316))]
    radio = objects['world.test.radio'].get_actor_location()
    places.append(('cabin', (radio.x + 150, radio.y, radio.z + 100)))
    for expected, location in places:
        place(location)
        yield from score(expected)
    place((-6500, 22000, 192))
    yield from score('coast')
    voice = soundscape.get_music_voice()
    count = soundscape.music_submissions
    ui.open_pause()
    yield .5
    check('menu pauses score', (voice.get_play_state() == u.AudioComponentPlayState.PAUSED))
    command('back')
    yield .5
    check('resume retains score voice', soundscape.get_music_voice() == voice and not (voice.get_play_state() == u.AudioComponentPlayState.PAUSED)
          and soundscape.music_submissions == count)
    # Cover probe uses an actual transient colliding roof, not a synthetic shelter flag.
    # Borrow a distant primitive only inside disposable PIE; restore every changed property.
    roof = next(a for a in u.GameplayStatics.get_all_actors_of_class(world, u.StaticMeshActor)
                if a.static_mesh_component.static_mesh
                and a.static_mesh_component.static_mesh.get_path_name() == '/Engine/BasicShapes/Cube.Cube'
                and (a.get_actor_location() - pawn.get_actor_location()).length() > 10000)
    component = roof.static_mesh_component
    roof_state = (roof.get_actor_transform(), component.mobility, component.get_collision_profile_name())
    component.set_mobility(u.ComponentMobility.MOVABLE)
    roof.set_actor_location(pawn.get_actor_location() + u.Vector(0, 0, 350), False, True)
    roof.set_actor_rotation(u.Rotator(), True)
    roof.set_actor_scale3d(u.Vector(8, 8, .2))
    component.set_collision_profile_name('BlockAll')
    yield from game_seconds(3)
    check('collision cover attenuates thunder', abs(soundscape.shelter_gain - .25) < .01, gain=soundscape.shelter_gain)
    restore_roof()
    yield from game_seconds(3)
    check('open sky restores thunder gain', abs(soundscape.shelter_gain - 1) < .01, gain=soundscape.shelter_gain)
    yield from until(lambda: soundscape.get_thunder_voice() is not None, 100)
    thunder = soundscape.get_thunder_voice()
    check('scheduler starts real thunder', thunder.is_playing() and soundscape.thunder_submissions > 0)
    check('thunder uses Effects', thunder.get_editor_property('sound_class_override') == routing.effects_class)
    ui.open_pause()
    yield .4
    check('pause freezes thunder', (thunder.get_play_state() == u.AudioComponentPlayState.PAUSED))
    command('back')
    yield .4
    check('resume retains thunder', soundscape.get_thunder_voice() == thunder and not (thunder.get_play_state() == u.AudioComponentPlayState.PAUSED))
    # Actual mixer captures isolate these voices while retaining the existing option owner.
    u.SystemLibrary.execute_console_command(world, 'au.DisableAppVolume 1', pc)
    u.SystemLibrary.execute_console_command(world, 'au.Debug.SoloAudio', pc)
    # Capture an isolated send, retaining actual source and SoundClass gain controls.
    submix = None
    for other in u.ObjectIterator(u.AudioComponent):
        if 'UEDPIE_' in other.get_path_name() and other.sound and 'SW_M1Ambience' in other.sound.get_name():
            silenced.append((other, other.get_editor_property('volume_multiplier')))
            other.set_volume_multiplier(0)
    for label, field, steps in [('music-baseline', None, 0), ('music-muted', 7, 20),
                                 ('music-half', 7, 10), ('master-muted', 6, 20)]:
        yield from options(8, 20) if field is None else options(field, steps, 8)
        voice.play(0)
        yield .3
        duration = 12 if label in ('music-baseline', 'music-half') else 4
        u.AudioMixerLibrary.start_recording_output(world, duration, submix)
        recording = True
        yield duration
        u.AudioMixerLibrary.stop_recording_output(world, u.AudioRecordingExportType.WAV_FILE,
            'm3-soundscape-' + label, str(ROOT / 'local-evidence'), submix)
        recording = False
        rows.append(dict(case='mixer capture ' + label, status='captured_pending_signal_analysis'))
    yield from options()
    old_thunder = soundscape.get_thunder_voice()
    yield from until(lambda: soundscape.get_thunder_voice() is not None and soundscape.get_thunder_voice() != old_thunder, 110)
    thunder = soundscape.get_thunder_voice()
    submix = None  # Capture the real output; mute the unrelated music bus for all thunder cases.
    for label, extra in [('thunder-baseline', None), ('thunder-effects-muted', 8), ('thunder-master-muted', 6)]:
        yield from options(7, 20, extra)
        check('same thunder voice ' + label, soundscape.get_thunder_voice() == thunder)
        thunder.play(8)  # Supplied distant thunder has an intentional silent propagation pre-roll.
        yield .3
        u.AudioMixerLibrary.start_recording_output(world, 4, submix)
        recording = True
        yield 4
        u.AudioMixerLibrary.stop_recording_output(world, u.AudioRecordingExportType.WAV_FILE,
            'm3-soundscape-' + label, str(ROOT / 'local-evidence'), submix)
        recording = False
        rows.append(dict(case='mixer capture ' + label, status='captured_pending_signal_analysis'))
    yield from options()
    recovery = pawn.get_component_by_class(u.CoastalPlayerRecoveryComponent)
    check('real recovery accepted', recovery.request_defeat_return())
    paused_during_return = False
    while saves.is_player_return_active():
        yield .1
        current = soundscape.get_music_voice()
        paused_during_return |= current is not None and current.get_play_state() == u.AudioComponentPlayState.PAUSED
    check('recovery suspends score', paused_during_return)
    yield from game_seconds(5)
    previous = soundscape.get_music_voice()
    result = saves.load_campaign(SAVE)
    check('disposable reload', result == u.CoastalSaveResult.LOADED, result=str(result))
    yield from until(lambda: soundscape.get_music_voice() is not None and soundscape.get_music_voice() != previous)
    check('reload retires prior epoch', previous not in [v for v in u.ObjectIterator(u.AudioComponent) if v.get_outer() == pc and v.is_playing()])
    routing.destroy_component(pc)
    check('routing teardown stops both synchronously', not soundscape.ready and soundscape.get_music_voice() is None
          and soundscape.get_thunder_voice() is None)
    yield .5
    check('no automatic route rebind', not soundscape.ready)


iterator = run()


def restore_roof():
    global roof
    if roof is not None and roof_state is not None:
        transform, mobility, profile = roof_state
        roof.set_actor_transform(transform, False, True)
        roof.static_mesh_component.set_collision_profile_name(profile)
        roof.static_mesh_component.set_mobility(mobility)
        roof = None


def finish(error=None):
    u.unregister_slate_post_tick_callback(handle)
    if recording:
        u.AudioMixerLibrary.stop_recording_output(world, u.AudioRecordingExportType.WAV_FILE,
            'm3-soundscape-interrupted', str(ROOT / 'local-evidence'), submix)
    restore_roof()
    for voice, gain in silenced:
        try:
            voice.set_volume_multiplier(gain)
        except Exception:
            pass
    move.set_movement_mode(original_mode)
    u.SystemLibrary.execute_console_command(world, 'au.DisableAppVolume ' + str(app_override), pc)
    u.SystemLibrary.execute_console_command(world, 'au.Debug.ClearSoloAudio', pc)
    unchanged = all(Path(p).is_file() and hashlib.sha256(Path(p).read_bytes()).hexdigest() == digest for p, digest in before.items())
    REPORT.write_text(json.dumps(dict(passed=error is None and unchanged, error=error, rows=rows,
        save_set=SAVE, preexisting_saves_preserved=unchanged, elapsed_seconds=time.monotonic()-started), indent=2))


def tick(_delta):
    global next_tick
    if time.monotonic() < next_tick:
        return
    try:
        if time.monotonic()-started > 480:
            raise RuntimeError('Soundscape test timeout')
        next_tick = time.monotonic() + next(iterator)
    except StopIteration:
        finish()
    except Exception as exc:
        finish(str(exc))


handle = u.register_slate_post_tick_callback(tick)
print('Soundscape acceptance queued: ' + SAVE)
