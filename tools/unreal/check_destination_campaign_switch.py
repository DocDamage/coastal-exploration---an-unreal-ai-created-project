"""Verify optional state clears across real coordinator campaign replacements."""
import hashlib
import json
from pathlib import Path
import unreal as u

completed = str(globals().get('completed_save_set', ''))
old = 'coastal_test_m3_swimming_0908a'
if not completed.startswith('coastal_test_'):
    raise RuntimeError('Use an existing disposable completed quest campaign')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
saves = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator) if 'UEDPIE_' in s.get_path_name()]
if len(saves) != 1 or str(saves[0].get_active_save_set()) != completed:
    raise RuntimeError('Load the completed disposable quest campaign first')
saves = saves[0]
objects = [a for a in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalWorldObject)
           if str(a.world_id).startswith(('world.north_reach.', 'world.coastal_records.'))]
if len(objects) != 7 or not all(a.is_active() for a in objects):
    raise RuntimeError('All seven destination records must already be completed')
directory = Path(u.Paths.project_saved_dir()) / 'SaveGames'
def hashes():
    return {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in directory.glob('*.sav')}
before = hashes()
rows = []
for name, active in ((old, False), (completed, True)):
    result = saves.load_campaign(name)
    if result != u.CoastalSaveResult.LOADED or any(a.is_active() != active for a in objects):
        raise RuntimeError('Optional world state did not match the replaced campaign')
    journal = [str(j) for j in saves.get_journal()]
    expected = [str(a.journal_entry) for a in objects]
    if any((j in journal) != active for j in expected):
        raise RuntimeError('Optional journal state leaked across campaign replacement')
    rows.append({'save_set': name, 'all_destinations_active': active,
                 'generation': saves.get_generation()})
if before != hashes():
    raise RuntimeError('Campaign replacement unexpectedly wrote a save file')
Path('F:/coastline/local-evidence/m3-destination-campaign-switch-ue58.json').write_text(
    json.dumps({'passed': True, 'rows': rows, 'all_save_hashes_unchanged': True}, indent=2), encoding='utf-8')
print('Old/completed campaign switching resets and restores all seven world/journal pairs')
