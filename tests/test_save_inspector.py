from pathlib import Path
import importlib.util
import struct
import tempfile
import unittest
import zlib

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('inspector', ROOT / 'tools/inspect_save_envelope.py')
inspector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inspector)


def fixture(payload=b'123456789'):
    return struct.pack('<8sIII', b'COASTSAV', 1, len(payload), zlib.crc32(payload)) + payload


class SaveInspectorTests(unittest.TestCase):
    def test_known_crc(self):
        result = inspector.inspect(fixture())
        self.assertEqual(result['status'], 'integrity_valid')
        self.assertEqual(result['computed_crc32'], 'cbf43926')

    def test_all_truncations(self):
        data = fixture()
        for i in range(len(data)):
            self.assertEqual(inspector.inspect(data[:i])['status'], 'corrupt')

    def test_payload_corruption(self):
        data = bytearray(fixture()); data[-1] ^= 1
        self.assertEqual(inspector.inspect(data)['reason'], 'checksum_mismatch')

    def test_future_version(self):
        data = bytearray(fixture()); data[8] = 2
        self.assertEqual(inspector.inspect(data)['status'], 'unsupported')

    def test_trailing_data(self):
        self.assertEqual(inspector.inspect(fixture() + b'x')['status'], 'corrupt')

    def test_empty_payload(self):
        self.assertEqual(inspector.inspect(fixture(b''))['status'], 'corrupt')

    def test_copy_keeps_original(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, dst = Path(tmp)/'original.sav', Path(tmp)/'fixture.sav'
            src.write_bytes(fixture())
            inspector.corrupt_copy(src, dst)
            self.assertEqual(src.read_bytes(), fixture())
            self.assertEqual(inspector.inspect(dst.read_bytes())['reason'], 'checksum_mismatch')

    def test_refuses_same_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp)/'original.sav'; src.write_bytes(fixture())
            with self.assertRaises(ValueError): inspector.corrupt_copy(src, src)
            self.assertEqual(src.read_bytes(), fixture())

    def test_refuses_existing_destination(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, dst = Path(tmp)/'original.sav', Path(tmp)/'existing.sav'
            src.write_bytes(fixture()); dst.write_bytes(b'keep')
            with self.assertRaises(FileExistsError): inspector.corrupt_copy(src, dst)
            self.assertEqual(dst.read_bytes(), b'keep')

    def test_refuses_bad_source(self):
        with tempfile.TemporaryDirectory() as tmp:
            src, dst = Path(tmp)/'bad.sav', Path(tmp)/'fixture.sav'
            src.write_bytes(b'invalid')
            with self.assertRaises(ValueError): inspector.corrupt_copy(src, dst)
            self.assertFalse(dst.exists())


if __name__ == '__main__':
    unittest.main()
