"""Author project-owned IK rigs for the existing player and staged Mutable human."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
expected = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != expected.resolve():
    raise RuntimeError('Expected independent host')
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('End PIE before authoring')
base = '/Game/Coastal/Character/Rigs'
tools = u.AssetToolsHelpers.get_asset_tools()
source = u.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple')
target = u.load_asset('/Game/Character/Body/SK_BaseBody')
report = dict(rigs=[], retargeters=[], sources=[])


def rig(name, mesh):
    asset = u.load_asset(base + '/' + name) if u.EditorAssetLibrary.does_asset_exist(base + '/' + name) else tools.create_asset(name, base, u.IKRigDefinition, u.IKRigDefinitionFactory())
    controller = u.IKRigController.get_controller(asset)
    if not controller.set_skeletal_mesh(mesh):
        raise RuntimeError('Unable to set mesh for ' + name)
    characterized = controller.apply_auto_generated_retarget_definition()
    if not characterized:
        raise RuntimeError('Automatic retarget definition failed for ' + name)
    fbik = controller.apply_auto_fbik()
    chains = [str(c.chain_name) for c in controller.get_retarget_chains()]
    if len(chains) < 5:
        raise RuntimeError('Incomplete humanoid chain mapping: ' + name)
    u.EditorAssetLibrary.save_loaded_asset(asset)
    report['rigs'].append(dict(path=asset.get_path_name(), mesh=mesh.get_path_name(), auto_definition=characterized, auto_fbik=fbik, chains=chains))
    return asset


source_rig = rig('IK_CoastalPlayer', source)
target_rig = rig('IK_CoastalMutable', target)
name = 'RTG_PlayerToMutable'
asset = u.load_asset(base + '/' + name) if u.EditorAssetLibrary.does_asset_exist(base + '/' + name) else tools.create_asset(name, base, u.IKRetargeter, u.IKRetargetFactory())
controller = u.IKRetargeterController.get_controller(asset)
controller.set_ik_rig(u.RetargetSourceOrTarget.SOURCE, source_rig)
controller.set_ik_rig(u.RetargetSourceOrTarget.TARGET, target_rig)
controller.add_default_ops()
controller.auto_map_chains(u.AutoMapChainType.FUZZY, True)
controller.auto_align_all_bones(u.RetargetSourceOrTarget.TARGET)
u.EditorAssetLibrary.save_loaded_asset(asset)
report['retargeters'].append(asset.get_path_name())
for path in ['/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple',
             '/Game/CampingAnimations/Demo/Mannequins/Meshes/SKM_Quinn_Simple',
             '/Game/MorbidMotions_Pack/Demo/Characters/Mannequins/Meshes/SKM_Quinn_Simple',
             '/Game/SwimmingAnimationPack/Demo/Characters/Mannequins/Meshes/SKM_Quinn_Simple']:
    mesh = u.load_asset(path)
    compatible = u.IKRigController.get_controller(source_rig).is_skeletal_mesh_compatible(mesh)
    if not compatible:
        raise RuntimeError('Existing action source requires another rig: ' + path)
    report['sources'].append(dict(mesh=path, skeleton=mesh.skeleton.get_path_name(), compatible=compatible))
(ROOT / 'local-evidence/m3-character-retarget-rigs.json').write_text(json.dumps(report, indent=2))
print('CHARACTER_RIGS', report)
