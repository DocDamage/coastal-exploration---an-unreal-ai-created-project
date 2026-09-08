#!/usr/bin/env python3
"""Validate specification consistency and source layout; not an Unreal build test."""
from __future__ import annotations
import json
import re
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
checks = 0
failures: list[str] = []


def check(condition: bool, message: str) -> None:
    global checks
    checks += 1
    if not condition:
        failures.append(message)


def load(name: str) -> dict:
    path = ROOT / 'data' / name
    try:
        obj = json.loads(path.read_text(encoding='utf-8'))
    except (OSError, ValueError) as exc:
        raise RuntimeError(f'Cannot read {path}: {exc}') from exc
    if not isinstance(obj, dict):
        raise RuntimeError(f'{name} must have an object root.')
    check(obj.get('schema_version') == 1, f'{name}: unsupported schema')
    return obj


def unique(values: list[str], label: str) -> None:
    check(len(values) == len(set(values)), f'{label}: duplicated IDs')
    check(all(isinstance(x, str) and x for x in values), f'{label}: empty IDs')


def main() -> int:
    items = load('items.json')
    quest = load('first_signal.json')
    level = load('level_blockout.json')
    inputs = load('input_map.json')
    assets = load('asset_register.json')
    unique([x['id'] for x in items['items']], 'items')
    unique([x['id'] for x in inputs['actions']], 'input actions')
    unique([x['id'] for x in assets['assets']], 'asset register')
    unique([x['id'] for x in level['areas']], 'level areas')
    unique(level['world_ids'], 'world IDs')
    by_id = {x['id']: x for x in items['items']}
    for item in items['items']:
        check(item['quantity'] > 0, f"{item['id']}: invalid quantity")
        check(item['stack_limit'] >= item['quantity'], f"{item['id']}: invalid stack")
        check(len(item['grid']) == 2 and all(x > 0 for x in item['grid']),
              f"{item['id']}: invalid grid")
        for grid_name in ('initial_inventory_grid', 'cabin_storage_grid'):
            check(all(a <= b for a, b in zip(item['grid'], items[grid_name])),
                  f"{item['id']}: cannot fit {grid_name}")
        check(item['mesh_path'] is None and item['icon_path'] is None,
              f"{item['id']}: starter must not claim resolved asset paths")
    for item_id, count in quest['required_items'].items():
        check(item_id in by_id, f'Quest has undefined item {item_id}')
        check(isinstance(count, int) and count > 0, f'Invalid requirement {item_id}')
        if item_id in by_id:
            check(by_id[item_id]['protect_from_discard'], f'Critical item unprotected: {item_id}')
            check(by_id[item_id]['consumed_by'] == quest['transaction_id'],
                  f'Wrong transaction mapping: {item_id}')
    check(set(quest['states']) == set(quest['objective_copy']), 'Missing objective text')
    check(quest['duration_target_minutes'][0] <= quest['duration_target_minutes'][1],
          'Invalid duration range')

    source = ROOT / 'Plugins/CoastalFoundation/Source/CoastalFoundation'
    component = (source / 'Private/FirstSignalComponent.cpp').read_text()
    types = (source / 'Public/CoastalFoundationTypes.h').read_text()
    core = (source / 'Public/Core/FirstSignalRules.h').read_text()
    adapter = (source / 'Private/CoastalInventoryAdapter.cpp').read_text()
    cpp_ids = set(re.findall(r'\.ItemId\s*=\s*TEXT\("([^"]+)"\)', component))
    check(cpp_ids == set(quest['required_items']), 'C++/JSON required item IDs disagree')
    check('int32 Quantity = 1;' in types, 'Default C++ requirement quantity changed')
    check(all(v == 1 for v in quest['required_items'].values()), 'Update C++ for new quantities')
    check(quest['transaction_id'] in component, 'C++/JSON transaction ID mismatch')
    for name in quest['states']:
        check(name in core and name in types, f'Phase missing from source: {name}')
    check('return ECoastalInventoryCommit::NotConfigured;' in adapter,
          'Unwired adapter does not fail closed')
    check('return ECoastalInventoryCommit::Committed;' not in adapter,
          'Default adapter must not pretend to consume items')
    check('Templates/UnrealTemplate.h' in component, 'TGuardValue include missing')

    area_ids = {x['id'] for x in level['areas']}
    graph = {x: set() for x in area_ids}
    for a, b in level['main_connections'] + level['optional_connections']:
        check(a in area_ids and b in area_ids, 'Connection names an unknown area')
        if a in graph and b in graph:
            graph[a].add(b)
            graph[b].add(a)
    reached, todo = set(), ['area.cabin']
    while todo:
        node = todo.pop()
        if node in reached:
            continue
        reached.add(node)
        todo.extend(graph.get(node, ()) - reached)
    check(reached == area_ids, 'Blockout graph has unreachable areas')
    check(not level['actual_route_length_measured'], 'Do not claim a measured blockout route')
    check(not assets['source_packages_available'], 'Do not claim purchased source assets are included')
    for asset in assets['assets']:
        check(asset['url'].startswith('https://'), f"{asset['id']}: invalid source URL")
        check(asset['import_test'] == 'not_run' and asset['package_test'] == 'not_run',
              f"{asset['id']}: source package cannot certify local tests")
        check(asset['local_content_root'] is None, f"{asset['id']}: invented local asset root")

    plugin = ROOT / 'Plugins/CoastalFoundation/CoastalFoundation.uplugin'
    descriptor = json.loads(plugin.read_text())
    check(descriptor['Modules'][0]['Name'] == 'CoastalFoundation', 'Wrong module name')
    check(not descriptor['CanContainContent'], 'Source starter claims included plugin content')
    check('EngineVersion' not in descriptor, 'Untested source must not imply a pinned engine build')
    for path in source.rglob('*.h'):
        text = path.read_text()
        includes = re.findall(r'^#include\s+"([^"]+)"', text, flags=re.M)
        if '.generated.h' in text:
            check(includes[-1] == path.stem + '.generated.h',
                  f'{path.name}: generated header must be last include')
    for ext in ('*.h', '*.cpp', '*.cs'):
        for path in source.rglob(ext):
            check(len(path.read_text().splitlines()) <= 300, f'{path.name}: too large for this starter')
    for path in ROOT.rglob('*'):
        if path.is_file():
            check(path.suffix.lower() not in {'.uasset', '.umap', '.uproject', '.fbx', '.upluginmanifest'},
                  f'Unexpected binary/project content: {path}')
    for path in ROOT.rglob('*.md'):
        text = path.read_text(encoding='utf-8')
        text = re.sub(r'(?ms)^```[^\n]*\n.*?^```[ \t]*$', '', text)
        for target in re.findall(r'\]\(([^)]+)\)', text):
            if target.startswith(('https://', 'http://', '#', 'mailto:')):
                continue
            target = unquote(target.split('#')[0])
            check((path.parent / target).exists(), f'Broken local link in {path.name}: {target}')
    room = load('test_room.json')
    journal = load('journal.json')
    dependencies = load('m1_dependency_register.json')
    unique([x['world_id'] for x in room['objects']], 'M1 world IDs')
    unique([x['id'] for x in journal['entries']], 'M1 journal IDs')
    check(room['units'] == 'centimeters', 'M1 generator expects centimeters')
    check(room['map_path'].startswith('/Game/Coastal/Maps/'), 'Unexpected test-map destination')
    check(room['status'] == 'editor_recipe_not_an_included_map', 'Do not claim generated map')
    pickups = [x for x in room['objects'] if x['kind'] == 'PICKUP']
    check({x['item_id'] for x in pickups} == set(quest['required_items']) and len(pickups) == 2,
          'Test room must have exactly the two required source pickups')
    for kind in ('RADIO', 'STORAGE', 'DOOR', 'DISCOVERY', 'MAINTENANCE_NOTE'):
        check(sum(x['kind'] == kind for x in room['objects']) == 1, f'M1 expected one {kind}')
    for obj in room['objects']:
        check(len(obj['position_cm']) == len(obj['size_cm']) == 3, 'Invalid proxy transform')
        check(all(x > 0 for x in obj['size_cm']), 'Invalid proxy dimensions')
    story = (source / 'Private/CoastalStoryLibrary.cpp').read_text()
    for entry in journal['entries']:
        check(entry['id'] in story, f'Journal ID absent from runtime: {entry["id"]}')
        check(json.dumps(entry['text'], ensure_ascii=True) in story,
              f'Journal text not synchronized: {entry["id"]}')
    check(json.dumps(quest['radio_message']) in story, 'Radio transcript mismatch')
    if any(dependencies[key] for key in ('engine_compiled', 'vendor_integrated', 'windows_packaged')):
        # The delivered source baseline had no host. Later integration claims need
        # separate current evidence; never rewrite the historical delivery reports.
        evidence_path = ROOT / 'docs/native-evidence/integrated-gameplay-summary.json'
        native_path = ROOT / 'docs/native-evidence/integrated-unreal-automation-summary.json'
        check(evidence_path.is_file() and native_path.is_file(), 'Native claims need current evidence summaries')
        if evidence_path.is_file() and native_path.is_file():
            current = json.loads(evidence_path.read_text(encoding='utf-8'))
            native = json.loads(native_path.read_text(encoding='utf-8'))
            check(current.get('automated_packaged_core_loop_passed') is True
                  and current.get('same_campaign_across_relaunch') is True,
                  'Packaged claim needs coherent fresh-process acceptance')
            check(native.get('failed') == 0 and native.get('notRun') == 0
                  and native.get('succeeded', 0) + native.get('succeededWithWarnings', 0) >= 38,
                  'Native claim needs all supplied Unreal tests')
            check(dependencies.get('full_m1_gate_passed') == current.get('full_m1_gate_passed'),
                  'Full acceptance status must remain distinct from core loop status')
    else:
        check(not dependencies.get('packaged_core_loop_passed', False), 'Do not claim unrun integration gates')
    for method in ('GetProviderStatus', 'TryCollectWorldItem', 'TryTransfer', 'ExportInventory',
                   'ValidateInventory', 'RestoreInventory', 'BuildNewInventory'):
        check(method + '_Implementation' in adapter, f'Missing fail-closed adapter method {method}')
    builder = (ROOT / 'tools/unreal/build_systems_test.py').read_text()
    check('does_asset_exist' in builder and 'get_dirty_map_packages' in builder,
          'Map-generation overwrite/unsaved-work guard missing')
    check('CoastalMissionDirector' in builder and 'CoastalWorldObject' in builder, 'Generator classes out of sync')
    for path in ROOT.rglob('*.py'):
        try:
            compile(path.read_text(encoding='utf-8'), str(path), 'exec')
            check(True, f'Python syntax: {path.name}')
        except SyntaxError as exc:
            check(False, f'Python syntax: {path.name}: {exc}')
    ui_input = (source / 'Private/CoastalUIInput.cpp').read_text()
    ui_stack = (source / 'Private/CoastalUIStack.cpp').read_text()
    ui_commands = (source / 'Private/CoastalUICommands.cpp').read_text()
    ui_present = (source / 'Private/CoastalUIPresentation.cpp').read_text()
    ui_panel = (source / 'Private/CoastalPanelWidget.cpp').read_text()
    ui_session = (source / 'Private/CoastalUISessionComponent.cpp').read_text()
    check('ReadContainerView_Implementation' in adapter and 'View = {};' in adapter,
          'View adapter must fail closed and clear stale output')
    check(descriptor['VersionName'] == '0.1.10-m1-display-settings-source', 'Wrong M1.10 source version')
    check(any(p['Name'] == 'EnhancedInput' and p['Enabled'] for p in descriptor['Plugins']),
          'Enhanced Input plugin dependency missing')
    build = (source / 'CoastalFoundation.Build.cs').read_text()
    for module in ('UMG', 'Slate', 'SlateCore', 'InputCore', 'EnhancedInput'):
        check('"' + module + '"' in build, f'Missing UI module dependency: {module}')
    check('ClearAllMappings' not in ui_input and 'ResetIgnore' not in ui_input,
          'UI must not erase unrelated mappings or input locks')
    check('FInputModeUIOnly' in ui_input and 'bIgnoreAllPressedKeysUntilRelease = true' in ui_input,
          'UI-only ownership / release guard missing')
    check('AcquireUIBlocker' in ui_stack and 'ReleaseUIBlocker' in ui_stack,
          'Panel blockers must have ownership and teardown')
    check('ClaimCommand' in ui_commands and 'Epoch != Saves->GetSessionEpoch()' in ui_commands,
          'Missing top-screen/frame/session checks')
    check('NativePaint' in ui_panel and 'Presented.MarkPainted' in ui_panel,
          'Transcript presentation gate missing')
    check('OnSaveNotice.RemoveDynamic' in ui_session and 'RemoveMenuInput' in ui_session,
          'UI teardown missing')
    for command in set(re.findall(r'Add\(TEXT\("([^"\n]+)"\)', ui_present)):
        check('TEXT("' + command + '")' in ui_commands,
              f'Visible command has no dispatcher branch: {command}')
    acceptance = load('m1_2_ui_acceptance.json')
    unique([x['id'] for x in acceptance['tests']], 'M1.2 acceptance IDs')
    check(all(x['result'] == 'not_run' and x['evidence'] is None for x in acceptance['tests']),
          'Must not mark unexecuted engine/controller scenarios passed')
    ui_dependencies = load('m1_2_dependency_register.json')
    check(not any(ui_dependencies[x] for x in ('host_project_available', 'vendor_packages_available',
          'unreal_compiled', 'native_widgets_rendered', 'controller_gameplay_tested', 'windows_packaged')),
          'Unrun integration status misrepresented')
    automation_count = sum(p.read_text().count('IMPLEMENT_SIMPLE_AUTOMATION_TEST(')
                           for p in (source / 'Private/Tests').glob('*.cpp'))
    check(ui_dependencies['unreal_editor_automation_tests_total'] == 7,
          'Retained M1.2 automation inventory must remain historical')
    startup_dependencies = load('m1_3_dependency_register.json')
    check(startup_dependencies['unreal_editor_automation_tests_total'] == 11,
          'Retained M1.3 automation inventory must remain historical')
    check(not any(startup_dependencies[k] for k in ('host_project_available', 'vendor_packages_available',
          'unreal_compiled', 'native_startup_executed', 'native_widgets_rendered', 'editor_room_audit_executed',
          'vendor_transactions_tested', 'controller_gameplay_tested', 'windows_packaged', 'paid_assets_included')),
          'M1.3 must not claim unexecuted engine/vendor/Windows gates')
    startup_acceptance = load('m1_3_startup_acceptance.json')
    unique([x['id'] for x in startup_acceptance['tests']], 'M1.3 acceptance IDs')
    check(len(startup_acceptance['tests']) == 26 and all(x['result'] == 'not_run' and x['evidence'] is None
          for x in startup_acceptance['tests']), 'Startup ledger must contain only unrun local tests')
    worksheet = load('m1_3_integration_worksheet.json')
    check(all(v is None for k,v in worksheet.items() if k not in ('schema_version','status')),
          'Do not invent local integration paths or versions')
    manifest_source = (source / 'Public/Core/TestRoomRules.h').read_text()
    manifest_rows = re.findall(r'\{"([^"]+)", "([^"]+)", "([^"]*)", "([^"]*)", (\d+), false\}', manifest_source)
    check(len(manifest_rows) == 7, 'Native M1 authoring manifest must be present')
    for row in room['objects']:
        matches = [m for m in manifest_rows if m[0] == row['world_id']]
        check(len(matches) == 1 and matches[0][1:] == (row['kind'], row.get('item_id',''),
              row.get('journal_entry',''), '1'), 'Native room/data mismatch: ' + row['world_id'])
    startup = (source / 'Private/CoastalSessionBootstrapComponent.cpp').read_text()
    preflight = (source / 'Private/CoastalStartupPreflight.cpp').read_text()
    provider_audit = (source / 'Private/CoastalProviderAudit.cpp').read_text()
    editor_audit = (ROOT / 'tools/unreal/audit_systems_test.py').read_text()
    for forbidden in ('TryCollectWorldItem(', 'TryTransfer(', 'TryCommitRequirements(', 'RestoreInventory(', 'BuildNewInventory('):
        check(forbidden not in provider_audit, 'Provider audit must not probe mutation: ' + forbidden)
    for forbidden in ('StartNewCampaign(', 'LoadCampaign(', 'SaveNow(', 'SaveGameToSlot('):
        check(forbidden not in startup, 'Bootstrap must not create/load/write a campaign: ' + forbidden)
    check(startup.index('RunPreflight(') < startup.index('->Saves->Configure(')
          < startup.index('BoundBridge->Configure(') < startup.index('BoundUI->InitializeUI('),
          'Startup must check prerequisites before binding in the original order')
    check('bPublishing' in startup and 'Gate.Begin()' in startup and 'StopForIntegrationFailure' in startup,
          'Startup reentrancy and partial-failure guards missing')
    check('AuditProvisionalProvider' in preflight and 'Report.Issues.IsEmpty()' in preflight
          and '->Configure(' not in preflight, 'Preflight must remain read-only')
    for forbidden in ('new_level(', 'load_level(', 'save_current_level(', 'spawn_actor', 'set_editor_property('):
        check(forbidden not in editor_audit, 'Editor audit must not modify maps: ' + forbidden)
    check('inspect_test_room(world)' in editor_audit and 'is_in_play_in_editor()' in editor_audit,
          'Editor audit must use actual native world inspection and refuse PIE')
    check('UCoastalPlacementLibrary::IsDryDestination' in (source/'Private/CoastalSaveRestore.cpp').read_text()
          and preflight.count('UCoastalPlacementLibrary::IsDryDestination') == 3,
          'Startup and restore must use the same dry-placement policy')
    check('if (!InstallMenuInput()) { RemoveMenuInput();' in ui_session,
          'Partially installed menu input must be removed on initialization failure')
    interaction_dependencies = load('m1_4_dependency_register.json')
    check(interaction_dependencies['unreal_editor_automation_tests_total'] == 15,
          'Historical M1.4 automation inventory must stay at fifteen')
    check(not any(interaction_dependencies[k] for k in ('host_project_available', 'vendor_packages_available',
          'unreal_compiled', 'native_relay_executed', 'native_input_executed', 'hyper_focus_prompt_wired',
          'vendor_transactions_tested', 'controller_gameplay_tested', 'windows_packaged', 'paid_assets_included')),
          'M1.4 cannot claim unrun native/vendor/runtime gates')
    interaction_acceptance = load('m1_4_interaction_acceptance.json')
    unique([x['id'] for x in interaction_acceptance['tests']], 'M1.4 acceptance IDs')
    check(len(interaction_acceptance['tests']) == 31 and all(x['result'] == 'not_run' and x['evidence'] is None
          for x in interaction_acceptance['tests']), 'Interaction ledger must retain all unrun real-engine scenarios')
    relay = (source/'Private/CoastalInteractionRelayComponent.cpp').read_text()
    relay_header = (source/'Public/CoastalInteractionRelayComponent.h').read_text()
    interaction_input = (source/'Private/CoastalInteractionInput.cpp').read_text()
    offer = (source/'Private/CoastalInteractionOffer.cpp').read_text()
    bridge = (source/'Private/CoastalInteractionBridge.cpp').read_text()
    rules = (source/'Public/Core/InteractionRules.h').read_text()
    check('BoundRelay->InitializeRelay(BoundBridge)' in startup and startup.index('BoundRelay->InitializeRelay(')
          < startup.index('BoundUI->InitializeUI('), 'Present relay must initialize before the UI')
    check('Relays.Num() > 1' in preflight and 'Relays.Num() == 1' in preflight,
          'Relay preflight must support intentional legacy absence but reject duplicates')
    check(bridge.count('DispatchGate.Begin(GFrameCounter)') == 3 and bridge.count('FDispatchEnd Dispatch{DispatchGate}') == 3,
          'Interact, transfer and transcript need whole-call dispatch guards')
    check('~FDispatchEnd() { Gate.End(); }' in bridge and '++InputRevision' in bridge,
          'Dispatch teardown and UI permission revision required')
    for forbidden in ('TryCollectWorldItem(', 'TryCommitRequirements(', 'TryTransfer(', 'RequestRadioRepair(',
                      'FinishRadioTransmission(', 'StartNewCampaign(', 'LoadCampaign(', 'RequestSave('):
        check(forbidden not in offer and forbidden not in relay and forbidden not in interaction_input,
              'Relay and offer source must not bypass the game bridge: ' + forbidden)
    for forbidden in ('TActorIterator', 'LineTrace', 'SphereTrace', 'OverlapMulti', 'CreateWidget', 'AddToViewport'):
        check(forbidden not in relay and forbidden not in interaction_input,
              'Relay must not add target discovery or a competing prompt renderer: ' + forbidden)
    for forbidden in ('ClearAllMappings', 'ResetIgnore', 'SetInputMode', 'SetIgnoreMoveInput', 'SetIgnoreLookInput'):
        check(forbidden not in interaction_input and forbidden not in relay,
              'Relay must not seize unrelated input ownership: ' + forbidden)
    check('InputOwner = ECoastalInteractInputOwner::VendorEvents' in relay_header,
          'Default mode must preserve external input ownership')
    check('BoundInputOwner != ECoastalInteractInputOwner::VendorEvents' in relay,
          'Native mode must reject external action submission')
    for event in ('Started', 'Completed', 'Canceled'):
        check('ETriggerEvent::' + event in interaction_input, 'Native lifecycle event missing: ' + event)
    check('ETriggerEvent::Triggered' not in interaction_input and 'bIgnoreAllPressedKeysUntilRelease = true' in interaction_input,
          'Native input must not repeat on Triggered and must suppress held keys at install')
    check('EKeys::E' in interaction_input and 'EKeys::Gamepad_FaceButton_Left' in interaction_input,
          'Native Interact keys must match the original input specification')
    check('RemoveMappingContext(InteractContext)' in interaction_input and 'PopInputComponent(InteractInput)' in interaction_input,
          'Relay teardown must remove only owned native resources')
    check('Lease.Valid' in relay and 'Intent.Synchronize' in relay and 'MaxAgeFrames = 2' in rules,
          'Fresh-focus and permission/session invalidation contract missing')
    check('TWeakObjectPtr<ACoastalWorldObject> FocusedTarget' in relay_header,
          'Focused actor must not be kept alive by a strong target reference')
    check('ClearExpected' in relay and 'SamePresentation' in relay and 'OnOfferChanged.Broadcast' in relay,
          'Focus-loss ordering and changed-only presentation required')
    check('coastal::ChooseOffer' in offer and 'coastal::WorldPermissionFor' in offer and 'CheckRequirements(' in offer,
          'Runtime offer must use shared projection rules and real carried-availability reads')
    check('GetRepairRequirements()' in offer and 'ApplyNativeState' not in offer,
          'Offer must read mission requirements without world mutation')
    recovery_dependencies = load('m1_5_dependency_register.json')
    check(recovery_dependencies['unreal_editor_automation_tests_total'] == 19,
          'Retained M1.5 Unreal automation inventory must remain historical')
    for key, value in recovery_dependencies.items():
        if isinstance(value, bool):
            check(value is False, 'Unrun recovery native/vendor/package evidence must not be claimed: ' + key)
    acceptance = load('m1_5_recovery_acceptance.json')
    unique([x['id'] for x in acceptance['tests']], 'M1.5 acceptance IDs')
    check(len(acceptance['tests']) == 32 and all(x['result'] == 'not_run' and x['evidence'] is None
          for x in acceptance['tests']), 'Recovery ledger requires 32 unrun native scenarios')
    recovery = (source/'Private/CoastalPlayerRecoveryComponent.cpp').read_text()
    execution = (source/'Private/CoastalRecoveryExecution.cpp').read_text()
    return_input = (source/'Private/CoastalRecoveryInput.cpp').read_text()
    save_return = (source/'Private/CoastalSaveRecovery.cpp').read_text()
    placement = (source/'Private/CoastalPlacementLibrary.cpp').read_text()
    save_restore = (source/'Private/CoastalSaveRestore.cpp').read_text()
    recovery_rules = (source/'Public/Core/RecoveryRules.h').read_text()
    gate = (source/'Public/Core/CampaignRules.h').read_text()
    for forbidden in ('TryCollectWorldItem(', 'TryCommitRequirements(', 'TryTransfer(', 'RestoreInventory(',
                      'BuildNewInventory(', 'LoadCampaign(', 'StartNewCampaign('):
        check(not any(forbidden in text for text in (recovery, execution, return_input, save_return)),
              'Return must not create or mutate inventory/campaigns: ' + forbidden)
    for forbidden in ('ClearAllMappings', 'ResetIgnore', 'SetInputMode', 'SetPause(', 'AddMappingContext('):
        check(forbidden not in return_input and forbidden not in execution,
              'Return must preserve unrelated contexts/input mode/pause: ' + forbidden)
    check('ReturnInput->Priority = 90' in return_input and 'ReturnInput->bBlockInput = true' in return_input
          and 'PopInputComponent(ReturnInput)' in return_input, 'Exclusive gameplay input and owned teardown required')
    check('bIgnoreAllPressedKeysUntilRelease = true' in return_input and 'StopJumping()' in return_input,
          'Jump and held-input state must not leak out of relocation')
    check('TryReturnCandidates' in recovery_rules and 'coastal::TryReturnCandidates' in execution,
          'Bounded candidate order must be shared with standalone tests')
    check('coastal::ReturnFlow Flow' in (source/'Public/CoastalPlayerRecoveryComponent.h').read_text()
          and 'coastal::CheckpointVisit Visit' in (source/'Public/CoastalPlayerRecoveryComponent.h').read_text(),
          'Runtime must use tested phase and dwell rules')
    check('Gate.BeginRecovery()' in save_return and 'Gate.EndRecovery(); Gate.RequestSave()' in save_return
          and 'recovery_ || poisoned_' in gate, 'Return must exclusively reserve existing campaign gate')
    check('return SafeDestination(Player->GetActorTransform())' in save_return
          and 'return SafeDestination(Player->GetActorTransform())' in save_restore,
          'Both relocation and campaign restore must validate actual adjusted teleport result')
    check('TouchesCapsule' in placement and 'BelowBoundary' in placement and 'GetUpVector' in placement,
          'All dry-destination checks must reject authored hazards, fall plane and tilted capsules')
    check('AddTickPrerequisiteComponent(this)' in recovery and 'TG_PostPhysics' in recovery,
          'Recovery sampling must precede deferred saves and follow movement')
    check('Recoveries.Num() > 1' in preflight and 'Recoveries.Num() != 1' in preflight,
          'Recovery startup must reject duplicate owners and ownerless safety volumes')
    check('BoundRecovery->InitializeRecovery' in startup
          and startup.index('BoundRecovery->InitializeRecovery') < startup.index('BoundUI->InitializeUI'),
          'Recovery must bind before UI subscribes')
    check('OnReturnNotice.AddUniqueDynamic' in ui_session and 'OnReturnNotice.RemoveDynamic' in ui_session
          and 'Saves->IsPlayerReturnActive()' in ui_stack, 'Native UI feedback, teardown and modal exclusion required')
    check('bFailurePublished' in execution and 'FailPlayerReturn' in execution and 'ClearFade()' in execution,
          'Terminal failures must remain visible, fail closed and not repeatedly publish')
    import runpy
    layout = load('m1_5_safety_layout.json')
    validator = runpy.run_path(str(ROOT/'tools/recovery_layout.py'))['validate_layout']
    try:
        check(len(validator(layout)) == 4, 'Expected four optional safety test volumes')
    except (ValueError, TypeError) as exc:
        check(False, 'Safety recipe validation: ' + str(exc))
    for constant, key in [('ReturnFadeOut','fade_out'), ('ReturnFadeIn','fade_in'),
                          ('ReturnStableTime','ground_stability'), ('ReturnSettleLimit','settle_timeout'),
                          ('CheckpointDwell','checkpoint_dwell')]:
        check('constexpr double ' + constant + ' = ' + str(layout['timings_seconds'][key]) + ';' in recovery_rules,
              'Safety timing specification drift: ' + key)
    check('def build(include_safety: bool = False)' in builder and 'safety = validate(' in builder
          and builder.index('safety = validate(') < builder.index('level.new_level('),
          'Opt-in recipe must validate before level creation and retain legacy default')
    options_spec = load('m1_6_options_spec.json')
    options_dependencies = load('m1_6_dependency_register.json')
    check(options_dependencies['plugin_version'] == '0.1.6-m1-player-options-source', 'Historical M1.6 version mismatch')
    check(options_dependencies['unreal_editor_automation_tests_total'] == 22,
          'Historical M1.6 native test inventory must remain twenty-two (not run)')
    for key, value in options_dependencies.items():
        if isinstance(value, bool):
            check(value is False, 'Unrun options integration must not be claimed: ' + key)
    check(all(options_dependencies[k] is None for k in ('engine_version', 'host_project', 'agis_version', 'hyper_version')),
          'Do not invent real host/engine/vendor versions')
    options_acceptance = load('m1_6_options_acceptance.json')
    unique([x['id'] for x in options_acceptance['tests']], 'M1.6 acceptance IDs')
    check(len(options_acceptance['tests']) == 34 and all(x['result'] == 'not_run' and x['evidence'] is None
          for x in options_acceptance['tests']), 'Options ledger must contain 34 unrun local scenarios')
    options_rules = (source/'Public/Core/PlayerOptionsRules.h').read_text()
    options_codec = (source/'Public/Core/OptionsPersistence.h').read_text()
    options_session = (source/'Public/Core/OptionsSession.h').read_text()
    options_native = (source/'Private/CoastalLocalOptions.cpp').read_text()
    options_header = (source/'Public/CoastalLocalOptions.h').read_text()
    options_ui = (source/'Private/CoastalUIOptions.cpp').read_text()
    options_look = (source/'Private/CoastalLookInputComponent.cpp').read_text()
    options_look_header = (source/'Public/CoastalLookInputComponent.h').read_text()
    panel_input = (source/'Private/CoastalPanelInput.cpp').read_text()
    ui_hud = (source/'Private/CoastalHUDWidget.cpp').read_text()
    ui_rules = (source/'Public/Core/UIFlowRules.h').read_text()
    for forbidden in ('TryCollectWorldItem(', 'TryCommitRequirements(', 'TryTransfer(', 'RestoreInventory(',
                      'BuildNewInventory(', 'LoadCampaign(', 'StartNewCampaign(', 'SaveNow(', 'RequestSave(', 'DeleteGameInSlot('):
        check(not any(forbidden in text for text in (options_native, options_ui, options_look)),
              'Options must not mutate inventory/campaigns or delete files: ' + forbidden)
    for forbidden in ('ClearAllMappings', 'SetIgnoreLookInput(', 'ResetIgnore', 'AddMappingContext(',
                      'PushInputComponent(', 'SetInputMode', 'SetPause('):
        check(forbidden not in options_look, 'Camera/look consumer must not acquire competing input ownership: ' + forbidden)
    spec_fields = options_spec['options']
    for field, default, low, high, step, cpp in (
            ('mouse_percent',100,25,300,25,'mousePercent'), ('stick_percent',100,25,300,25,'stickPercent'),
            ('field_of_view',85,70,110,5,'fieldOfView'), ('text_percent',100,100,150,10,'textPercent')):
        check(spec_fields[field] == dict(default=default, min=low, max=high, step=step), 'Options data drift: ' + field)
        check(cpp + ' = ' + str(default) in options_rules, 'Option default missing from runtime: ' + field)
        check(f'p.{cpp} >= {low} && p.{cpp} <= {high} && p.{cpp} % {step} == 0' in options_rules,
              'Option validation drift: ' + field)
    check(spec_fields['invert_y']['default'] is False and 'bool invertY = false' in options_rules, 'Invert-Y default changed')
    persistence = options_spec['persistence']
    check(persistence['slots'] == ['CoastalLocalOptions_v1_A', 'CoastalLocalOptions_v1_B'], 'Preference namespace drift')
    for name in persistence['slots']:
        check(name in options_native and not name.startswith('Coastal_'), 'Preference file entered campaign namespace: ' + name)
    check(persistence['payload_bytes'] == 40 and persistence['encoded_bytes'] == 60
          and 'LegacyOptionsPayloadSize = 40' in options_codec and 'OptionsPayloadSize + EnvelopeHeaderSize' in options_codec,
          'Preference encoded size drift')
    check(persistence['user_index'] == 0 and 'SlotName(Index), 0' in options_native, 'Local preference user index changed')
    for flag in ('auto_write_on_load', 'campaign_data_included', 'delete_on_failure', 'cross_process_lock'):
        check(persistence[flag] is False, 'Unsupported preference claim: ' + flag)
    check(persistence['verified_readback'] and 'bytes != verified.bytes' in options_session
          and 'verified.slot.record.generation != next.generation' in options_session,
          'Preference writes must compare exact read-back and generation')
    check('coastal::OptionsSession State' in options_header and 'State.Initialize' in options_native
          and 'State.SaveAndApply' in options_native, 'Runtime must use the tested preference protocol')
    init_body = options_session.split('template<class Read> bool Initialize(Read read)', 1)[1].split('bool ApplySession', 1)[0]
    apply_body = options_session.split('bool ApplySession', 1)[1].split('template<class Read, class Write>', 1)[0]
    check('write(' not in init_body and 'write(' not in apply_body, 'Load/session-only apply must not write')
    check('current.bytes != observed_[i].bytes' in options_session and 'OptionsNotice::DiskChanged' in options_session,
          'External preference file changes must block writes')
    check('selection_.writable = false; notice_ = OptionsNotice::WriteUnverified' in options_session,
          'Unverified preference write must lock further disk writes')
    check('Unsupported' in options_codec and 'ReadError' in options_codec and 'a.record.generation == b.record.generation' in options_codec,
          'Unsupported/unreadable/conflicting preferences must not be guessed')
    look_spec = options_spec['look']
    check(look_spec['default_use_host_look_events'] is False and 'bool bUseHostLookEvents = false' in options_look_header,
          'Host look routing must be opt-in')
    check(look_spec['base_stick_rate_input_units_per_second'] == 90.0 and 'BaseStickRate = 90.0' in options_rules,
          'Stick rate tuning drift')
    check(look_spec['max_stick_frame_seconds'] == 0.1 and 'std::min(seconds, 0.1)' in options_rules, 'Stick hitch cap drift')
    check(look_spec['neutral_epsilon'] == 0.0001 and 'std::abs(x) <= 0.0001 && std::abs(y) <= 0.0001' in options_rules,
          'True stick neutral guard missing')
    check(not look_spec['mouse_delta_time_multiplier'] and not look_spec['second_dead_zone'], 'Unsupported look normalization claims')
    check('SubmitMouseLook' in options_look and 'SubmitStickLook' in options_look and 'Gate.Mouse' in options_look
          and 'Gate.Stick' in options_look, 'Both real host device routes must use the shared input gate')
    mouse_body = options_look.split('bool UCoastalLookInputComponent::SubmitMouseLook', 1)[1].split('bool UCoastalLookInputComponent::SubmitStickLook', 1)[0]
    check('GetDeltaSeconds' not in mouse_body, 'Mouse deltas must not be multiplied by frame time')
    for token in ('GetSessionEpoch()', 'GetInputRevision()', 'AllowsWorldInput()', 'IsLookInputIgnored()',
                  'GetViewTarget()', 'ECameraProjectionMode::Perspective', 'GetPawn()', 'OriginalFov'):
        check(token in options_look, 'Missing camera/session permission safeguard: ' + token)
    check('SetFieldOfView' in options_look and 'bHostLookBound = bUseHostLookEvents' in options_look,
          'Actual camera apply and frozen host-route opt-in required')
    check('InitializeOptions()' in ui_session and 'ReleaseOptions()' in ui_session, 'UI must own preferences and release consumer')
    check('Kind == K::Settings' in ui_commands and 'OptionsCommand(Id)' in ui_commands,
          'Settings commands must use existing top-only dispatcher')
    for command in set(re.findall(r'Add\(TEXT\("([^"\n]+)"\)', options_ui)):
        check(command == 'back' or 'Id == TEXT("' + command + '")' in options_ui, 'Options button has no handler: ' + command)
    check('OptionsDraft = Options->Get()' in options_ui and 'Options->ApplySession(OptionsDraft)' in options_ui
          and 'Options->SaveAndApply(OptionsDraft)' in options_ui, 'Draft/session/verified-disk choices required')
    check('Field == coastal::OptionField::Text' in options_ui and 'IsCameraReady()' in options_ui and 'IsLookReady()' in options_ui,
          'Unavailable host settings must be disabled without blocking text settings')
    check('Frame->SetContent(Scroller)' in ui_panel and 'Scroller->AddChild(Column)' in ui_panel
          and 'Column->AddChildToVerticalBox(Actions)' in ui_panel, 'Whole menu including commands must scroll')
    check('EditableStyle.SetFont' in ui_panel and 'SaveField->SetWidgetStyle(EditableStyle)' in ui_panel
          and 'SaveField->SetFont(' not in ui_panel, 'Editable font must use the actual Slate widget style API')
    for label, base in (('Heading',26), ('Body',20), ('Notice',16), ('Help',14), ('Label',19)):
        check(label + '->SetFont' in ui_panel and 'Session->FontSize(' + str(base) + ')' in ui_panel,
              'Native panel label must use base-relative text scaling: ' + label)
    check('ScrollWidgetIntoView' in panel_input and 'FocusDefault(0, false)' in ui_stack and 'Widget->ScrollToTop()' in ui_stack,
          'Focus visibility must coexist with initial body presentation')
    check('VisibleUIIntersection' in ui_rules and 'coastal::VisibleUIIntersection' in ui_panel
          and 'GetLayoutBoundingRect()' in ui_panel, 'Transcript paint gate must require a visible body intersection')
    check('Slot->SetAutoSize(true)' in ui_hud and 'Session->HasModal()' in ui_hud and 'SetWidthOverride' in ui_hud,
          'HUD must grow by content, adapt width and not compete with modal text')
    for filename in ('options_core_tests.cpp', 'options_codec_tests.cpp', 'options_storage_tests.cpp'):
        check((ROOT/'tests'/filename).is_file(), 'Missing options regression suite: ' + filename)
    sprint_spec = load('m1_7_sprint_spec.json')
    sprint_dependencies = load('m1_7_dependency_register.json')
    check(sprint_dependencies['plugin_version'] == '0.1.7-m1-sprint-source', 'Historical sprint version mismatch')
    check(sprint_dependencies['unreal_editor_automation_tests_total'] == 26,
          'Historical M1.7 native automation inventory remains twenty-six, not run')
    for key, value in sprint_dependencies.items():
        if isinstance(value, bool):
            check(value is False, 'Unrun sprint/engine/vendor execution must not be claimed: ' + key)
    check(all(sprint_dependencies[k] is None for k in ('engine_version','host_project','agis_version','hyper_version')),
          'Do not invent engine/host/vendor versions')
    acceptance = load('m1_7_sprint_acceptance.json')
    unique([x['id'] for x in acceptance['tests']], 'M1.7 acceptance IDs')
    check(len(acceptance['tests']) == 35 and all(x['result'] == 'not_run' and x['evidence'] is None for x in acceptance['tests']),
          'Sprint acceptance must contain 35 unrun native scenarios')
    sprint = (source/'Private/CoastalSprintComponent.cpp').read_text()
    sprint_header = (source/'Public/CoastalSprintComponent.h').read_text()
    sprint_core = (source/'Public/Core/SprintRules.h').read_text()
    for forbidden in ('TryCollectWorldItem(', 'TryCommitRequirements(', 'TryTransfer(', 'RestoreInventory(',
                      'BuildNewInventory(', 'LoadCampaign(', 'StartNewCampaign(', 'SaveNow(', 'RequestSave(',
                      'DeleteGameInSlot(', 'AddMovementInput(', 'SetMovementMode(', 'Velocity =',
                      'MaxWalkSpeedCrouched =', 'AddMappingContext(', 'ClearAllMappings', 'SetInputMode',
                      'SetPause(', 'SetIgnoreMoveInput(', 'ResetIgnore'):
        check(forbidden not in sprint, 'Sprint must not take unrelated ownership or mutate campaign: ' + forbidden)
    for token in ('coastal::SprintGate Gate', 'coastal::WalkSpeedLease SpeedLease'):
        check(token in sprint_header, 'Native Sprint must use shared tested ownership/input core: ' + token)
    check('float WalkSpeed = 330.0f' in sprint_header and 'float SprintSpeed = 520.0f' in sprint_header
          and sprint_spec['default_walk_cm_s'] == 330.0 and sprint_spec['default_sprint_cm_s'] == 520.0,
          'Sprint default tuning drift')
    check('bool bUseHostSprintEvents = false' in sprint_header and not sprint_spec['default_use_host_sprint_events'],
          'Host Sprint route must remain opt-in')
    for token in ('AllowsWorldInput()', 'IsMoveInputIgnored()', 'GetSessionEpoch()', 'GetInputRevision()',
                  'MovementMode == MOVE_Walking', '!Character->bIsCrouched', 'NM_Standalone',
                  'GetCharacterMovement() == Movement', 'Owners.Num() != 1'):
        check(token in sprint, 'Sprint permission/lifetime check missing: ' + token)
    for token in ('AddTickPrerequisiteActor(Controller)', 'AddTickPrerequisiteComponent(this)',
                  'RemoveTickPrerequisiteActor(Controller)', 'RemoveTickPrerequisiteComponent(this)',
                  'SpeedLease.Release', 'SpeedLease.Apply', 'Gate.Sample', 'TG_PrePhysics'):
        check(token in sprint, 'Sprint ownership/execution binding missing: ' + token)
    check('SprintSampleLeaseFrames = 2' in sprint_core and sprint_spec['input_sample_lease_frames'] == 2,
          'Actual-input freshness contract drift')
    check('frame <= enabledFrame_' in sprint_core and 'frame <= sampleFrame_' in sprint_core
          and 'toggle != toggle_' in sprint_core and 'revision != revision_' in sprint_core,
          'Same-frame/rearm/option/menu guards missing')
    check('bool sprintToggle = false' in options_rules and 'OptionFieldCount = 10' in options_rules
          and 'case OptionField::SprintMode' in options_rules, 'Sixth bounded field missing')
    check('SprintOwners[0]->InitializeOptions(Options, Bridge)' in options_ui and 'SprintInput->ReleaseOptions()' in ui_session,
          'Existing UI must initialize and release optional sprint owner')
    for token in ('coastal::OptionField::SprintMode', 'IsSprintReady()', 'OptionsDraft.sprintToggle != Live.sprintToggle',
                  'OptionsDraft.sprintToggle = Defaults.sprintToggle', 'coastal::OptionFieldCount', 'ApplySprintOptions()'):
        check(token in options_ui, 'Sprint UI draft/apply/availability binding missing: ' + token)
    check('SprintInput->LastDetail' in ui_session and 'SprintInput->LastDetail' in options_ui,
          'Sprint ownership failures must have native UI presentation')
    migration = sprint_spec['persistence']
    check(migration['slots'] == persistence['slots'] and migration['read_schemas'] == [1,2]
          and migration['write_schema'] == 2 and not migration['auto_write_on_load']
          and not migration['campaign_schema_changed'], 'Preference upgrade contract drift')
    check(migration['payload_bytes'] == 44 and migration['encoded_bytes'] == 64
          and 'schema == 2 ? 44 : OptionsPayloadSize' in options_codec,
          'Retained historical schema2 size contract drift')
    for token in ('schema != 1 && schema != 2 && schema != OptionsSchema', 'schema == 1 ? LegacyOptionsPayloadSize',
                  'schema >= 2 && at(40) > 1', 'schema >= 2 && at(40) == 1',
                  'record = {values, generation, schema}'):
        check(token in options_codec, 'Strict legacy/new decoding missing: ' + token)
    check('UsesLegacyRecord()' in options_session and 'State.UsesLegacyRecord()' in options_native,
          'Explicit legacy profile diagnostics missing')
    check('State.UsesLegacyRecord()' not in options_look, 'Camera must not gain independent preference migration ownership')
    fixture = json.loads((ROOT/'tests/fixtures/m1_6_options_fixture.json').read_text())
    raw = bytes.fromhex(fixture['legacy_options_hex'])
    import hashlib
    check(len(raw) == 60 and hashlib.sha256(raw).hexdigest() == fixture['legacy_options_sha256'],
          'Frozen original M1.6 codec fixture hash mismatch')
    header = (ROOT/'tests/fixtures/m1_6_options_fixture.h').read_text()
    check(bytes(int(h,16) for h in re.findall(r'0x([0-9a-f]{2})',header)) == raw, 'Legacy fixture header differs from provenance')
    for filename in ('sprint_core_tests.cpp','options_upgrade_tests.cpp'):
        check((ROOT/'tests'/filename).is_file(), 'Missing sprint/upgrade regression suite: ' + filename)
    from verify_audio_section import verify
    verify(ROOT, source, descriptor, automation_count, check)
    from verify_playback_section import verify as verify_playback
    verify_playback(ROOT, source, descriptor, automation_count, check)
    from verify_display_section import verify as verify_display
    verify_display(ROOT, source, descriptor, automation_count, check)
    print(f'{checks} structural checks; {len(failures)} failures')
    for message in failures:
        print('FAIL:', message, file=sys.stderr)
    print('Scope: JSON/source/document consistency only; no Unreal, AGIS, platform-save, or gameplay execution.')
    return 1 if failures else 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (KeyError, TypeError, ValueError, RuntimeError, OSError) as exc:
        print(f'Validation aborted: {exc}', file=sys.stderr)
        sys.exit(2)
