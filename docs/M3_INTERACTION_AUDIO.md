# M3 interaction and menu audio

September 9 checkpoint: all ten selected clips are imported and saved in the
independent UE5.8.2 host. The native build and all 54 native tests pass (53 clean,
one retained AGIS warning). The final disposable campaign
`coastal_test_m3_audio_0909e` passes 69 gameplay assertions and four mixer captures.
Effects mute and Master mute both record zero signal; Effects 50% records
0.4988 times the baseline RMS. These are scripted/game-output checks, with
listening, tonal balance, physical controls and packaged acceptance still open.

The first ordered batch selects ten WAVs from Fantasy UI Essentials, CANDLE LIGHT
Horror Interaction and CANDLE LIGHT Bonus Vol.02. The manifest is
`data/m3_interaction_audio.json`. Raw WAVs and Unreal SoundWaves stay in the
private workspace/host. Bonus Vol.01 remains reserved for tonal review.

`tools/stage_m3_interaction_audio.py` checks PCM format, duration, nonzero signal
and peaks, and records exact archive/source hashes. It preserves included license
documents under the private staging directory. `tools/unreal/import_m3_interaction_audio.py`
checks host identity, refuses existing destination assets, imports the selected
clips and saves their volume, Effects class and PlayWhenSilent settings.
The existing `/Game/Coastal/Audio` cook rule includes the new directory; a real
Windows cook/package remains a separate gate.

The existing `CoastalActionAudio` owner now consumes contextual feedback from the
interaction bridge and native UI. Successful pickups/transfers, storage, notes,
records and doors select their own cues after the authoritative operation returns.
Repeated/blocked input and AlreadyApplied results are quiet. Errors use a distinct
cue. Native menu opening, explicit Back, item/record selection and setting changes
emit feedback only from the existing UI owner. Automatic storage/transcript/journal
panels do not also emit a menu-open cue.

One transient feedback voice replaces the previous one. Door sounds use distance
attenuation at the interaction point; interface cues remain local and audible
during menu pause. The owner retires finished sources without retry and stops old
voices on campaign changes, recovery, routing release and owner teardown. All
feedback uses the existing Effects class, with Master applied by the existing
options owner. No campaign/preferences schema, inventory or save authority changed.

`tools/unreal/check_m3_interaction_audio.py` exercises a disposable campaign,
actual AGIS pickups/transfers, native menus, door events, cancellation, reload and
mixer captures. Component submission counts are diagnostics, not proof of sound
at the speakers. Listening, final tonal balance, physical controls and packaged
acceptance must be recorded separately.

## Credits and provenance

CANDLE LIGHT supplies the Horror Interaction and Cozy Everyday Objects effects.
Fantasy UI Essentials supplies the menu/item cues. The two included licenses allow
project use/modification, prohibit standalone redistribution and make credit optional.
The bonus ZIP has no included license/readme; its user-supplied provenance is
recorded, and delivery provenance review remains open. Original files must never
be published as part of this source repository.

Local evidence uses `m3-interaction-audio-*` under `../local-evidence`.
The live harness temporarily sets `au.DisableAppVolume 1` and solos the PIE
audio device to capture a background editor. It restores the prior console
value and clears the solo afterward. Coastal's actual Master/Effects mix stays
active throughout. Earlier fully silent captures remain failed evidence, not
mute acceptance. Run `tools/check_m3_feedback_recordings.py` after the live check
to verify nonzero baseline, mute and half-gain output independently.

The first HDD build attempt was stopped for cache relocation; it is retained as
`m3-interaction-audio-hdd-build-stopped.*`. Only generated PCH cache moved to the
SSD. Its original directory is retained beside the junction, and the copy is
SHA256-verified in `m3-ue58-ssd-pch-0909.json`.
