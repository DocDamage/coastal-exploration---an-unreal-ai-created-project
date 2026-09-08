from __future__ import annotations
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

PATH = Path(__file__).resolve().parents[1] / "tools/integration_report.py"
spec = importlib.util.spec_from_file_location("coastal_report", PATH)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class IntegrationReportTests(unittest.TestCase):
    def report(self, issues=None):
        return module.make_report("/Game/Test.Test", "fixture-not-a-real-engine", issues or [], [])
    def test_manifest_pass_never_passes_runtime(self):
        r = self.report()
        self.assertTrue(r["room_manifest_passed"])
        self.assertFalse(r["disk_map_verified"])
        self.assertTrue(all(v == "not_run" for v in r["runtime_gates"].values()))
    def test_fail_preserves_issue(self):
        i = {"code": "room.missing_object", "subject": "world.test.fuse", "detail": "Required actor missing."}
        r = self.report([i]); self.assertFalse(r["room_manifest_passed"])
        self.assertEqual(r["issues"], [i])
        i["code"] = "changed"; self.assertEqual(r["issues"][0]["code"], "room.missing_object")
    def test_dirty_packages_explicit(self):
        r = module.make_report("world", "fixture", [], ["B", "A", "B"])
        self.assertEqual(r["dirty_packages"], ["A", "B"])
        self.assertFalse(r["disk_map_verified"])
    def test_invalid_fields_rejected(self):
        for args in [("", "fixture", [], []), ("world", "", [], []), ("world", "fixture", {}, []),
                     ("world", "fixture", [{"code": "bad"}], []), ("world", "fixture", [], [3]),
                     ("world", "fixture", [{"code":"x","subject":"s","detail":""}], [])]:
            with self.subTest(args=args), self.assertRaises(ValueError): module.make_report(*args)
    def test_large_list_refused(self):
        with self.assertRaises(ValueError):
            module.make_report("world", "fixture", [{"code":"x","subject":"s","detail":"d"}]*513, [])
    def test_write_new_roundtrip(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)/"report.json"; r = self.report(); module.write_new_report(p,r)
            self.assertEqual(json.loads(p.read_text()), r)
    def test_existing_file_not_overwritten(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)/"report.json"; p.write_text("original")
            with self.assertRaises(FileExistsError): module.write_new_report(p,self.report())
            self.assertEqual(p.read_text(), "original")
    def test_missing_parent_not_created(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)/"absent"/"report.json"
            with self.assertRaises(ValueError): module.write_new_report(p,self.report())
            self.assertFalse(p.parent.exists())
    def test_wrong_extension_refused(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/"world.umap"
            with self.assertRaises(ValueError): module.write_new_report(p,self.report())
            self.assertFalse(p.exists())
    def test_invented_pass_claim_refused(self):
        with tempfile.TemporaryDirectory() as d:
            for key in module.NOT_RUN:
                r=self.report();r["runtime_gates"][key]="passed";p=Path(d)/(key+".json")
                with self.assertRaises(ValueError): module.write_new_report(p,r)
                self.assertFalse(p.exists())
    def test_room_pass_mismatch_refused(self):
        with tempfile.TemporaryDirectory() as d:
            r=self.report();r["room_manifest_passed"]=False
            with self.assertRaises(ValueError): module.write_new_report(Path(d)/"report.json",r)
    def test_unknown_claim_or_naive_date_refused(self):
        with tempfile.TemporaryDirectory() as d:
            for mutate in (lambda r: r.update(unreal_compiled=True), lambda r: r.update(observed_utc="2026-09-05T12:00:00")):
                r=self.report();mutate(r)
                with self.assertRaises(ValueError): module.write_new_report(Path(d)/"report.json",r)

if __name__ == "__main__": unittest.main()
