import copy
import importlib.util
import json
from pathlib import Path
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('recovery_layout', ROOT / 'tools/recovery_layout.py')
layout = importlib.util.module_from_spec(spec)
spec.loader.exec_module(layout)
class RecoveryLayoutTests(unittest.TestCase):
    def setUp(self):
        self.data = json.loads((ROOT / 'data/m1_5_safety_layout.json').read_text())
    def bad(self):
        with self.assertRaises(ValueError): layout.validate_layout(self.data)
    def test_actual_recipe_valid_without_mutation(self):
        before=copy.deepcopy(self.data)
        self.assertEqual(len(layout.validate_layout(self.data)),4)
        self.assertEqual(self.data,before)
    def test_wrong_map(self): self.data['map_id']='level.coast'; self.bad()
    def test_no_runtime_pass_claim(self): self.data['status']='passed'; self.bad()
    def test_duplicate_names(self): self.data['volumes'].append(copy.deepcopy(self.data['volumes'][0])); self.bad()
    def test_unknown_type(self): self.data['volumes'][0]['kind']='SWIMMING'; self.bad()
    def test_negative_extent(self): self.data['volumes'][0]['half_extent_cm'][0]=-2; self.bad()
    def test_zero_extent(self): self.data['volumes'][0]['half_extent_cm'][0]=0; self.bad()
    def test_nan(self): self.data['volumes'][0]['center_cm'][0]=float('nan'); self.bad()
    def test_infinity(self): self.data['volumes'][0]['center_cm'][0]=float('inf'); self.bad()
    def test_boolean_coordinate(self): self.data['volumes'][0]['center_cm'][0]=True; self.bad()
    def test_excessive_coordinate(self): self.data['volumes'][0]['center_cm'][0]=1000001; self.bad()
    def test_unknown_field(self): self.data['project_path']='fake'; self.bad()
    def test_empty_and_excessive_rows(self):
        original=self.data['volumes']; self.data['volumes']=[]; self.bad()
        self.data['volumes']=original*9; self.bad()
    def test_wrong_units(self): self.data['units']='metres'; self.bad()
    def test_timing_drift(self): self.data['timings_seconds']['fade_out']=4; self.bad()
    def test_invalid_labels(self): self.data['volumes'][0]['label']='../../Content'; self.bad()
    def test_unhashable_type(self): self.data['volumes'][0]['kind']=[]; self.bad()
    def test_huge_integer(self): self.data['volumes'][0]['center_cm'][0]=10**400; self.bad()
    def test_boolean_schema(self): self.data['schema_version']=True; self.bad()
    def test_bad_vector_length(self): self.data['volumes'][0]['center_cm']=[1,2]; self.bad()
    def test_non_dictionary(self): self.data=[]; self.bad()
if __name__ == '__main__': unittest.main()
