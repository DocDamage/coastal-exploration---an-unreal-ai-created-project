"""Verify saves/rollback files and distinguish the backed-up pistol map edit."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
baseline = json.loads((ROOT / 'local-evidence/m3-activities-preservation-baseline.json').read_text())
binding_path = ROOT / 'local-evidence/m3-pistol-binding.json'
binding = json.loads(binding_path.read_text()) if binding_path.exists() else {}
authored_map = ROOT / 'LocalHost58/CoastalExploration/Content/Coastal/Maps/L_FirstSignal.umap'
backup = ROOT / 'local-evidence/m3-activities-asset-backups/L_FirstSignal-before-pistol.umap'
rope_report = ROOT / 'local-evidence/m3-dynamic-rope-enable.json'
rope_enable = json.loads(rope_report.read_text()) if rope_report.exists() else {}
rope_project = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
rope_backup = ROOT / 'local-evidence/m3-activities-asset-backups/CoastalExploration-before-dynamic-rope.uproject'
gantry_path = ROOT / 'local-evidence/m3-grapple-gantry.json'
gantry = json.loads(gantry_path.read_text()) if gantry_path.exists() else {}
gantry_backup = ROOT / 'local-evidence/m3-activities-asset-backups/L_FirstSignal-before-grapple.umap'
zipline_path = ROOT / 'local-evidence/m3-zipline-authoring.json'
zipline = json.loads(zipline_path.read_text()) if zipline_path.exists() else {}
zipline_backup = ROOT / 'local-evidence/m3-activities-asset-backups/L_FirstSignal-before-zipline.umap'
rows = []
for name, expected in baseline.items():
    path = Path(name)
    if path.suffix not in ('.sav','.umap','.uproject'):
        continue
    actual = hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None
    reviewed_pistol_edit = (path.resolve() == authored_map.resolve()
        and binding.get('before_sha256') == expected
        and binding.get('after_sha256') == actual and backup.is_file()
        and hashlib.sha256(backup.read_bytes()).hexdigest() == expected)
    reviewed_gantry_edit = (path.resolve() == authored_map.resolve()
        and binding.get('before_sha256') == expected and backup.is_file()
        and hashlib.sha256(backup.read_bytes()).hexdigest() == expected
        and gantry.get('before_sha256') == binding.get('after_sha256')
        and gantry_backup.is_file() and hashlib.sha256(gantry_backup.read_bytes()).hexdigest() == gantry.get('before_sha256')
        and gantry.get('after_sha256') == actual)
    reviewed_rope_enable = (path.resolve() == rope_project.resolve()
        and rope_enable.get('before_sha256') == expected and rope_enable.get('after_sha256') == actual
        and rope_backup.is_file() and hashlib.sha256(rope_backup.read_bytes()).hexdigest() == expected)
    reviewed_zipline_edit = (path.resolve() == authored_map.resolve()
        and binding.get('before_sha256') == expected and backup.is_file()
        and hashlib.sha256(backup.read_bytes()).hexdigest() == expected
        and gantry.get('before_sha256') == binding.get('after_sha256')
        and gantry_backup.is_file() and hashlib.sha256(gantry_backup.read_bytes()).hexdigest() == gantry.get('before_sha256')
        and zipline.get('before_sha256') == gantry.get('after_sha256')
        and zipline_backup.is_file() and hashlib.sha256(zipline_backup.read_bytes()).hexdigest() == zipline.get('before_sha256')
        and zipline.get('after_sha256') == actual)
    rows.append(dict(path=name, unchanged=actual == expected, reviewed_pistol_edit=reviewed_pistol_edit,
                     reviewed_rope_enable=reviewed_rope_enable,
                     reviewed_gantry_edit=reviewed_gantry_edit,
                     reviewed_zipline_edit=reviewed_zipline_edit,
                     expected=expected, actual=actual))
report = dict(passed=all(r['unchanged'] or r['reviewed_pistol_edit'] or r['reviewed_rope_enable'] or r['reviewed_gantry_edit'] or r['reviewed_zipline_edit'] for r in rows), files=len(rows),
              unchanged=sum(r['unchanged'] for r in rows),
              reviewed_map_edits=sum(not r['unchanged'] and (r['reviewed_pistol_edit'] or r['reviewed_gantry_edit'] or r['reviewed_zipline_edit']) for r in rows),
              reviewed_plugin_enables=sum(not r['unchanged'] and r['reviewed_rope_enable'] for r in rows),
              saves=sum(Path(r['path']).suffix == '.sav' for r in rows), rows=rows)
(ROOT / 'local-evidence/m3-activities-preservation.json').write_text(json.dumps(report,indent=2))
print(json.dumps({k:v for k,v in report.items() if k != 'rows'}))
raise SystemExit(0 if report['passed'] else 1)
