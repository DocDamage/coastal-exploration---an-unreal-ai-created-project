"""Record actual installed activity animations and geometry in the independent host."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
expected = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != expected.resolve():
    raise RuntimeError('Independent host required')
registry = u.AssetRegistryHelpers.get_asset_registry()
roots = ['/Game/MotoInteractionAnims', '/Game/CampingAnimations', '/Game/FreeAnimationSet',
         '/Game/Free_Crawl_Animation', '/Game/FreeLadderAnimationSet', '/Game/ShipAndSea']
rows = []
for root in roots:
    for data in registry.get_assets_by_path(root, True):
        if str(data.asset_class_path.asset_name) not in ('AnimSequence', 'StaticMesh', 'SkeletalMesh'):
            continue
        asset = data.get_asset()
        row = dict(path=asset.get_path_name(), kind=asset.get_class().get_name())
        if isinstance(asset, u.AnimSequence):
            row.update(length=asset.get_play_length(), skeleton=asset.get_editor_property('skeleton').get_path_name())
        rows.append(row)
target = u.load_asset('/Game/Character/Body/SK_BaseBody')
result = dict(assets=rows, target_skeleton=target.get_editor_property('skeleton').get_path_name(),
              target_bones=[str(n) for n in u.AnimationLibrary.get_animation_track_names(
                  u.load_asset('/Game/Coastal/Character/Animations/CA_AS_Get_Hit_Front'))])
(ROOT / 'local-evidence/m3-activity-assets.json').write_text(json.dumps(result, indent=2))
print(json.dumps(dict(asset_count=len(rows), target_skeleton=result['target_skeleton'], target_bones=result['target_bones'])))
