"""An editor on the same port must never silently receive another project's work."""
import sys
from pathlib import Path
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import unreal_mcp_call as client


class EditorIdentityTests(unittest.TestCase):
    def test_wrong_project_blocks_execution(self):
        with patch.object(client, 'request', return_value={
                'project': str(client.PROJECT.parent / 'Other.uproject'), 'engine': '5.8.2'}) as request:
            with self.assertRaisesRegex(RuntimeError, 'another project'):
                client.call('editor.execute_python', {'code': 'result = 1'})
            self.assertEqual(request.call_count, 1)

    def test_wrong_engine_blocks_execution(self):
        with patch.object(client, 'request', return_value={
                'project': str(client.PROJECT), 'engine': '5.7.4'}) as request:
            with self.assertRaisesRegex(RuntimeError, 'Unreal 5.8'):
                client.call('editor.save')
            self.assertEqual(request.call_count, 1)

    def test_verified_identity_allows_requested_operation(self):
        identity = {'project': str(client.PROJECT), 'engine': '5.8.2'}
        with patch.object(client, 'request', side_effect=[identity, {'saved': True}]) as request:
            self.assertEqual(client.call('editor.save'), {'saved': True})
            self.assertEqual(request.call_args.args[:2], ('editor.save', {}))

    def test_oversized_request_never_connects(self):
        with patch.object(client.socket, 'create_connection') as connect:
            with self.assertRaises(ValueError):
                client.request('editor.execute_python', {'code': 'x' * 262144})
            connect.assert_not_called()
