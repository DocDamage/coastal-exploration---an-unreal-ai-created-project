"""Exercise village journal gating, pitch completion and lifecycle cleanup in disposable PIE."""
import hashlib
import json
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
REPORT = ROOT / 'local-evidence/m3-merchant-live.json'
SAVE = str(globals().get('save_set', ''))
if not SAVE.startswith('coastal_test_'):
    raise RuntimeError('Pass an active completed disposable save_set beginning coastal_test_')
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (
        ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent UE5.8 host required')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
pawn = u.GameplayStatics.get_player_character(world, 0)
pc = u.GameplayStatics.get_player_controller(world, 0)
ui = pc.get_component_by_class(u.CoastalUISessionComponent)
bridge = pawn.get_component_by_class(u.CoastalInteractionBridge)
move = pawn.get_component_by_class(u.CharacterMovementComponent)
saves = next(s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name())
presenters = u.GameplayStatics.get_all_actors_of_class(world, u.CoastalMerchantPresenter)
objects = {str(a.world_id): a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalWorldObject)}
if not level.is_in_play_in_editor() or str(saves.get_active_save_set()) != SAVE or len(presenters) != 1:
    raise RuntimeError('Expected named active disposable PIE campaign and one merchant')
missions = [m for m in u.ObjectIterator(u.FirstSignalComponent) if 'UEDPIE_' in m.get_path_name()]
if len(missions) != 1 or not missions[0].export_snapshot().message_heard:
    raise RuntimeError('Disposable campaign must have completed First Signal')
destination_journal = [str(j) for j in saves.get_journal() if str(j).startswith(
    ('journal.north_reach.harbour_', 'journal.north_reach.platform_',
     'journal.north_reach.powell_', 'journal.coastal_records.'))]
if destination_journal:
    raise RuntimeError('Use a completed disposable campaign before the destination-record chain')
merchant = presenters[0]
before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
          for p in (Path(u.Paths.project_saved_dir()) / 'SaveGames').rglob('*.sav')
          if not p.name.startswith('Coastal_' + SAVE + '_')}
prerequisites = [
    ('world.north_reach.harbour_log', (28200, 23000, 448)),
    ('world.north_reach.platform_signal', (38600, -14000, 898)),
    ('world.north_reach.powell_order', (25500, 4000, 398)),
    ('world.coastal_records.hallsands', (-6500, 22000, 192)),
]
rows, phase, prerequisite_index, deadline, next_time, phase_deadline = [], 'prerequisite', 0, time.monotonic()+180, 0, 0
pitch_path = '/Game/Coastal/M3/Merchant/UE5/Animation/AS_PitchBarter_Rt.AS_PitchBarter_Rt'
idle_path = '/Game/Coastal/M3/Merchant/UE5/Animation/AS_IdleBartering.AS_IdleBartering'
finished = False


def close_panel():
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport() and p.is_visible()]
    if len(panels) != 1:
        raise RuntimeError('Expected one painted journal')
    panels[0].call_method('Execute', args=('back',))


def animation():
    instance = merchant.mesh.get_anim_instance()
    asset = instance.get_animation_asset() if instance else None
    return asset.get_path_name() if asset else None


def interact(world_id, location, reread=False):
    actor = objects[world_id]
    move.stop_movement_immediately()
    pawn.set_actor_location(u.Vector(*location), False, True)
    if not bridge.preview_interaction(actor).can_interact:
        raise RuntimeError('Interaction unavailable: ' + world_id)
    expected = u.CoastalActionResult.ALREADY_APPLIED if reread else u.CoastalActionResult.APPLIED
    if bridge.try_interact(actor) != expected:
        raise RuntimeError('Interaction failed: ' + world_id)


def finish(error=None):
    global finished
    if finished:
        return
    finished = True
    u.unregister_slate_post_tick_callback(handle)
    unchanged = all(Path(p).is_file() and hashlib.sha256(Path(p).read_bytes()).hexdigest() == digest
                    for p, digest in before.items())
    REPORT.write_text(json.dumps({'passed': error is None and unchanged, 'error': error, 'rows': rows,
        'save_set': SAVE, 'preexisting_saves_preserved': unchanged,
        'scope': 'Scripted presentation PIE; no trade/economy, physical-device or package acceptance'}, indent=2))


