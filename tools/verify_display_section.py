"""M1.10 source/data checks, not a compiler or Windows display test."""
from __future__ import annotations
import json
from pathlib import Path
from collections.abc import Callable


def verify(root: Path, source: Path, descriptor: dict, automation_count: int,
           check: Callable[[bool, str], None]) -> None:
    spec = json.loads((root/'data/m1_10_display_spec.json').read_text())
    deps = json.loads((root/'data/m1_10_dependency_register.json').read_text())
    ledger = json.loads((root/'data/m1_10_display_acceptance.json').read_text())
    for name, obj in [('spec',spec),('deps',deps),('ledger',ledger)]:
        check(obj['schema_version']==1,'M1.10 schema: '+name)
    check(descriptor['VersionName']==spec['plugin_version']==deps['plugin_version']=='0.1.10-m1-display-settings-source','Current M1.10 version')
    check(descriptor['Version']==20,'M1.10 numeric plugin version')
    # This register describes the imported M1.10 delivery. Later host tests
    # (including CapacityManifest) must not invalidate that historical count.
    check(deps['unreal_editor_automation_tests_total']==38 and automation_count>=38,
          'Historical M1.10 native inventory retained, not execution evidence')
    check(deps['standalone_cpp_suites']==18 and len(list((root/'tests').glob('*_tests.cpp')))>=18,
          'Historical M1.10 standalone inventory retained, not execution evidence')
    for key,val in deps.items():
        if isinstance(val,bool): check(not val,'No invented native result: '+key)
    for key in ['host_project','engine_version','agis_version','hyper_version','actual_monitor_configuration',
                'actual_supported_resolutions','actual_game_user_settings_path']:
        check(deps[key] is None,'No guessed local host detail: '+key)
    cases=ledger['tests']
    check(len(cases)==38 and len({r['id'] for r in cases})==38,'38 unique manual display cases')
    for row in cases:
        check(row['result']=='not_run' and row['evidence'] is None,'Manual case not executed: '+row['id'])
    core=(source/'Public/Core/DisplaySettingsRules.h').read_text()
    native=(source/'Private/CoastalDisplaySettings.cpp').read_text()
    trial=(source/'Private/CoastalDisplayTrial.cpp').read_text()
    ui=(source/'Private/CoastalUIDisplay.cpp').read_text()
    present=(source/'Private/CoastalUIDisplayPresentation.cpp').read_text()
    life=(source/'Private/CoastalUISessionComponent.cpp').read_text()
    stack=(source/'Private/CoastalUIStack.cpp').read_text()
    commands=(source/'Private/CoastalUICommands.cpp').read_text()
    header=(source/'Public/CoastalUISessionComponent.h').read_text()
    for token in ['FPlatformTime::Seconds()', 'GetSizeXY()', 'GetWindowMode()', 'IsForegroundWindow()',
                  'GetConvenientWindowedResolutions', 'GetSupportedFullscreenResolutions', 'GetDesktopResolution()',
                  'EWorldType::Game', 'PLATFORM_WINDOWS', '!GIsEditor', 'GetLocalPlayers().Num() == 1',
                  'Client->Viewport == BoundViewport', 'UGameUserSettings::StaticClass()']:
        check(token in native,'Native observed/admission API: '+token)
    for token in ['Model.Begin(', 'Model.Step(', 'Model.Keep(', 'Model.Cancel(']:
        check(token in native+trial,'Native uses exact tested state machine: '+token)
    check('TrialSeconds = 15.0, RestoreSeconds = 5.0' in core and spec['trial_seconds']==15 and spec['restore_observation_seconds']==5,'Real-time bounded policy')
    check('FSystemResolution::RequestResolutionChange(' in trial,'Deferred native request, not settings Apply-and-save')
    check('bEnableDisplaySettings = false' in header and spec['default_opt_in'] is False,'Explicit display ownership')
    check('Model.Keep(' in trial.split('bool UCoastalDisplaySettings::Keep',1)[1].split('if (bRequestSave)',1)[0],'Keep gate precedes optional persistence')
    check(trial.count('Settings->SaveSettings()')==1 and 'Settings->ConfirmVideoMode()' in trial,'One explicit native save site')
    check('disk persistence is not verified' in trial,'No void-API disk success claim')
    for token in ['->ApplySettings(', '->ApplyResolutionSettings(', '->ValidateSettings(', '->RevertVideoMode(',
                  '->SetToDefaults(', '->LoadSettings(', 'SaveNow(', 'SaveGameToSlot(', 'SaveDataToSlot(',
                  'DeleteGameInSlot(', 'SetIgnoreMoveInput(', 'SetInputMode(', 'AcknowledgeTranscript(', 'TryTransfer(']:
        check(token not in native+trial,'Display adapter does not take unrelated authority: '+token)
    check('TickDisplay();' in life and life.index('TickDisplay();') < life.index('if (Saves->IsBusy())'),'Timer precedes busy early return')
    check('CancelDisplay();' in stack and 'DisplaySettings->Release()' in life,'Close/clear/shutdown cancel wiring')
    check('Panel->ResetPresentation(); Panel->ScrollToTop();' in ui,'New-mode paint reset and visible body recovery')
    check('CanKeep(' in present and 'Panel->HasBeenPresented()' in present,'No keep without visible painted confirmation')
    check(present.index('Add(TEXT("display_revert")') < present.index('Add(TEXT("display_keep")'),'Revert is default focus')
    check('EmergencyDisplayCancel' in commands and 'Flow.ClaimCommand(' in commands,'Unpainted cancellation keeps ticket/frame checks')
    check('Model.Phase() == coastal::DisplayPhase::Failed' in trial,'Failed restore cannot become silent success')
    check('Model.Before()' in trial,'Rollback actual pretrial snapshot, not old config')
    check(not spec['preservation']['campaign_schema_changed'] and not spec['preservation']['preferences_schema_changed'],'No migration')
    for path in ['docs/M1_10_IMPLEMENTATION.md','docs/M1_10_DISPLAY_WIRING.md','docs/WHAT_REMAINS.md',
                 'tests/display_settings_tests.cpp','Plugins/CoastalFoundation/Source/CoastalFoundation/Private/Tests/M1DisplayAutomationTests.cpp']:
        check((root/path).is_file(),'Required M1.10 file: '+path)
