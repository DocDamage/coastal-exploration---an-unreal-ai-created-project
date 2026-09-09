"""Stage the sample's default instance in addition to the graph dependency closure."""
import json
from ue58_paths import WORKSPACE, TRIAL, EVIDENCE, validate_trial
from stage_m3_mutable import digest
from extract_expansion_content import _reject_link_ancestors

validate_trial()
source = WORKSPACE / 'MutableSample/Content/Character/COI_Character.uasset'
target = TRIAL / 'Content/Character/COI_Character.uasset'
_reject_link_ancestors(source)
_reject_link_ancestors(target)
sha = digest(source)
if target.exists():
    if digest(target) != sha:
        raise SystemExit('Existing default instance differs; preserved')
else:
    with target.open('xb') as output:
        output.write(source.read_bytes())
if digest(target) != sha:
    raise SystemExit('Default instance copy did not verify')
(EVIDENCE / 'm3-mutable-default-copy.json').write_text(json.dumps(dict(source=str(source), target=str(target), sha256=sha, verified=True), indent=2))
