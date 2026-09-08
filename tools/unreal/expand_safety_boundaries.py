"""Move existing boundary actors beyond the expanded routes, preserving sea recovery."""
import json
from pathlib import Path
import unreal as u

level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
if level.is_in_play_in_editor() or editor.get_editor_world().get_name() != 'L_FirstSignal':
    raise RuntimeError('Expected saved First Signal editor world outside PIE')
actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
by_name = {a.get_actor_label(): a for a in actors if isinstance(a, u.CoastalSafetyVolume)}
# All authored routes fit inside x[-35000,46000], y[-21000,35000].
# Keep boundary walls outside scans, worksites, and their connecting approaches.
layout = {
    'West boundary': ((-35000, 7000, 1500), (300, 28300, 5000)),
    'East boundary': ((46000, 7000, 1500), (300, 28300, 5000)),
    'North boundary': ((5500, 35000, 1500), (40800, 300, 5000)),
    'Below boundary': ((5500, 7000, -4200), (40800, 28300, 200)),
}
rows = []
for label, (location, extent) in layout.items():
    actor = by_name[label]
    rows.append({'name': label, 'before': list(actor.get_actor_location().to_tuple()),
                 'location': location, 'extent': extent})
    actor.set_actor_location(u.Vector(*location), False, True)
    actor.get_editor_property('bounds').set_box_extent(u.Vector(*extent), False)
# Deep sea already covers this full area; its upper surface remains at -120cm.
# It continues to catch southward departures and falls from the sea-platform route.
sea = by_name['Deep sea']
rows.append({'name': 'Deep sea', 'unchanged_location': list(sea.get_actor_location().to_tuple()),
             'unchanged_extent': list(sea.get_editor_property('bounds').get_scaled_box_extent().to_tuple())})
if not level.save_current_level():
    raise RuntimeError('Expanded boundary save failed')
Path('F:/coastline/local-evidence/m3-expansion-boundaries.json').write_text(json.dumps(rows, indent=2))
print(json.dumps(rows))
