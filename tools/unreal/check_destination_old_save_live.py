"""Load an old seven-record save read-only and verify optional discoveries reset unread."""
import hashlib
import json
import time
from pathlib import Path
import unreal as u

save_set = str(globals().get('save_set', ''))
if not save_set or len(save_set) > 64:
    raise RuntimeError('Pass the exact pre-expansion save_set to inspect')
report_path = (Path(__file__).resolve().parents[3] / 'local-evidence/m3-destination-old-save-live-ue58.json')
save_dir = Path(u.Paths.project_saved_dir()) / 'SaveGames'
slots = [save_dir / f'Coastal_{save_set}_{suffix}.sav' for suffix in ('A', 'B')]
existing = [path for path in slots if path.exists()]
if not existing:
    raise RuntimeError('The requested pre-expansion save pair does not exist in this host')
before_hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in existing}
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
saves_found = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name()]
if not world or len(saves_found) != 1 or saves_found[0].has_active_campaign():
    raise RuntimeError('Run in fresh PIE at the campaign screen; no live campaign may be replaced')
saves = saves_found[0]
optional_ids = [
    'world.north_reach.harbour_log', 'world.north_reach.platform_signal',
    'world.north_reach.powell_order', 'world.coastal_records.hallsands',
    'world.coastal_records.village', 'world.coastal_records.baelo',
    'world.coastal_records.prison', 'world.outer_coast.atlantis', 'world.outer_coast.station']
objects = {str(a.world_id): a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalWorldObject)}
if any(world_id not in objects for world_id in optional_ids):
    raise RuntimeError('The expanded map is missing an approved optional destination record')
result = saves.load_campaign(u.Name(save_set))
if result not in (u.CoastalSaveResult.LOADED, u.CoastalSaveResult.RECOVERED_PREVIOUS):
    raise RuntimeError('Old save migration was rejected: ' + str(result))
generation = saves.get_generation()
started = time.monotonic()
finished = False

def finish(error=None):
    global finished
    if finished:
        return
    finished = True
    u.unregister_slate_post_tick_callback(handle)
    after_hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in existing}
    report_path.write_text(json.dumps({
        'passed': error is None, 'error': error, 'save_set': save_set,
        'load_result': str(result), 'generation': generation,
        'optional_records_reset_unread': error is None,
        'save_hashes_unchanged': before_hashes == after_hashes,
        'before_hashes': before_hashes, 'after_hashes': after_hashes,
        'scope': 'Read-only old-save load in fresh PIE; no migration write requested'}, indent=2), encoding='utf-8')

def tick(delta):
    try:
        if time.monotonic() - started < 1.5:
            return
        journal = {str(entry) for entry in saves.get_journal()}
        if saves.get_generation() != generation:
            raise RuntimeError('Load alone unexpectedly advanced the save generation')
        if any(objects[world_id].is_active() for world_id in optional_ids):
            raise RuntimeError('Missing optional record leaked active state across load')
        if any(entry.startswith(('journal.coastal_records.', 'journal.outer_coast.'))
               or entry in {'journal.north_reach.harbour_log',
                            'journal.north_reach.platform_signal',
                            'journal.north_reach.powell_order'} for entry in journal):
            raise RuntimeError('Old save acquired forged destination journal progress')
        after = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in existing}
        if after != before_hashes:
            raise RuntimeError('Loading the old save wrote to its A/B files')
        finish()
    except Exception as exc:
        finish(str(exc))

handle = u.register_slate_post_tick_callback(tick)
print('Old-save destination migration check queued for ' + save_set)
