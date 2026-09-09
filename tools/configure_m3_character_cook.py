"""Include dynamically loaded character definitions and Mutable child graphs in the private host cook."""
import hashlib
import json
from ue58_paths import PROJECT, EVIDENCE, validate_trial

validate_trial()
config = PROJECT.parent / 'Config/DefaultGame.ini'
before = config.read_bytes()
backup = EVIDENCE / 'm3-character-before-DefaultGame.ini'
if not backup.exists():
    backup.write_bytes(before)
text = before.decode('utf-8-sig')
section = '[/Script/UnrealEd.ProjectPackagingSettings]'
if section not in text:
    raise SystemExit('Existing packaging section required')
additions = [f'+DirectoriesToAlwaysCook=(Path="{path}")'
             for path in ('/Game/Coastal/Character', '/Game/Character')]
missing = [line for line in additions if line not in text]
if missing:
    text = text.replace(section, section + '\n' + '\n'.join(missing), 1)
    config.write_text(text, encoding='utf-8')
report = dict(path=str(config), added=missing, backup=str(backup),
              sha256=hashlib.sha256(config.read_bytes()).hexdigest())
(EVIDENCE / 'm3-character-cook-config.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report))
