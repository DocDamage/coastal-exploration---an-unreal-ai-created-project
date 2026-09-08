"""Set the generated M2 map to dynamic lighting before its first composition review."""
import unreal as u
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if level.is_in_play_in_editor() or list(u.EditorLoadingAndSavingUtils.get_dirty_map_packages()):
    raise RuntimeError('Preserve unsaved editor work before finishing M2 lighting')
if not level.load_level('/Game/Coastal/Maps/L_FirstSignal'):
    raise RuntimeError('M2 has not been assembled')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
for cls, component in [(u.DirectionalLight,u.DirectionalLightComponent),(u.SkyLight,u.SkyLightComponent)]:
    for actor in u.GameplayStatics.get_all_actors_of_class(world,cls):
        actor.get_component_by_class(component).set_mobility(u.ComponentMobility.MOVABLE)
world.get_world_settings().set_editor_property('force_no_precomputed_lighting', True)
if not level.save_current_level():
    raise RuntimeError('M2 lighting save failed')
u.log('COASTAL_M2_DYNAMIC_LIGHTING_SAVED')
if __name__ == '__main__':
    u.SystemLibrary.quit_editor()
