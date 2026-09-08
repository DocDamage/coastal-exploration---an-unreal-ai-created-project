"""Bind the owned Morbid rest clip to a dry prison-cell rest point after native build."""
import json
import shutil
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
from m3_destination_authoring import Destination

d = Destination('morbid_shelter')
evidence = Path('F:/coastline/local-evidence')
host = Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).parent
backup_prefix = 'm3-morbid-ue58-before-' if 'LocalHost58' in str(host) else 'm3-morbid-before-'
bp_path = '/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter'
bp = u.load_asset(bp_path)
pawn_class = u.EditorAssetLibrary.load_blueprint_class(bp_path)
component = u.get_default_object(pawn_class).get_component_by_class(u.CoastalCampingActionComponent)
if not component:
    raise RuntimeError('Expected the existing native camping action owner')
paths = {
    'shelter_presentation_mesh': '/Game/MorbidMotions_Pack/Demo/Characters/Mannequins/Meshes/SKM_Quinn_Simple',
    'shelter_rest_animation': '/Game/MorbidMotions_Pack/Animations_Vol2/30fps/ANIM_idle_sleeping_A',
}
assets = {key: u.load_asset(path) for key, path in paths.items()}
if any(asset is None for asset in assets.values()):
    raise RuntimeError('The selected Morbid mesh/animation is missing')
if len({asset.get_editor_property('skeleton').get_path_name() for asset in assets.values()}) != 1:
    raise RuntimeError('Morbid mesh and animation must share their actual skeleton')
clip = assets['shelter_rest_animation']
if clip.get_editor_property('enable_root_motion') or clip.get_play_length() <= 0:
    raise RuntimeError('Shelter rest requires a finite non-root-motion clip')
# Preserve original map/Blueprint files before any authoring changes.
for relative, filename in (
    ('Coastal/Maps/L_FirstSignal.umap', backup_prefix + 'L_FirstSignal.umap'),
    ('ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset', backup_prefix + 'player.uasset'),
):
    backup = evidence / filename
    if not backup.exists():
        shutil.copy2(host / 'Content' / relative, backup)
bp.modify()
component.modify()
for key, asset in assets.items():
    component.set_editor_property(key, asset)
component.set_editor_property('shelter_actor_tag', 'Coastal.ShelterRest')
u.BlueprintEditorLibrary.compile_blueprint(bp)
if not u.EditorAssetLibrary.save_loaded_asset(bp, False):
    raise RuntimeError('Could not save player shelter bindings')

# One empty side of the south-east prison cell, away from its bed and toilet.
# Marker is standing capsule centre; supplied skeletal pose lowers onto the mat.
floor_z = 400.0
half_height = u.get_default_object(pawn_class).capsule_component.get_unscaled_capsule_half_height()
marker = d.actor(u.TargetPoint, 'Prison cell rest', (21150, 28430, floor_z + half_height + 2), yaw=0)
tags = list(marker.tags)
if u.Name('Coastal.ShelterRest') not in tags:
    tags.append(u.Name('Coastal.ShelterRest'))
marker.tags = tags
marker.set_actor_hidden_in_game(True)
mat = d.mesh('Prison rest mat', '/Engine/BasicShapes/Cube', (21150, 28430, floor_z + 1),
             scale=(2.2, 1.0, .015), collision=False,
             materials=['/Game/Coastal/M2/Materials/M_RadioCase'])
d.sign('Rest direction', 'Shelter rest', (21430, 28730, 570), yaw=-90)
rows = d.save()
report = {'paths': paths, 'clip_seconds': clip.get_play_length(),
          'marker': list(marker.get_actor_location().to_tuple()), 'actors': rows,
          'scope': 'One short player rest action using Morbid animation; no healing or time-skip claim',
          'runtime_verified': False}
(evidence / 'm3-morbid-shelter-authoring.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps({'authored': True, 'marker': report['marker'], 'clip_seconds': report['clip_seconds']}))
