"""Author the bounded player-facing controls using inspected Mutable parameters."""
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[3]
if Path(u.Paths.convert_relative_path_to_full(u.Paths.get_project_file_path())).resolve() != (ROOT / 'LocalHost58/CoastalExploration/CoastalExploration.uproject').resolve():
    raise RuntimeError('Independent host required')
base = '/Game/Coastal/Character'
path = base + '/DA_CoastalCharacter'
factory = u.DataAssetFactory()
factory.set_editor_property('data_asset_class', u.CoastalCharacterDefinition)
asset = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset('DA_CoastalCharacter', base, u.CoastalCharacterDefinition, factory)
instance = u.load_asset(base + '/COI_CoastalDefault')
retarget = u.load_asset(base + '/Rigs/RTG_PlayerToMutable')
if not asset or not instance or not retarget:
    raise RuntimeError('Character definition dependencies unavailable')
controls = []
record = []


def choice(parameter, label, values, linked=''):
    c = u.CoastalAppearanceControl()
    c.set_editor_property('parameter', parameter)
    c.set_editor_property('linked_parameter', linked)
    c.set_editor_property('label', u.Text(label))
    c.set_editor_property('kind', u.CoastalAppearanceField.CHOICE)
    c.set_editor_property('choices', values)
    controls.append(c)
    record.append(dict(parameter=parameter, label=label, choices=values, linked=linked))


def scalar(parameter, label, default):
    c = u.CoastalAppearanceControl()
    c.set_editor_property('parameter', parameter)
    c.set_editor_property('label', u.Text(label))
    c.set_editor_property('kind', u.CoastalAppearanceField.SCALAR)
    c.set_editor_property('minimum', 0.0)
    c.set_editor_property('maximum', 1.0)
    c.set_editor_property('step', 0.1)
    c.set_editor_property('default_selection', default)
    controls.append(c)
    record.append(dict(parameter=parameter, label=label, minimum=0, maximum=1, step=0.1, default=default))


choice('Body Type 1', 'Body type', ['Regular', 'Slim', 'Strong', 'Overweight'], 'Body Type 2')
scalar('Skin Tone', 'Skin tone', 5)
scalar('Aged', 'Age detail', 0)
choice('Head Accessories', 'Hair', ['None', 'Hair Zhen', 'Hair Nasim', 'Hair Aoi'])
choice('Eye Color', 'Eye color', ['BrownHazel', 'DarkBrown', 'BrownHoney', 'BlueGrey', 'BlueMarine', 'BlueSky', 'GreenEmerald', 'GreenOlive'])
choice('EyeType', 'Eye shape', ['A', 'B', 'C'])
choice('NoseType', 'Nose shape', ['A', 'B', 'C'])
choice('LipsType', 'Lip shape', ['A', 'B', 'C'])
choice('ChinType', 'Chin shape', ['A', 'B', 'C'])
choice('Shirts', 'Shirt', ['TShirt', 'TankTop'])
choice('TShirt Colors', 'T-shirt color', ['Blue', 'Black', 'Green', 'Grey', 'Red', 'Yellow'])
choice('Jacket', 'Jacket', ['Jacket_A', 'Jacket_B', 'None'])
choice('Pants', 'Trousers', ['Jeans', 'Chinos', 'Pants'])
choice('Chinos Base Color', 'Chino color', ['Light Brown', 'Brown', 'Dark Brown', 'Blue'])
choice('Trousers Color', 'Trouser color', ['Black', 'Beige', 'Blue'])
choice('Shoes', 'Footwear', ['Boots', 'Sneakers'])
asset.set_editor_property('appearance_version', 'coastal.human.v1')
asset.set_editor_property('default_instance', instance)
asset.set_editor_property('controls', controls)
rig_report = json.loads((ROOT / 'local-evidence/m3-character-retarget-rigs.json').read_text())
asset.set_editor_property('retargeters', {u.load_asset(row['skeleton']):retarget for row in rig_report['sources']})
u.EditorAssetLibrary.save_loaded_asset(asset)
output = dict(asset=asset.get_path_name(), default=instance.get_path_name(), controls=record, retarget=retarget.get_path_name())
(ROOT / 'local-evidence/m3-character-definition.json').write_text(json.dumps(output, indent=2))
print('CHARACTER_DEFINITION', len(controls), 'controls saved')
