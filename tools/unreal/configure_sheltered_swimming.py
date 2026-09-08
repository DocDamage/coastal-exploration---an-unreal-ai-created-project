"""Bind inspected swim assets and one real physics-volume brush after native build."""
import json
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
from m3_destination_authoring import Destination

d = Destination('sheltered_swimming')
bp = u.load_asset('/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter')
pawn_class = u.EditorAssetLibrary.load_blueprint_class('/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter')
component = u.get_default_object(pawn_class).get_component_by_class(u.CoastalSwimmingComponent)
if not component:
    raise RuntimeError('Rebuild the host with the native swimming component before authoring')
paths = {
    'presentation_mesh': '/Game/SwimmingAnimationPack/Demo/Characters/Mannequins/Meshes/SKM_Quinn_Simple',
    'idle_animation': '/Game/SwimmingAnimationPack/Animations/Mannequin_UE5/InPlace/anim_swim_idle_A',
    'forward_animation': '/Game/SwimmingAnimationPack/Animations/Mannequin_UE5/InPlace/anim_swim_forward_A',
}
assets = {key: u.load_asset(path) for key, path in paths.items()}
if any(asset is None for asset in assets.values()):
    raise RuntimeError('Inspected swimming asset is unavailable')
if len({asset.get_editor_property('skeleton').get_path_name() for asset in assets.values()}) != 1:
    raise RuntimeError('Swimming mesh and clips do not share a skeleton')
bp.modify()
component.modify()
for key, asset in assets.items():
    component.set_editor_property(key, asset)
u.BlueprintEditorLibrary.compile_blueprint(bp)
if not u.EditorAssetLibrary.save_loaded_asset(bp, False):
    raise RuntimeError('Player blueprint swimming configuration did not save')

zone = d.actor(u.CoastalSwimmingZone, 'Sheltered basin', (11700, -1650, -270))
zone.set_editor_property('water_surface_z', -90.0)
zone.set_editor_property('max_recovery_exemption_depth_cm', 220.0)
zone.set_editor_property('water_volume', True)
zone.set_editor_property('physics_on_contact', True)
zone.set_editor_property('priority', 10)
if not u.CoastalVendorInspection.build_swimming_zone_box(zone, u.Vector(1200, 3700, 360)):
    raise RuntimeError('Native engine brush creation/validation failed')
brush = zone.brush_component
if (not zone.is_authored_correctly()
        or str(brush.get_collision_profile_name()) != 'OverlapAllDynamic'
        or brush.get_collision_enabled() != u.CollisionEnabled.QUERY_ONLY
        or not brush.get_editor_property('generate_overlap_events')
        or brush.get_collision_response_to_channel(u.CollisionChannel.ECC_PAWN) != u.CollisionResponseType.ECR_OVERLAP):
    raise RuntimeError('Swimming zone readback is invalid')
rows = d.save()
Path('F:/coastline/local-evidence/m3-swimming-configuration.json').write_text(json.dumps({
    'assets': paths, 'zone': rows, 'box_size': [1200, 3700, 360], 'water_surface_z': -90,
    'scope': 'Saved editor configuration; gameplay acceptance recorded separately',
}, indent=2), encoding='utf-8')
