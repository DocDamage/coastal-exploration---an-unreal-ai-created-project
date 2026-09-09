"""Read the weapon presentation and owned pistol bounds before authoring alignment."""
import json
from pathlib import Path
import unreal as u

world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
report = dict(directors=[], meshes=[])
for actor in u.GameplayStatics.get_all_actors_of_class(world, u.CoastalCombatDirector):
    report['directors'].append(dict(actor=actor.get_path_name(), mesh=str(actor.weapon_visual_asset),
        transform=str(actor.weapon_visual_transform), muzzle=str(actor.muzzle_offset)))
for data in u.AssetRegistryHelpers.get_asset_registry().get_assets_by_path('/Game/INVENTORY/Items/Resources/Pistol', True):
    if str(data.asset_class_path.asset_name) != 'StaticMesh':
        continue
    mesh = data.get_asset()
    bounds = mesh.get_bounds()
    report['meshes'].append(dict(path=mesh.get_path_name(), origin=str(bounds.origin),
        extent=str(bounds.box_extent), materials=[str(x.material_interface) for x in mesh.static_materials]))
destination = Path(__file__).resolve().parents[3] / 'local-evidence/m3-pistol-alignment-audit.json'
destination.write_text(json.dumps(report, indent=2))
print(json.dumps(report))
