"""Run both investigation chains in a completed disposable First Signal PIE campaign."""
import json
import time
from pathlib import Path
import unreal as u

REPORT = Path('F:/coastline/local-evidence/m3-destination-quests-live-ue58.json')
REPORT.write_text(json.dumps({'passed': False, 'status': 'starting'}), encoding='utf-8')
save_set = str(globals().get('save_set', ''))
if not save_set.startswith('coastal_test_'):
    raise RuntimeError('Pass an active completed disposable save_set beginning coastal_test_')

steps = [
    ('world.north_reach.harbour_log', 'journal.north_reach.harbour_log',
     (28200, 23000, 448), 'Read harbour inspection log', 'NORTH REACH'),
    ('world.north_reach.platform_signal', 'journal.north_reach.platform_signal',
     (38600, -14000, 898), 'Check the signal station', 'NORTH REACH'),
    ('world.north_reach.powell_order', 'journal.north_reach.powell_order',
     (25500, 4000, 398), 'Read the work order', 'COASTAL RECORDS'),
    ('world.coastal_records.hallsands', 'journal.coastal_records.hallsands',
     (-6500, 22000, 192), 'Inspect the evacuation marker', 'COASTAL RECORDS'),
    ('world.coastal_records.village', 'journal.coastal_records.village',
     (-1600, 15300, 2298), 'Read the village register', 'COASTAL RECORDS'),
    ('world.coastal_records.baelo', 'journal.coastal_records.baelo',
     (8000, 20000, 526), 'Inspect the survey tablet', 'COASTAL RECORDS'),
    ('world.coastal_records.prison', 'journal.coastal_records.prison',
     (21600, 29000, 498), 'Read the prison duty record', 'COASTAL RECORDS'),
]
try:
    world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
    pawn = u.GameplayStatics.get_player_character(world, 0)
    pc = u.GameplayStatics.get_player_controller(world, 0)
    movement = pawn.get_component_by_class(u.CharacterMovementComponent)
    bridge = pawn.get_component_by_class(u.CoastalInteractionBridge)
    ui = pc.get_component_by_class(u.CoastalUISessionComponent)
    saves_found = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
                   if 'UEDPIE_' in s.get_path_name() and s.has_active_campaign()]
    missions = [m for m in u.ObjectIterator(u.FirstSignalComponent)
                if 'UEDPIE_' in m.get_path_name()]
    if len(saves_found) != 1 or len(missions) != 1:
        raise RuntimeError('Expected one PIE save coordinator and First Signal owner')
    saves, mission = saves_found[0], missions[0]
    if not bridge or not ui or not movement:
        raise RuntimeError('Expected the configured interaction, UI and CharacterMovement owners')
    objects = {str(a.world_id): a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalWorldObject)}
    if str(saves.get_active_save_set()) != save_set or not mission.export_snapshot().message_heard:
        raise RuntimeError('Expected the named active campaign with First Signal complete')
    if any(journal in [str(v) for v in saves.get_journal()] for _, journal, *_ in steps):
        raise RuntimeError('Use a completed disposable campaign before either destination chain has started')
    if set(world_id for world_id, *_ in steps) - set(objects):
        raise RuntimeError('One or more authored destination records are missing in PIE')
except Exception as exc:
    REPORT.write_text(json.dumps({'passed': False, 'status': 'preflight_failed', 'error': str(exc)}))
    raise

index = 0
phase = 'place'
deadline = time.monotonic() + 90
next_time = 0
rows = []
generation_before = saves.get_generation()
finished = False

def close_journal():
    panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
              if p.is_in_viewport()]
    if len(panels) != 1:
        raise RuntimeError('Expected exactly one immediate journal panel')
    panels[0].call_method('Execute', args=('back',))

def finish(error=None):
    global finished
    if finished:
        return
    finished = True
    u.unregister_slate_post_tick_callback(handle)
    REPORT.write_text(json.dumps({'passed': error is None, 'error': error, 'save_set': save_set,
                                  'generation_before': generation_before,
                                  'generation_after': saves.get_generation(), 'rows': rows,
                                  'scope': 'Scripted PIE interaction/save/reload; physical-device traversal remains separate'},
                                 indent=2), encoding='utf-8')

def tick(delta):
    global index, phase, next_time, deadline
    try:
        now = time.monotonic()
        if now < next_time:
            return
        if now > deadline:
            raise RuntimeError('Timed out during destination step ' + str(index) + ' phase ' + phase)
        if index < len(steps):
            world_id, journal_id, arrival, action, title_after = steps[index]
            actor = objects[world_id]
            if phase == 'place':
                if ui.has_modal():
                    raise RuntimeError('Unexpected modal before destination interaction')
                movement.stop_movement_immediately()
                if not pawn.set_actor_location(u.Vector(*arrival), False, True):
                    raise RuntimeError('Could not place pawn at verified dry arrival')
                phase = 'interact'; next_time = now + .35; return
            if phase == 'interact':
                offer = bridge.preview_interaction(actor)
                if not offer.visible or not offer.can_interact or str(offer.action_text) != action:
                    raise RuntimeError('Contextual offer failed for ' + world_id + ': ' + str(offer.detail))
                if bridge.try_interact(actor) != u.CoastalActionResult.APPLIED:
                    raise RuntimeError('Interaction was not applied for ' + world_id)
                phase = 'journal'; next_time = now + .35; return
            if phase == 'close_journal':
                # The native UI intentionally rejects commands before the
                # panel has really painted. Wait through throttled frames.
                if ui.has_modal():
                    close_journal()
                    next_time = now + .1
                    return
                index += 1; phase = 'place'; next_time = now + .1; return
            journal = [str(value) for value in saves.get_journal()]
            if journal_id not in journal or not actor.is_active() or not ui.has_modal():
                raise RuntimeError('Discovery did not atomically update state and open its journal entry')
            if str(ui.campaign_title()) != title_after:
                raise RuntimeError('HUD campaign title did not advance from journal progress')
            rows.append({'step': index, 'world_id': world_id, 'journal_id': journal_id,
                         'action': action, 'objective_after': str(ui.objective()),
                         'title_after': str(ui.campaign_title())})
            phase = 'close_journal'; next_time = now + .1; return
        if phase == 'place':
            result = saves.save_now()
            if result != u.CoastalSaveResult.SAVED:
                raise RuntimeError('Explicit quest checkpoint save failed: ' + str(result))
            phase = 'reload'; next_time = now + .5; deadline = now + 30; return
        if phase == 'reload':
            result = saves.load_campaign(u.Name(save_set))
            if result not in (u.CoastalSaveResult.LOADED, u.CoastalSaveResult.RECOVERED_PREVIOUS):
                raise RuntimeError('Saved destination chain did not reload: ' + str(result))
            phase = 'verify'; next_time = now + .5; return
        journal = [str(value) for value in saves.get_journal()]
        if any(journal_id not in journal or not objects[world_id].is_active()
               for world_id, journal_id, *_ in steps):
            raise RuntimeError('Reload lost destination world/journal progress')
        if str(ui.campaign_title()) != 'COASTAL RECORDS' or 'complete' not in str(ui.objective()).lower():
            raise RuntimeError('Completed campaign presentation did not survive reload')
        finish()
    except Exception as exc:
        finish(str(exc))

handle = u.register_slate_post_tick_callback(tick)
print('Destination quest live check queued for ' + save_set)
