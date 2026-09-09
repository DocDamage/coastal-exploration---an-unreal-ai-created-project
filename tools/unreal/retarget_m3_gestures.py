"""Retarget owned UE4 gestures into the generated human's skeleton and bind the emote cycle."""
import json
import shutil
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
project = ROOT/'LocalHost58/CoastalExploration'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (project/'CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('End PIE first')
base = '/Game/Coastal/Character'
tools = u.AssetToolsHelpers.get_asset_tools()
source = u.load_asset('/Game/RamsterZ_FreeAnims_Volume1/Demo/Mannequin/Character/Mesh/SK_Mannequin')
target = u.load_asset('/Game/Character/Body/SK_BaseBody')
rig_path = base+'/Rigs/IK_Ramster'
if u.EditorAssetLibrary.does_asset_exist(rig_path):
    rig = u.load_asset(rig_path)
else:
    rig = tools.create_asset('IK_Ramster',base+'/Rigs',u.IKRigDefinition,u.IKRigDefinitionFactory())
    controller = u.IKRigController.get_controller(rig)
    if not controller.set_skeletal_mesh(source) or not controller.apply_auto_generated_retarget_definition():
        raise RuntimeError('Cannot characterize gesture source')
    controller.apply_auto_fbik()
    u.EditorAssetLibrary.save_loaded_asset(rig)
retarget_path = base+'/Rigs/RTG_RamsterToMutable'
if u.EditorAssetLibrary.does_asset_exist(retarget_path):
    retarget = u.load_asset(retarget_path)
else:
    retarget = tools.create_asset('RTG_RamsterToMutable',base+'/Rigs',u.IKRetargeter,u.IKRetargetFactory())
    controller = u.IKRetargeterController.get_controller(retarget)
    controller.set_ik_rig(u.RetargetSourceOrTarget.SOURCE,rig)
    controller.set_ik_rig(u.RetargetSourceOrTarget.TARGET,u.load_asset(base+'/Rigs/IK_CoastalMutable'))
    controller.add_default_ops()
    controller.auto_map_chains(u.AutoMapChainType.FUZZY,True)
    controller.auto_align_all_bones(u.RetargetSourceOrTarget.TARGET)
    u.EditorAssetLibrary.save_loaded_asset(retarget)
definition = u.load_asset(base+'/DA_CoastalCharacter')
backup = ROOT/'local-evidence/m3-activities-asset-backups/DA_CoastalCharacter-before-gestures.uasset'
if not backup.exists():
    shutil.copy2(project/'Content/Coastal/Character/DA_CoastalCharacter.uasset',backup)
actions = dict(definition.get_editor_property('actions'))
sources = [('emote_'+str(i),'SillyGesture/SillyGesture0'+str(i)) for i in range(1,5)]
sources.append(('idle_variation_relaxed','H2H/Standing_Idle'))
rows=[]
for cue, relative in sources:
    path='/Game/RamsterZ_FreeAnims_Volume1/AnimationSequence/'+relative
    output=base+'/Animations/CA_'+relative.split('/')[-1]
    if u.EditorAssetLibrary.does_asset_exist(output):
        clip=u.load_asset(output)
    else:
        inputs=u.IKRetargetBatchOperationInputs()
        for key,value in dict(assets_to_retarget=[u.EditorAssetLibrary.find_asset_data(path)],source_mesh=source,
            target_mesh=target,ik_retarget_asset=retarget,prefix='CA_',target_path=base+'/Animations',
            include_referenced_assets=False,overwrite_existing_files=False).items():
            inputs.set_editor_property(key,value)
        result=u.IKRetargetBatchOperation.run_batch_retarget(inputs)
        if len(result)!=1:
            raise RuntimeError('Expected one output for '+cue)
        clip=result[0].get_asset()
    if clip.get_editor_property('skeleton')!=target.get_editor_property('skeleton'):
        raise RuntimeError('Unexpected gesture skeleton')
    clip.set_editor_property('enable_root_motion',False)
    clip.set_editor_property('force_root_lock',True)
    u.AnimationLibrary.remove_all_animation_notify_tracks(clip)
    u.EditorAssetLibrary.save_loaded_asset(clip)
    action=u.CoastalCharacterAction()
    duration=min(5.0,clip.get_play_length())
    action.set_editor_property('clip',clip)
    action.set_editor_property('duration_seconds',duration)
    action.set_editor_property('upper_body',False)
    actions[cue]=action
    rows.append(dict(cue=cue,source=path,output=clip.get_path_name(),duration=duration))
definition.set_editor_property('actions',actions)
definition.set_editor_property('emotes',[cue for cue,_ in sources if cue.startswith('emote_')])
definition.set_editor_property('idle_variations',['idle_variation_relaxed'])
if not u.EditorAssetLibrary.save_loaded_asset(definition):
    raise RuntimeError('Definition save failed')
(ROOT/'local-evidence/m3-gesture-retarget.json').write_text(json.dumps(rows,indent=2))
print(json.dumps(rows))
