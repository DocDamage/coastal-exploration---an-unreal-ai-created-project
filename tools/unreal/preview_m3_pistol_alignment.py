"""Preview assembled left-hand pistol in disposable PIE before persistent binding."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]

def configure_weapon():
    combat = pawn.get_component_by_class(u.CoastalCombatComponent)
    body = pawn.get_component_by_class(u.CoastalMutableCharacterComponent).get_generated_body()
    weapon = combat.get_transient_weapon()
    if not weapon:
        raise RuntimeError('Encounter weapon required')
    clip = u.load_asset('/Game/Coastal/Character/Animations/CA_AS_Pistol_Point_Front')
    pose = u.AnimPoseExtensions.get_anim_pose_at_time(clip, .99, u.AnimPoseEvaluationOptions())
    hand = u.AnimPoseExtensions.get_bone_pose(pose, 'hand_l', u.AnimPoseSpaces.WORLD)
    desired = u.Transform(location=hand.translation+u.Vector(0,6,3), rotation=u.Rotator(yaw=90))
    relative = u.MathLibrary.make_relative_transform(desired, hand)
    combat.set_editor_property('weapon_attach_socket', 'hand_l')
    weapon.attach_to_component(body, 'hand_l', u.AttachmentRule.SNAP_TO_TARGET,
        u.AttachmentRule.SNAP_TO_TARGET, u.AttachmentRule.KEEP_WORLD, False)
    visual = next(c for c in weapon.get_components_by_class(u.StaticMeshComponent) if c.get_name()=='CoastalWeaponVisual')
    visual.set_static_mesh(u.load_asset('/Game/Coastal/Activities/Combat/SM_CoastalPistol'))
    visual.set_relative_transform(relative,False,True)
    muzzle = next(c for c in weapon.get_components_by_class(u.SceneComponent) if c.get_name()=='Muzzle')
    muzzle_local = u.MathLibrary.transform_location(relative,u.Vector(14.581,0,4.45))
    muzzle.set_relative_location(muzzle_local,False,True)
    result = dict(socket='hand_l', location=list(relative.translation.to_tuple()),
        rotation=list(relative.rotation.rotator().to_tuple()), muzzle=list(muzzle_local.to_tuple()))
    (ROOT/'local-evidence/m3-pistol-alignment-preview.json').write_text(json.dumps(result,indent=2))

capture_name='m3-pistol-aligned-preview.png'
exec((Path(__file__).resolve().parent/'capture_m3_combat_animation.py').read_text(encoding='utf-8-sig'))
