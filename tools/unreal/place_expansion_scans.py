"""Append survey destinations to First Signal; preserve its existing campaign actors."""
import json
import shutil
from pathlib import Path
import unreal as u

level = u.get_editor_subsystem(u.LevelEditorSubsystem)
editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
world = editor.get_editor_world()
if world.get_name() != 'L_FirstSignal' or level.is_in_play_in_editor():
    raise RuntimeError('Expected First Signal editor world')
if list(u.EditorLoadingAndSavingUtils.get_dirty_map_packages()):
    raise RuntimeError('Preserve unsaved map edits before expansion')
evidence = Path('F:/coastline/local-evidence')
original = Path('F:/coastline/LocalHost/CoastalExploration/Content/Coastal/Maps/L_FirstSignal.umap')
backup = evidence / 'L_FirstSignal-before-destination-expansion.umap'
if not backup.exists():
    shutil.copy2(original, backup)
report = []
for name, position in [('Hallsands', (-19000, 22000, -2300)), ('Baelo', (8000, 23000, 200))]:
    tag = 'Coastal.Expansion.' + name
    existing = [a for a in actors.get_all_level_actors() if a.actor_has_tag(tag)]
    if existing:
        if len(existing) != 1:
            raise RuntimeError('Duplicate destination: ' + name)
        a = existing[0]
    else:
        sm = u.load_asset('/Game/Coastal/Expansion/' + name + '/SM_' + name)
        if not sm:
            raise RuntimeError('Import missing: ' + name)
        a = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(*position))
        a.set_actor_label('Hallsands — lost coastal village' if name == 'Hallsands' else 'Baelo Claudia — Roman town')
        a.set_editor_property('tags', [u.Name(tag)])
        a.set_folder_path('Coastal Expansion/' + name)
        c = a.static_mesh_component
        c.set_static_mesh(sm)
        c.set_collision_profile_name('BlockAll')
    bounds = a.get_actor_bounds(False)
    report.append({'destination': name, 'actor': a.get_path_name(), 'position': list(a.get_actor_location().to_tuple()),
                   'bounds_origin': list(bounds[0].to_tuple()), 'bounds_extent': list(bounds[1].to_tuple())})
if not level.save_current_level():
    raise RuntimeError('Expansion map save failed')
(evidence / 'm3-expansion-scan-placement.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
