# First Signal inventory icons

Original project artwork: a radio battery and glass marine fuse, using the
coastal UI's cream, teal and amber palette. No vendor art or external images.

Regenerate the transparent 256 x 256 PNGs with `python tools/create_item_icons.py`
(Pillow required). Run `tools/unreal/import_item_icons.py` inside the local Unreal
host outside PIE to import them under `/Game/Coastal/UI/Icons`, set UI texture
compression/no mipmaps and add their packaging directory to the host config.
The importer preserves unrelated packaging settings. Native inventory buttons
keep their item names when an optional icon is absent.
