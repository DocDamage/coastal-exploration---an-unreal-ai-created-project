"""Use the installed requested DayNightSystem with the coast's existing light/fog."""
import json
import sys
from pathlib import Path
import unreal as u

sys.path.insert(0, str(Path(__file__).parent))
from m3_destination_authoring import Destination

d = Destination('day_night')
all_actors = d.actors.get_all_level_actors()
suns = [a for a in all_actors if isinstance(a, u.DirectionalLight)]
fogs = [a for a in all_actors if isinstance(a, u.ExponentialHeightFog)]
if len(suns) != 1 or len(fogs) != 1:
    raise RuntimeError('Expected exactly one existing sun and one existing fog; refusing ambiguous ownership')
manager_class = getattr(u, 'DayNightManager', None)
if not manager_class:
    raise RuntimeError('Enable installed DayNightSystem and restart this host first')
managers = [a for a in all_actors if isinstance(a, manager_class)]
if managers and any(not any(str(t).startswith(d.prefix) for t in a.tags) for a in managers):
    raise RuntimeError('Another day/night manager already owns the scene')
sun, fog = suns[0], fogs[0]
light = sun.get_component_by_class(u.DirectionalLightComponent)
fog_component = fog.get_component_by_class(u.ExponentialHeightFogComponent)
manager = d.actor(manager_class, 'Coastal daylight', (0, 0, 3000))
manager.set_actor_hidden_in_game(True)
manager.set_actor_enable_collision(False)
# Day values retain authored brightness/fog. Start each play session at 10:00;
# world-clock campaign persistence will be integrated with destination/combat state.
settings = {
    'sun_light_actor': sun, 'fog_actor': fog,
    'current_time_of_day': 10.0, 'cycle_enabled': True,
    'real_seconds_per_game_minute': 2.0, 'time_scale_multiplier': 1.0,
    'sunrise_hour': 6.0, 'sunset_hour': 18.0, 'twilight_blend_hours': 1.0,
    'day_sun_intensity': light.get_editor_property('intensity'),
    'night_sun_intensity': max(.15, light.get_editor_property('intensity') * .04),
    'day_fog_density': fog_component.get_editor_property('fog_density'),
    'night_fog_density': fog_component.get_editor_property('fog_density') * 1.2,
}
manager.modify()
for key, value in settings.items():
    manager.set_editor_property(key, value)
light.set_mobility(u.ComponentMobility.MOVABLE)
rows = d.save()
report = {'plugin': 'DayNightSystem', 'sun': sun.get_path_name(), 'fog': fog.get_path_name(),
          'minutes_per_day_real': 48, 'start_hour': 10, 'campaign_time_persistent': False,
          'actors': rows, 'runtime_verified': False}
Path('F:/coastline/local-evidence/m3-day-night-authoring.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps({k: v for k, v in report.items() if k != 'actors'}))
