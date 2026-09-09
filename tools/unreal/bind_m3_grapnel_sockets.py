"""Author contact sockets on the original private grapnel mesh."""
from pathlib import Path
import json
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
expected = ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject'
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != expected.resolve():
    raise RuntimeError('Independent host required')
if u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor():
    raise RuntimeError('End PIE before authoring sockets')
mesh = u.load_asset('/Game/Coastal/Activities/Rope/SM_Grapnel')
rows = []
for name, location in [('HookPoint', (20, 0, 0)), ('RopeEye', (-4.2, 0, 0))]:
    socket = mesh.find_socket(name)
    if not socket:
        socket = u.new_object(u.StaticMeshSocket, outer=mesh, name=name)
        socket.set_editor_property('socket_name', name)
        mesh.add_socket(socket)
    socket.set_editor_property('relative_location', u.Vector(*location))
    rows.append(dict(socket=name, location=location))
if not u.EditorAssetLibrary.save_loaded_asset(mesh):
    raise RuntimeError('Grapnel socket save failed')
(ROOT/'local-evidence/m3-grapnel-sockets.json').write_text(json.dumps(rows, indent=2))
print(rows)
