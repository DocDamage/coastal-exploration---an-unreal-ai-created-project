"""Author the single transient German Shepherd companion bootstrap in First Signal."""
import hashlib
import json
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
from m3_destination_authoring import Destination

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'local-evidence/m3-coastal-companion-authoring.json'
SAVE_DIR = Path(u.Paths.project_saved_dir()) / 'SaveGames'


def hashes():
    return {str(p.relative_to(SAVE_DIR)).replace('\\', '/'): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(SAVE_DIR.rglob('*')) if p.is_file()}


if not str(u.SystemLibrary.get_engine_version()).startswith('5.8.'):
    raise RuntimeError('Coastal companion authoring is UE5.8-only')
d = Destination('coastal_companion')
before_saves = hashes()
before_ids = sorted(str(a.world_id) for a in d.actors.get_all_level_actors()
                    if isinstance(a, u.CoastalWorldObject))
assets = {
    'mesh': '/Game/German_Shepherd_3D_Model/Models/SK_GermanShepherd_01',
    'idle': '/Game/German_Shepherd_3D_Model/Animations/A_type1_Idle_Playing_v01',
    'walk': '/Game/German_Shepherd_3D_Model/Animations/A_type1_Walk_Loop_v01',
    'run': '/Game/German_Shepherd_3D_Model/Animations/A_type1_Run_Loop_v01',
}
loaded = {key: u.load_asset(path) for key, path in assets.items()}
if not isinstance(loaded['mesh'], u.SkeletalMesh) or any(not loaded[key] for key in ('idle', 'walk', 'run')):
    raise RuntimeError('German Shepherd mesh or locomotion clips are missing')

director = d.actor(u.CoastalCompanionDirector, 'German Shepherd director', (0, 0, 300))
director.set_editor_property('companion_class', u.CoastalCompanionCharacter)
director.set_editor_property('companion_mesh_asset', loaded['mesh'])
director.set_editor_property('idle_animation', loaded['idle'])
director.set_editor_property('walk_animation', loaded['walk'])
director.set_editor_property('run_animation', loaded['run'])
# The supplied mesh has feet at local Z=0 and faces along +Y. The project capsule
# stands 48 cm above the floor and CharacterMovement advances along +X.
director.set_editor_property('mesh_relative_transform', u.Transform(
    location=u.Vector(0, 0, -48), rotation=u.Rotator(yaw=-90), scale=u.Vector(1, 1, 1)))
rows = d.save()
after_ids = sorted(str(a.world_id) for a in d.actors.get_all_level_actors()
                   if isinstance(a, u.CoastalWorldObject))
if before_ids != after_ids or before_saves != hashes():
    raise RuntimeError('Companion authoring changed campaign saves or persistent WorldObject IDs')
report = {
    'passed': True,
    'engine': u.SystemLibrary.get_engine_version(),
    'actors': rows,
    'assets': {key: value.get_path_name() for key, value in loaded.items()},
    'mesh_transform': {'location': [0, 0, -48], 'yaw': -90, 'scale': [1, 1, 1]},
    'policy': ('One transient standalone companion; existing pause UI supplies the command; '
               'no input mapping, inventory authority, campaign schema, or purchased asset redistribution.'),
    'runtime_verified': False,
}
OUT.write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report))
