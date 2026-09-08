"""Boundary tests for the selected-root vendor content importer."""
import json
import subprocess
import sys
import tempfile
import unittest
import zipfile
import stat
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "extract_expansion_content.py"


class ExtractExpansionContentTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.archive = self.root / "vendor.zip"
        self.host = self.root / "Host"
        (self.host / "Content").mkdir(parents=True)

    def tearDown(self):
        self.temporary.cleanup()

    def write_archive(self, entries):
        with zipfile.ZipFile(self.archive, "w") as archive:
            for name, contents in entries:
                archive.writestr(name, contents)

    def run_import(self, *extra):
        return subprocess.run(
            [sys.executable, str(SCRIPT), str(self.archive), str(self.host), *extra],
            text=True,
            capture_output=True,
            check=False,
        )

    def test_selects_only_requested_content_root(self):
        self.write_archive([
            ("Bundle/Content/SwimmingAnimationPack/Anims/Swim.uasset", b"swim"),
            ("Bundle/Content/SwimmingAnimationPack/Anims/Swim.uexp", b"exp"),
            ("Bundle/Content/ThirdPerson/Blueprints/Unrelated.uasset", b"other"),
            ("Bundle/Outside/ignored.txt", b"ignored"),
        ])

        result = self.run_import("--root", "SwimmingAnimationPack")

        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["files_written"], 2)
        self.assertEqual(report["root_summaries"]["SwimmingAnimationPack"]["planned_files"], 2)
        self.assertTrue((self.host / "Content/SwimmingAnimationPack/Anims/Swim.uasset").is_file())
        self.assertFalse((self.host / "Content/ThirdPerson").exists())

    def test_rejects_duplicate_archive_destinations_before_writing(self):
        self.write_archive([
            ("Bundle/Content/SwimmingAnimationPack/Anims/Swim.uasset", b"first"),
            ("Bundle/Content/SwimmingAnimationPack/Anims/Swim.uasset", b"second"),
        ])

        result = self.run_import("--root", "SwimmingAnimationPack")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Duplicate archive destination", result.stderr)
        self.assertFalse((self.host / "Content/SwimmingAnimationPack").exists())

    def test_rejects_archive_symlink_before_writing(self):
        with zipfile.ZipFile(self.archive, "w") as archive:
            link = zipfile.ZipInfo("Bundle/Content/SwimmingAnimationPack/Alias.uasset")
            link.create_system = 3
            link.external_attr = (stat.S_IFLNK | 0o777) << 16
            archive.writestr(link, "outside")

        result = self.run_import("--root", "SwimmingAnimationPack")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Refusing archive symlink", result.stderr)
        self.assertFalse((self.host / "Content/SwimmingAnimationPack").exists())

    def test_rejects_existing_same_size_content_with_different_hash(self):
        self.write_archive([
            ("Bundle/Content/SwimmingAnimationPack/Anims/Swim.uasset", b"source"),
        ])
        target = self.host / "Content/SwimmingAnimationPack/Anims/Swim.uasset"
        target.parent.mkdir(parents=True)
        target.write_bytes(b"target")

        result = self.run_import("--root", "SwimmingAnimationPack")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Existing content differs", result.stderr)
        self.assertEqual(target.read_bytes(), b"target")

    def test_retry_verifies_existing_file_and_writes_only_missing_file(self):
        entries = [
            ("Bundle/Content/SwimmingAnimationPack/Anims/Existing.uasset", b"existing"),
            ("Bundle/Content/SwimmingAnimationPack/Anims/Missing.uasset", b"missing"),
        ]
        self.write_archive(entries)
        existing = self.host / "Content/SwimmingAnimationPack/Anims/Existing.uasset"
        existing.parent.mkdir(parents=True)
        existing.write_bytes(b"existing")

        result = self.run_import("--root", "SwimmingAnimationPack")

        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report["files_written"], 1)
        self.assertEqual(report["files_verified_existing"], 1)
        self.assertEqual(report["bytes_verified_existing"], len(b"existing"))
        self.assertEqual((self.host / "Content/SwimmingAnimationPack/Anims/Missing.uasset").read_bytes(), b"missing")


if __name__ == "__main__":
    unittest.main()
