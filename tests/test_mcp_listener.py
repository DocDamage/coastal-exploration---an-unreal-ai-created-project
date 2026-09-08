"""Protocol boundaries; engine behavior is checked against the live editor."""
import importlib.util
import json
from pathlib import Path
import sys
import types
import unittest
from unittest.mock import patch


class ListenerProtocolTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path = Path(__file__).resolve().parents[1] / "tools/unreal/coastal_mcp_listener.py"
        spec = importlib.util.spec_from_file_location("listener_under_test", path)
        cls.listener = importlib.util.module_from_spec(spec)
        with patch.dict(sys.modules, {"unreal": types.ModuleType("unreal")}):
            spec.loader.exec_module(cls.listener)

    def call(self, **fields):
        request = {"id": "test", "kind": "call_function", "method": "tools.list", "args": {}}
        request.update(fields)
        return self.listener.handle_request(json.dumps(request).encode())

    def test_lists_implemented_methods(self):
        reply = self.call()
        self.assertTrue(reply["ok"])
        self.assertIn("editor.get_state", {tool["name"] for tool in reply["result"]["tools"]})

    def test_rejects_malformed_envelopes(self):
        for raw in (b"[]", b"null", b"broken", b"\xff"):
            with self.subTest(raw=raw):
                self.assertFalse(self.listener.handle_request(raw)["ok"])
        for fields in ({"args": []}, {"method": []}, {"kind": "bad"}, {"id": {}}, {"id": True}):
            with self.subTest(fields=fields):
                self.assertFalse(self.call(**fields)["ok"])

    def test_unknown_method_fails_closed(self):
        self.assertFalse(self.call(method="editor.delete_everything")["ok"])

    def test_python_result_and_output(self):
        reply = self.call(method="editor.execute_python", args={"code": "print('hello'); result = 2 + 3"})
        self.assertEqual(reply["result"], {"stdout": "hello\n", "result": 5})

    def test_python_failure_is_an_error(self):
        reply = self.call(method="editor.execute_python", args={"code": "raise ValueError('failed')"})
        self.assertFalse(reply["ok"])
        self.assertEqual(reply["error"]["type"], "ValueError")

    def test_missing_code_cannot_execute(self):
        for code in (None, "", [], 12):
            self.assertFalse(self.call(method="editor.execute_python", args={"code": code})["ok"])

    def test_cyclic_python_result_returns_protocol_error(self):
        reply = self.call(method="editor.execute_python", args={"code": "result = []; result.append(result)"})
        encoded = self.listener.encode_response(reply)
        self.assertTrue(encoded.endswith(b"\n"))
        self.assertFalse(json.loads(encoded)["ok"])
        self.assertEqual(json.loads(encoded)["id"], "test")

    def test_nested_slate_ticks_do_not_redispatch(self):
        calls = []
        def nested_pump(delta):
            calls.append(delta)
            self.listener._tick(delta)
        with patch.object(self.listener, "_pump", nested_pump):
            self.listener._tick(0.1)
        self.assertEqual(calls, [0.1])
        self.assertFalse(self.listener._processing)


if __name__ == "__main__":
    unittest.main()