def tick(_delta):
    global phase, prerequisite_index, next_time, phase_deadline
    try:
        now = time.monotonic()
        if now < next_time:
            return
        if now > deadline:
            raise RuntimeError('Merchant live check timed out in ' + phase)
        if phase == 'prerequisite':
            world_id, location = prerequisites[prerequisite_index]
            interact(world_id, location)
            phase, next_time = 'close_prerequisite', now+.4
        elif phase == 'close_prerequisite':
            if not ui.has_modal() or 'IDLE' not in str(merchant.presentation):
                raise RuntimeError('Prerequisite journal failed or unrelated record animated merchant')
            rows.append({'case': 'unrelated_record_keeps_merchant_idle',
                         'world_id': prerequisites[prerequisite_index][0], 'passed': True})
            close_panel(); prerequisite_index += 1
            phase = 'village' if prerequisite_index == len(prerequisites) else 'prerequisite'
            next_time = now+.5
        elif phase == 'village':
            if ui.has_modal():
                next_time = now+.1; return
            interact('world.coastal_records.village', (-1600, 15300, 2298))
            phase, next_time = 'journal_wait', now+.4
        elif phase == 'journal_wait':
            if not ui.has_modal() or 'AWAITING_JOURNAL_CLOSE' not in str(merchant.presentation):
                raise RuntimeError('Village interaction did not queue through its expected journal')
            rows.append({'case': 'authoritative_village_record_queues_until_journal_close', 'passed': True})
            close_panel(); phase, next_time = 'pitch', now+.5
        elif phase == 'pitch':
            if ui.has_modal():
                next_time = now+.1; return
            if 'PITCHING' not in str(merchant.presentation) or animation() != pitch_path:
                raise RuntimeError('Journal close did not start the selected offer pitch')
            clip = merchant.mesh.get_anim_instance().get_animation_asset()
            if not clip or clip.get_play_length() <= 0:
                raise RuntimeError('Selected pitch has no bounded imported duration')
            rows.append({'case': 'selected_pitch_clip_after_journal_close', 'passed': True, 'asset': animation()})
            phase_deadline = now + clip.get_play_length() + 2
            phase, next_time = 'idle', now+.2
        elif phase == 'idle':
            if 'PITCHING' in str(merchant.presentation) and now < phase_deadline:
                next_time = now+.1; return
            if 'IDLE' not in str(merchant.presentation) or animation() != idle_path:
                raise RuntimeError('Bounded pitch did not restore merchant idle')
            rows.append({'case': 'bounded_pitch_restores_idle', 'passed': True, 'asset': animation()})
            interact('world.coastal_records.village', (-1600, 15300, 2298), reread=True)
            phase, next_time = 'pause_queue', now+.4
        elif phase == 'pause_queue':
            if not ui.has_modal():
                raise RuntimeError('Idempotent village reread did not reopen journal')
            close_panel(); phase, next_time = 'pause_pitch', now+.5
        elif phase == 'pause_pitch':
            if 'PITCHING' not in str(merchant.presentation):
                raise RuntimeError('Reread did not produce a fresh bounded pitch')
            ui.open_pause(); phase, next_time = 'pause_cancel', now+.4
        elif phase == 'pause_cancel':
            if 'IDLE' not in str(merchant.presentation) or animation() != idle_path:
                raise RuntimeError('Unrelated pause did not cancel pitch to idle')
            rows.append({'case': 'unrelated_pause_cancels_pitch', 'passed': True})
            close_panel(); phase, next_time = 'reload', now+.5
        elif phase == 'reload':
            result = saves.load_campaign(u.Name(SAVE))
            if result not in (u.CoastalSaveResult.LOADED, u.CoastalSaveResult.RECOVERED_PREVIOUS):
                raise RuntimeError('Disposable campaign reload failed: ' + str(result))
            phase, next_time = ('epoch', now+1)
            rows.append({'case': 'campaign_reload_completed', 'passed': True})
        else:
            if 'IDLE' not in str(merchant.presentation) or animation() != idle_path:
                raise RuntimeError('Campaign epoch reload did not settle in idle')
            rows.append({'case': 'campaign_epoch_cleanup_returns_idle', 'passed': True})
            finish()
    except Exception as exc:
        finish(str(exc))


REPORT.write_text(json.dumps({'passed': False, 'status': 'running', 'save_set': SAVE}))
handle = u.register_slate_post_tick_callback(tick)
print('Merchant live check queued for ' + SAVE)
