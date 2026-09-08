"""Check a disposable active PIE campaign for redundant saves across two menus."""
import json
import time
from pathlib import Path
import unreal as u

w = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
pawn = u.GameplayStatics.get_player_character(w, 0)
pc = u.GameplayStatics.get_player_controller(w, 0)
ui = pc.get_component_by_class(u.CoastalUISessionComponent)
saves = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
         if 'UEDPIE_' in s.get_path_name() and s.has_active_campaign()][0]
point = u.Vector(7600, 9100, 1318)
if not u.CoastalPlacementLibrary.is_dry_destination(pawn, u.Transform(location=point)):
    raise RuntimeError('Campsite is not dry')
pawn.get_component_by_class(u.CharacterMovementComponent).stop_movement_immediately()
pawn.set_actor_location(point, False, True)
rows = []
phase, deadline = 0, time.monotonic()+5


def tick(delta):
    global phase, deadline
    if time.monotonic() < deadline:
        return
    rows.append({'phase': phase, 'generation': saves.get_generation(),
                 'location': list(pawn.get_actor_location().to_tuple())})
    if phase in (0, 2):
        ui.open_pause()
    elif phase in (1, 3):
        panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(w, u.CoastalPanelWidget, False)
                  if p.is_in_viewport()]
        panels[0].call_method('Execute', args=('back',))
    elif phase == 4:
        u.SystemLibrary.execute_console_command(w,
            'Shot SHOWUI filename=F:/coastline/local-evidence/m3-checkpoint-after-menus.png', pc)
    else:
        u.unregister_slate_post_tick_callback(handle)
        result = {'observations': rows, 'stable_generation': len({r['generation'] for r in rows}) == 1,
                  'scope': 'Stationary campsite, two pause/back cycles and ordinary screenshot; active disposable PIE campaign.'}
        Path('F:/coastline/local-evidence/m3-checkpoint-live-regression.json').write_text(json.dumps(result, indent=2))
        return
    phase += 1
    deadline = time.monotonic()+4


handle = u.register_slate_post_tick_callback(tick)
print('Checkpoint regression queued: two pause/back cycles plus capture.')
