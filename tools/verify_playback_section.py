"""M1.9 source/spec checks. This does not compile or execute Unreal/audio."""
from __future__ import annotations
import json
from pathlib import Path
from collections.abc import Callable


def verify(root: Path, source: Path, descriptor: dict, automation_count: int,
           check: Callable[[bool, str], None]) -> None:
    spec = json.loads((root/'data/m1_9_playback_spec.json').read_text())
    deps = json.loads((root/'data/m1_9_dependency_register.json').read_text())
    ledger = json.loads((root/'data/m1_9_playback_acceptance.json').read_text())
    native = (source/'Private/CoastalAudioPlaybackComponent.cpp').read_text()
    voices = (source/'Private/CoastalAudioPlaybackSources.cpp').read_text()
    header = (source/'Public/CoastalAudioPlaybackComponent.h').read_text()
    rules = (source/'Public/Core/AudioPlaybackRules.h').read_text()
    ui = (source/'Private/CoastalUIAudio.cpp').read_text()
    life = (source/'Private/CoastalUISessionComponent.cpp').read_text()
    stack = (source/'Private/CoastalUIStack.cpp').read_text()
    mix = (source/'Private/CoastalAudioOptionsComponent.cpp').read_text()
    for label,obj in (('spec',spec),('dependencies',deps),('ledger',ledger)):
        check(obj['schema_version']==1,'M1.9 schema: '+label)
    check(deps['plugin_version']=='0.1.9-m1-audio-playback-source','Historical M1.9 version consistency')
    check(descriptor['Version']>=19,'Current delivery retains playback increment')
    check(deps['unreal_editor_automation_tests_total']==34 and automation_count>=34,'Historical M1.9 native inventory retained')
    for key,value in deps.items():
        if isinstance(value,bool): check(not value,'Do not invent native execution: '+key)
    check(all(deps[k] is None for k in ('host_project','engine_version','agis_version','hyper_version')),'No invented host details')
    cases=ledger['tests']; ids=[row['id'] for row in cases]
    check(len(cases)==34 and len(ids)==len(set(ids)),'34 unique actual-host acceptance cases')
    for row in cases:
        check(row['result']=='not_run' and row['evidence'] is None,'Native case remains unrun: '+row['id'])
    check('bool bEnablePlayback = false' in header and spec['default_opt_in'] is False,'Explicit playback opt-in')
    check(spec['ambience']['source_path'] is None and spec['radio']['source_path'] is None,'No fabricated sound paths')
    for token in ('IsPlayable()', 'IsLooping()', 'IsPlayWhenSilent()', 'GetDuration()',
                  'coastal::ValidAmbienceSource', 'coastal::ValidRadioSource'):
        check(token in native,'Validate actual source metadata: '+token)
    check('MaximumRadioSeconds = 900.0' in rules and spec['radio']['duration_max_seconds']==900,'Radio metadata duration cap')
    check('Playback.Step(Session->GetAudioPlaybackContext())' in native,'Native consumes exact tested playback model')
    check('AddTickPrerequisiteComponent(UI)' in native and 'RemoveTickPrerequisiteComponent(Session)' in native,'Balance UI tick ordering')
    for token in ('World->GetNetMode() != NM_Standalone','Owners.Num() != 1','IsComponentTickEnabled()',
                  'CurrentDevice.GetDeviceID() == Device.GetDeviceID()', 'AudioOptions->IsAudioReady()'):
        check(token in native,'Native fixed-owner prerequisite: '+token)
    for token in ('GetSessionEpoch()', 'TranscriptToken.IsValid()', 'Flow.IsTop(', 'HasBeenPresented()',
                  'Saves->IsBusy()', 'Saves->IsRecoveryRequired()', 'Saves->IsPlayerReturnActive()', 'Panels.Last()->IsVisible()'):
        check(token in ui,'Read actual session/presentation state: '+token)
    init = native.split('bool UCoastalAudioPlaybackComponent::InitializePlayback',1)[1].split('bool UCoastalAudioPlaybackComponent::IsPlaybackReady',1)[0]
    check('->Play(' not in init,'No sound during initialization')
    for token in ('bAutoActivate = false','bAutoDestroy = false','bCanPlayMultipleInstances = false',
                  'bIsUISound = bRadio','SoundClassOverride = Bus','AudioDeviceID = Device.GetDeviceID()',
                  'bAttenuate = false','SetVolumeMultiplier(1.0f)', 'SetPitchMultiplier(1.0f)', 'bSuppressSubtitles = true'):
        check(token in voices,'Native private voice policy: '+token)
    check(voices.count('->Play(')==2,'One Play site for each configured category')
    check('->SetPaused(true)' in voices and '->SetPaused(false)' in voices,'Pause/resume instead of replay')
    check('->Stop()' in voices and '->DestroyComponent()' in voices,'Explicit source cleanup')
    check('OnAudioFinished.Add' not in native+voices and 'OnAudioFinishedNative.Add' not in native+voices,'No audio-finished authority or retry')
    check('->IsPlaying(' not in native+voices,'No silent/drop/finish retry loop')
    for forbidden in ('AcknowledgeTranscript(', 'FinishRadioTransmission(', 'TryCollectWorldItem(', 'TryCommitRequirements(',
                      'TryTransfer(', 'RestoreInventory(', 'SaveNow(', 'LoadCampaign(', 'StartNewCampaign(', 'DeleteGameInSlot(',
                      'SetInputMode(', 'SetPause(', 'SetIgnoreMoveInput(', 'ClearAllMappings(', 'ClearSoundMixModifiers('):
        check(forbidden not in native+voices,'Playback cannot take gameplay/global ownership: '+forbidden)
    release=mix.split('void UCoastalAudioOptionsComponent::ReleaseOptions()',1)[1]
    check(release.index('if (bStopped) return') < release.index('OnRoutingReleased.Broadcast()') < release.index('PopSoundMixModifier('),
          'Notify dependent sources before mix pop, including recursive release')
    check('OnRoutingReleased.AddUObject' in native and 'OnRoutingReleased.Remove' in native,'Balanced routing-release subscription')
    check(life.index('AudioPlayback->ReleasePlayback()') < life.index('AudioOptions->ReleaseOptions()'),'UI stops sources before volume cleanup')
    check('InitializeAudioPlayback();' in life and 'AudioPlayback->InitializePlayback(this, AudioOptions)' in ui,'Existing UI initializes optional playback')
    check('SuspendAudioPlayback();' in stack and 'RefreshAudioPlayback();' in stack,'Immediate top-panel/cancellation hooks')
    check('AudioPlayback->LastDetail' in life,'Optional playback errors remain visible')
    check(not any(spec['content'].values()),'No audio/map/vendor/footstep content invented')
    check(not spec['preservation']['campaign_schema_changed'] and not spec['preservation']['preferences_schema_changed'],
          'No campaign or preference migration')
    script=(root/'data/m1_9_radio_recording_script.txt').read_text()
    check(json.loads((root/'data/first_signal.json').read_text())['radio_message'] in script,'Recording script matches existing transcript')
    for path in ('docs/M1_9_IMPLEMENTATION.md','docs/M1_9_PLAYBACK_WIRING.md','tests/audio_playback_tests.cpp',
                 'Plugins/CoastalFoundation/Source/CoastalFoundation/Private/Tests/M1AudioPlaybackAutomationTests.cpp'):
        check((root/path).is_file(),'M1.9 required file: '+path)
