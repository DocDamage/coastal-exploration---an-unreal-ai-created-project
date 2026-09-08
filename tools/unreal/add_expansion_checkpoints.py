"""Add query-only dry arrival checkpoints; retain all seven saved world IDs."""
import json
from pathlib import Path
import unreal as u

actors=u.get_editor_subsystem(u.EditorActorSubsystem)
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
if world.get_name()!='L_FirstSignal' or level.is_in_play_in_editor():
    raise RuntimeError('Expected First Signal outside PIE')
places=[('Hallsands',(-6500,22000,94)),('Baelo Claudia',(8000,20000,428)),
        ('Powell Dock',(25500,4000,300)),('Industrial Harbour',(28200,23000,350)),
        ('Abandoned Sea Platform',(38600,-14000,800)),('Overlook campsite',(7600,9100,1220))]
existing={str(t):a for a in actors.get_all_level_actors() for t in a.tags}
rows=[]
for name,ground in places:
    tag='Coastal.Expansion.Checkpoint.'+name
    center=u.Vector(ground[0],ground[1],ground[2]+98)
    a=existing.get(tag)
    if not a:
        a=actors.spawn_actor_from_class(u.CoastalSafetyVolume,center)
        a.tags=[u.Name(tag)]
    a.set_actor_label(name+' dry arrival')
    a.set_folder_path('Coastal Expansion/Recovery')
    a.set_actor_location(center,False,True)
    a.set_editor_property('kind',u.CoastalSafetyKind.DRY_CHECKPOINT)
    a.get_editor_property('bounds').set_box_extent(u.Vector(180,180,110),False)
    a.get_editor_property('return_point').set_world_location(center,False,False)
    # A named arrival sign is visible to the player; the checkpoint actor stays query-only.
    sign_tag=tag+'.Sign'
    sign=existing.get(sign_tag)
    if not sign:
        sign=actors.spawn_actor_from_class(u.TextRenderActor,u.Vector(ground[0],ground[1]-250,ground[2]+180),u.Rotator(yaw=-90))
        sign.tags=[u.Name(sign_tag)]
    sign.set_actor_label(name+' arrival sign')
    sign.set_folder_path('Coastal Expansion/Wayfinding')
    text=sign.get_component_by_class(u.TextRenderComponent)
    text.set_text(name)
    text.set_world_size(35)
    text.set_text_render_color(u.Color(235,220,180,255))
    text.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
    rows.append({'name':name,'center':list(center.to_tuple()),'checkpoint':a.get_path_name()})
if not level.save_current_level():
    raise RuntimeError('Checkpoint save failed')
(Path('F:/coastline/local-evidence')/'m3-expansion-checkpoints.json').write_text(json.dumps(rows,indent=2))
print(json.dumps(rows))
