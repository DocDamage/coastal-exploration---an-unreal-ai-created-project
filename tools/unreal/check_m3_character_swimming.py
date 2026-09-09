"""Run the established swimming route while observing the generated character presentation."""
import json
import time
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn = u.GameplayStatics.get_player_character(world, 0)
creator = pawn.get_component_by_class(u.CoastalMutableCharacterComponent)
swim = pawn.get_component_by_class(u.CoastalSwimmingComponent)
body = creator.get_generated_body()
if not body:
    raise RuntimeError('Generated character required')
script = Path(__file__).with_name('check_swimming_route.py')
route = {'__file__':str(script)}
exec(compile(script.read_text(), str(script), 'exec'), route)
samples = []
started = time.monotonic()
report = ROOT / 'local-evidence/m3-character-swimming.json'


def tick(delta):
    error = None
    try:
        if creator.get_generated_body() != body or not creator.is_appearance_ready() or body.get_editor_property('hidden_in_game'):
            raise RuntimeError('Generated appearance disappeared during route')
        if swim.is_surface_swimming():
            samples.append(dict(source=pawn.mesh.skeletal_mesh_asset.get_path_name(),
                                hand=list((body.get_socket_location('hand_r')-pawn.get_actor_location()).to_tuple())))
        if not route['finished']:
            return
        result = json.loads(route['REPORT'].read_text())
        if not result['passed'] or len(samples) < 5:
            raise RuntimeError('Swimming route failed or had no generated swim samples: ' + str(result.get('error')))
    except Exception as exc:
        error = str(exc)
        if not route['finished']:
            route['finish'](error)
    u.unregister_slate_post_tick_callback(handle)
    report.write_text(json.dumps(dict(passed=error is None, error=error, swim_samples=len(samples),
        sample=samples[::max(1,len(samples)//8)], seconds=time.monotonic()-started), indent=2))


handle = u.register_slate_post_tick_callback(tick)
