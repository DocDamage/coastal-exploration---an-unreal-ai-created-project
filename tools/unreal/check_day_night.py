"""Verify the installed clock drives real lighting and respects game pause in PIE."""
import json
import time
from pathlib import Path
import unreal as u

REPORT = Path('F:/coastline/local-evidence/m3-day-night-live.json')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
managers = u.GameplayStatics.get_all_actors_of_class(world, u.DayNightManager)
saves = [s for s in u.ObjectIterator(u.CoastalSaveCoordinator)
         if 'UEDPIE_' in s.get_path_name() and s.has_active_campaign()]
if len(managers) != 1 or len(saves) != 1 or not str(saves[0].get_active_save_set()).startswith('coastal_test_'):
    raise RuntimeError('Requires one clock and an active disposable test campaign')
manager = managers[0]
ui = u.GameplayStatics.get_player_controller(world, 0).get_component_by_class(u.CoastalUISessionComponent)
if ui.has_modal():
    raise RuntimeError('Close the test campaign menu before checking the clock')
light = manager.get_editor_property('sun_light_actor').get_component_by_class(u.DirectionalLightComponent)
fog = manager.get_editor_property('fog_actor').get_component_by_class(u.ExponentialHeightFogComponent)
original_hour = manager.get_current_time_of_day()
original_enabled = manager.get_editor_property('cycle_enabled')
rows = []
phase = 0
began = time.monotonic()
deadline = began + 3
manager.resume_cycle()
start_hour = manager.get_current_time_of_day()
done = False


def finish(error=None):
    global done
    if done:
        return
    done = True
    u.unregister_slate_post_tick_callback(handle)
    manager.set_time_of_day(original_hour)
    if original_enabled:
        manager.resume_cycle()
    else:
        manager.pause_cycle()
    REPORT.write_text(json.dumps({'passed': error is None, 'error': error, 'rows': rows,
                                 'campaign_time_persistent': False}, indent=2), encoding='utf-8')


def tick(delta):
    global phase, deadline, paused_hour
    try:
        now = time.monotonic()
        if now - began > 30:
            raise RuntimeError('Day/night verification timed out')
        if now < deadline:
            return
        if phase == 0:
            if manager.get_current_time_of_day() <= start_hour:
                raise RuntimeError('Clock did not advance in gameplay')
            rows.append({'case': 'clock_advances', 'passed': True})
            manager.pause_cycle()
            manager.set_time_of_day(12)
            noon = float(light.get_editor_property('intensity'))
            noon_fog = float(fog.get_editor_property('fog_density'))
            manager.set_time_of_day(0)
            midnight = float(light.get_editor_property('intensity'))
            midnight_fog = float(fog.get_editor_property('fog_density'))
            if noon <= midnight or midnight_fog < noon_fog:
                raise RuntimeError('Day/night time did not change the bound scene lighting')
            rows.append({'case': 'bound_sun_and_fog', 'passed': True,
                         'noon_intensity': noon, 'midnight_intensity': midnight,
                         'noon_fog': noon_fog, 'midnight_fog': midnight_fog})
            manager.set_time_of_day(original_hour)
            manager.resume_cycle()
            ui.open_pause()
            paused_hour = manager.get_current_time_of_day()
            phase, deadline = 1, now + 2
        elif phase == 1:
            if not u.GameplayStatics.is_game_paused(world) or abs(manager.get_current_time_of_day() - paused_hour) > .0001:
                raise RuntimeError('Clock advanced while the game was paused')
            rows.append({'case': 'native_menu_pauses_world_clock', 'passed': True})
            panels = [p for p in u.WidgetLibrary.get_all_widgets_of_class(world, u.CoastalPanelWidget, False)
                      if p.is_in_viewport()]
            if len(panels) != 1:
                raise RuntimeError('Ambiguous native menu')
            panels[0].call_method('Execute', args=('back',))
            phase, deadline = 2, now + 2
        elif phase == 2:
            if ui.has_modal() or manager.get_current_time_of_day() <= paused_hour:
                raise RuntimeError('Clock did not resume with gameplay')
            rows.append({'case': 'native_menu_resume', 'passed': True})
            finish()
    except Exception as exc:
        finish(str(exc))


REPORT.write_text(json.dumps({'passed': False, 'status': 'running'}), encoding='utf-8')
handle = u.register_slate_post_tick_callback(tick)
print('Day/night check running')
