"""Filesystem fixtures test the inspector, NOT an Unreal installation."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('preflight', ROOT / 'tools/preflight_host.py')
preflight = importlib.util.module_from_spec(spec)
spec.loader.exec_module(preflight)


class HostPreflightTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.project = self.root / 'Fixture.uproject'

    def inspect(self):
        return preflight.inspect_host(self.project, None, None, None)

    def test_missing_host_is_not_ready(self):
        self.assertFalse(self.inspect()['required_files_present'])
        self.assertFalse(self.project.exists())

    def test_malformed_descriptor_reported(self):
        self.project.write_text('{', encoding='utf-8')
        result = self.inspect()
        self.assertTrue(any(c['name'] == 'project_json' and not c['passed'] for c in result['checks']))
        self.assertEqual(self.project.read_text(), '{')

    def test_nonobject_json_reported(self):
        self.project.write_text('[]', encoding='utf-8')
        self.assertFalse(self.inspect()['required_files_present'])

    def test_disabled_input_detected(self):
        self.project.write_text(json.dumps({'Modules':[{'Name':'Fixture'}],
            'Plugins':[{'Name':'EnhancedInput', 'Enabled':False}]}), encoding='utf-8')
        result = self.inspect()
        self.assertFalse(next(c for c in result['checks'] if c['name'] == 'enhanced_input_not_disabled')['passed'])

    def test_editor_target_discovered_not_guessed(self):
        source = self.root / 'Source'
        source.mkdir()
        (source / 'ActualEditor.Target.cs').write_text('Type = TargetType.Editor;', encoding='utf-8')
        self.assertEqual(self.inspect()['editor_targets'], ['ActualEditor'])
        (source / 'OtherEditor.Target.cs').write_text('Type = TargetType.Editor;', encoding='utf-8')
        self.assertFalse(next(c for c in self.inspect()['checks'] if c['name'] == 'editor_target')['passed'])

    def test_never_claims_build_or_gameplay(self):
        result = self.inspect()
        for key in ('unreal_compilation', 'vendor_integration', 'editor_gameplay', 'windows_packaging'):
            self.assertEqual(result[key], 'not_run')

    def test_report_refuses_overwrite(self):
        report = self.root / 'report.json'
        report.write_text('keep', encoding='utf-8')
        with self.assertRaises(FileExistsError):
            preflight.write_new_report(report, self.inspect())
        self.assertEqual(report.read_text(), 'keep')

    def test_report_rejects_project_destination(self):
        with self.assertRaises(ValueError):
            preflight.write_new_report(self.project, self.inspect())
        self.assertFalse(self.project.exists())

    def test_new_report_roundtrip(self):
        report = self.root / 'report.json'
        result = self.inspect()
        preflight.write_new_report(report, result)
        self.assertEqual(json.loads(report.read_text()), result)


if __name__ == '__main__':
    unittest.main()
