#!/usr/bin/env python3
"""Read-only local-file preflight. NEVER builds, imports assets, or certifies APIs."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]


def object_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding='utf-8-sig'))
    if not isinstance(value, dict):
        raise ValueError(f'{path.name} must contain a JSON object')
    return value


def inspect_host(project: Path, engine: Path | None, agis: Path | None, hyper: Path | None) -> dict:
    result: dict = {
        'schema_version': 1,
        'scope': 'local file presence/configuration only; does not inspect vendor APIs or run Unreal',
        'project': str(project.resolve()), 'checks': [],
        'unreal_compilation': 'not_run', 'vendor_integration': 'not_run',
        'editor_gameplay': 'not_run', 'windows_packaging': 'not_run',
    }
    def check(name: str, passed: bool, detail: str) -> None:
        result['checks'].append({'name': name, 'passed': bool(passed), 'detail': detail})
    valid_project = project.is_file() and project.suffix.lower() == '.uproject'
    check('uproject', valid_project, 'Explicit host project must exist; nothing is created.')
    doc: dict = {}
    if valid_project:
        try:
            doc = object_json(project)
            check('project_json', True, 'Parsed project descriptor.')
        except (OSError, UnicodeError, ValueError) as exc:
            check('project_json', False, str(exc))
    check('cpp_host', isinstance(doc.get('Modules'), list) and len(doc.get('Modules', [])) > 0,
          'Use the actual C++ host project, not an invented target.')
    result['engine_association'] = doc.get('EngineAssociation')
    descriptors = doc.get('Plugins', [])
    check('enhanced_input_not_disabled', isinstance(descriptors, list) and not any(
        isinstance(p, dict) and p.get('Name') == 'EnhancedInput' and p.get('Enabled') is False
        for p in descriptors), 'The Coastal plugin declares EnhancedInput; the host must not explicitly disable it.')
    editor_targets: list[str] = []
    source = project.parent / 'Source'
    if source.is_dir():
        for target in sorted(source.rglob('*.Target.cs')):
            text = target.read_text(encoding='utf-8-sig')
            if re.search(r'Type\s*=\s*TargetType\.Editor\s*;', text):
                editor_targets.append(target.name.removesuffix('.Target.cs'))
    result['editor_targets'] = editor_targets
    check('editor_target', len(editor_targets) == 1, 'Exactly one explicit Editor target expected for this M1 host.')
    descriptor = project.parent / 'Plugins/CoastalFoundation/CoastalFoundation.uplugin'
    try:
        installed = object_json(descriptor)
        expected = object_json(ROOT / 'Plugins/CoastalFoundation/CoastalFoundation.uplugin')
        check('installed_coastal_version', installed.get('VersionName') == expected['VersionName'],
              f"Expected {expected['VersionName']}; found {installed.get('VersionName')}")
    except (OSError, UnicodeError, ValueError) as exc:
        check('installed_coastal_version', False, str(exc))
    ui_source = descriptor.parent / 'Source/CoastalFoundation/Private/CoastalUISessionComponent.cpp'
    check('installed_ui_source', ui_source.is_file(), 'Native UI source present; this is NOT a compiler check.')
    files = ('Engine/Build/BatchFiles/Build.bat', 'Engine/Build/BatchFiles/RunUAT.bat',
             'Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
    for relative in files:
        path = engine / relative if engine else None
        check(relative, bool(path and path.is_file()), str(path) if path else 'Supply --engine-root for the selected local engine.')
    for name, path in (('AGIS', agis), ('Hyper', hyper)):
        count = sum(1 for _ in path.rglob('*.uasset')) if path and path.is_dir() else 0
        check(f'{name}_asset_files', count > 0,
              f'{count} .uasset files found. Presence is not proof of compatibility or actual interface wiring.')
    result['required_files_present'] = all(c['passed'] for c in result['checks'])
    return result


def write_new_report(path: Path, report: dict) -> None:
    if path.suffix.lower() != '.json':
        raise ValueError('Report destination must be a new .json file.')
    with path.open('x', encoding='utf-8') as stream:
        json.dump(report, stream, indent=2)
        stream.write('\n')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', required=True, type=Path)
    parser.add_argument('--engine-root', type=Path)
    parser.add_argument('--agis-root', type=Path)
    parser.add_argument('--hyper-root', type=Path)
    parser.add_argument('--report', type=Path, help='Optional NEW JSON report; never overwrites.')
    args = parser.parse_args()
    try:
        report = inspect_host(args.project, args.engine_root, args.agis_root, args.hyper_root)
        if args.report:
            write_new_report(args.report, report)
        print(json.dumps(report, indent=2))
        return 0 if report['required_files_present'] else 1
    except (OSError, UnicodeError, ValueError) as exc:
        print(f'Preflight failed without changing project files: {exc}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
