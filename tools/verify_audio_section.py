"""M1.8 source/spec guardrails; deliberately not an Unreal/audio-device test."""
from __future__ import annotations
import hashlib
import json
import re
from pathlib import Path
from collections.abc import Callable

def verify(root: Path, source: Path, descriptor: dict, automation_count: int, check: Callable[[bool, str], None]) -> None:
    spec = json.loads((root/'data/m1_8_audio_spec.json').read_text())
    deps = json.loads((root/'data/m1_8_dependency_register.json').read_text())
    ledger = json.loads((root/'data/m1_8_audio_acceptance.json').read_text())
    native = (source/'Private/CoastalAudioOptionsComponent.cpp').read_text()
    header = (source/'Public/CoastalAudioOptionsComponent.h').read_text()
    rules = (source/'Public/Core/AudioOptionsRules.h').read_text()
    options = (source/'Public/Core/PlayerOptionsRules.h').read_text()
    codec = (source/'Public/Core/OptionsPersistence.h').read_text()
    session = (source/'Public/Core/OptionsSession.h').read_text()
    ui = (source/'Private/CoastalUIOptions.cpp').read_text()
    ui_life = (source/'Private/CoastalUISessionComponent.cpp').read_text()
    for name,obj in [('spec',spec),('dependencies',deps),('acceptance',ledger)]:
        check(obj['schema_version']==1,'M1.8 document schema: '+name)
    check(deps['plugin_version']=='0.1.8-m1-audio-options-source','Retained M1.8 historical version')
    check(deps['unreal_editor_automation_tests_total']==30 and automation_count>=30,'Retained M1.8 native inventory remains historical; current exact count checked separately')
    for key,value in deps.items():
        if isinstance(value,bool): check(not value,'No unsupported local native evidence claim: '+key)
    check(all(deps[k] is None for k in ('host_project','engine_version','agis_version','hyper_version')),'No invented host/vendor details')
    tests=ledger['tests']; ids=[t['id'] for t in tests]
    check(len(tests)==36 and len(ids)==len(set(ids)),'36 unique native acceptance cases')
    for t in tests:
        check(t['result']=='not_run' and t['evidence'] is None,'Native case remains unrun: '+t['id'])
    for field in ('master_percent','ambience_percent','effects_percent','radio_percent'):
        check(spec['options'][field]=={'default':100,'min':0,'max':100,'step':5},'Volume design bounds: '+field)
    for field in ('masterPercent','ambiencePercent','effectsPercent','radioPercent'):
        check(field+' = 100' in options and 'ValidVolumePercent(p.'+field+')' in options,'Bounded default volume: '+field)
        check(field in codec and field in ui,'Codec/UI field connected: '+field)
    check('OptionFieldCount = 10' in options,'Ten fields, existing indices retained')
    check('AudioOptionFadeSeconds = 0.1' in rules and spec['routing']['apply_fade_seconds']==0.1,'Original apply fade setting')
    check('bool bUseDedicatedSoundClasses = false' in header and not spec['routing']['default_opt_in'],'No implied sound routing')
    check(all(v is None for v in spec['routing']['class_paths'].values()),'No fabricated audio asset paths')
    for token in ('coastal::ValidAudioClasses(Info)','coastal::BuildAudioGains','MixState.Begin','MixState.NeedsUpdate','MixState.Submitted','MixState.Release'):
        check(token in native,'Native uses shared tested helper: '+token)
    for token in ('Class->ParentClass == nullptr','Class->ChildClasses.IsEmpty()','Class->PassiveSoundMixModifiers.IsEmpty()',
                  'World->GetNumPlayerControllers() != 1','World->GetNetMode() != NM_Standalone','World->IsGameWorld()',
                  'World->AllowAudioPlayback()','World->GetAudioDevice()','Owners.Num() != 1','IsComponentTickEnabled()'):
        check(token in native,'Native audio prerequisite: '+token)
    check('FAudioDeviceHandle Device' in header and 'CurrentDevice.GetDeviceID() == Device.GetDeviceID()' in native,'Cleanup retains original device identity')
    check(native.count('PushSoundMixModifier(')==1 and native.count('PopSoundMixModifier(')==1,'One guarded push and one guarded pop site')
    check(native.index('OwnedMix->SoundClassEffects.Add')<native.index('Device->PushSoundMixModifier'),'Startup gains seeded before push')
    check('OwnedMix->FadeInTime = 0' in native and 'OwnedMix->FadeOutTime = 0' in native,'No deliberate full-volume startup or delayed cleanup')
    check('Adjuster.bApplyToChildren = false' in native and 'AudioOptionFadeSeconds), false' in native,'Independent class-only overrides')
    check('Device->SetSoundMixClassOverride' in native and 'Device->PopSoundMixModifier' in native,'Real Engine device API, no substitute mixer')
    for forbidden in ('ClearSoundMixModifiers(', 'SetBaseSoundMix(', 'SetDefaultBaseSoundMix(', 'SetTransientMasterVolume(',
                      'SetTransientPrimaryVolume(', 'Properties.Volume =', 'Properties.Pitch =', 'PlaySound', 'SpawnSound',
                      'TryCollectWorldItem(', 'TryCommitRequirements(', 'TryTransfer(', 'RestoreInventory(',
                      'SaveNow(', 'LoadCampaign(', 'StartNewCampaign(', 'FinishRadioTransmission(', 'AcknowledgeTranscript(',
                      'ClearAllMappings(', 'SetInputMode', 'SetPause(', 'SetIgnoreMoveInput(', 'DeleteGameInSlot('):
        check(forbidden not in native,'Audio component cannot take unrelated ownership: '+forbidden)
    check('AudioOwners[0]->InitializeOptions(Options)' in ui and 'Controller->GetComponents<UCoastalAudioOptionsComponent>' in ui,'UI initializes one optional controller owner')
    check('AudioOptions->ReleaseOptions()' in ui_life and 'AudioOptions->LastDetail' in ui_life,'UI cleanup and diagnostics connected')
    for token in ('coastal::IsAudioOption(Field)','AudioOptions->IsAudioReady()','coastal::CopyAudioOptions(OptionsDraft, Defaults)',
                  '!coastal::SameAudioOptions(OptionsDraft, Live)','AudioOptions->ApplyAudioOptions()'):
        check(token in ui,'Audio draft/availability/apply safeguard: '+token)
    check(ui.index('const bool Applied =')<ui.index('AudioOptions->ApplyAudioOptions()'),'No audio apply before preference operation result')
    p=spec['persistence']
    check(p['read_schemas']==[1,2,3] and p['write_schema']==3 and not p['auto_write_on_load'],'Read old formats; upgrade explicitly only')
    check('OptionsPayloadSize = 60' in codec and 'OptionsSchema = 3' in codec and p['encoded_bytes']==80,'Current encoding size/version')
    for token in ('schema != 1 && schema != 2 && schema != OptionsSchema','schema == 2 ? 44 : OptionsPayloadSize',
                  'schema >= 2 && at(40) > 1','schema >= 2 && at(40) == 1','at(44) > 100','at(48) > 100','at(52) > 100','at(56) > 100'):
        check(token in codec,'Strict preference reader: '+token)
    check('SourceSchema()' in session and 'NeedsFormatUpgrade()' in session,'Accurate legacy schema metadata')
    fixture=json.loads((root/'tests/fixtures/m1_7_options_fixture.json').read_text())
    raw=bytes.fromhex(fixture['fixture_options_hex'])
    check(len(raw)==64 and hashlib.sha256(raw).hexdigest()==fixture['fixture_options_sha256'],'Frozen actual M1.7 fixture hash')
    check(fixture['source_schema']==2 and fixture['generation']==29,'Frozen fixture source/version metadata')
    fixture_header=(root/'tests/fixtures/m1_7_options_fixture.h').read_text()
    check(bytes(int(h,16) for h in re.findall(r'0x([0-9a-f]{2})',fixture_header))==raw,'Frozen fixture header matches provenance')
    check(not any(spec['content'].values()),'No supplied audio/assets/map falsely claimed')
    for path in ('tests/audio_options_tests.cpp','tests/audio_upgrade_tests.cpp','docs/M1_8_IMPLEMENTATION.md','docs/M1_8_AUDIO_WIRING.md'):
        check((root/path).is_file(),'M1.8 required delivery: '+path)
