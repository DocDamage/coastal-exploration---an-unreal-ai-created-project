# M3 destination music and thunder

The independent UE5.8 host uses seven selected score tracks and two distant thunder
clips from the supplied music/thunder archives. The exact selections, gains and
private import destination are in `data/m3_soundscape.json`. Original source audio,
converted WAVs and included documentation stay under `../LocalVendor/M3Soundscape`.
Purchased payloads are not part of this repository.

Source credits: **CANDLE LIGHT**, Fantasy Workshop & Crafting Music; **CDanSantana**,
Thunder Elements; the user-supplied Complete Game Music Library and `horror music`
archives. The Complete library includes a commercial-use license with optional
attribution. Workshop archives include a README; the horror and thunder archives
have no license text in their inventories. Their supplied provenance remains in
the local asset register for the later distribution review.

`CoastalSoundscape` is a controller presentation component initialized after the
existing UI and audio options. It owns at most one score voice and one thunder
voice. The original coastal ambience and transcript owner remain active. Music
uses the existing Ambience bus, labelled **Music & ambience** in settings; thunder
uses Effects. Master scales both through the existing options owner. No settings,
inventory, quest or campaign schema changes are introduced.

## Destination selection

| Location authority | Score | Entry radius |
|---|---|---|
| Open coast / outside destination regions | Beyond the Hill | Fallback |
| Cabin radio | Quiet Corner | 11 m |
| Village register | Village Square | 36 m |
| Existing campsite marker | Quiet Road | 7 m |
| Powell work order | Quiet Smithy | 22 m |
| Prison record / station log | Dark Ambient | 25 m / 23 m |
| Baelo tablet / Atlantis survey | Buried Shrine | 24 m / 45 m |

The component resolves actual authored actors once at initialization. Missing or
duplicate anchors refuse the optional score; a healthy campaign remains usable.
Regions retain their score for an additional four metres at the boundary to avoid
rapid switching. A two-second fade to silence precedes the new score's two-second
fade in. Rapid travel chooses the latest destination without accumulating voices.

Loops retain their source duration and musical timing. Private conversion decodes
OGG/PCM24 to PCM16 and applies ten-millisecond ramps at both music edges to remove
sample discontinuities. The long prison track is looped with the same treatment;
this does not establish that its musical seam or tonal fit has passed listening.

## Thunder and lifetime

The first distant thunder event is scheduled after 20 seconds of active gameplay;
subsequent starts are spaced 55–85 seconds apart, with no overlapping thunder.
The supplied far/soft clips retain approximately 5.56/7.19 seconds of silent
pre-roll; audible thunder follows that delay. Mixer probes seek to eight seconds
to measure the signal, while gameplay scheduling plays each complete source.
An actual upward Visibility trace samples overhead cover twice per second. Under
cover thunder smoothly falls to 25% gain and a 1.8 kHz low-pass; open sky restores
full authored gain and an 18 kHz cutoff. This broad cover approximation can also
respond to dense collision geometry. Thunder is a local environmental layer,
not a positional strike or weather simulation.

Menus, transcript presentation and recovery pause both voices and their timers.
Campaign epoch changes destroy old voices and reset scheduling. Routing release,
device change, pawn replacement, component unregistration and teardown retire
owned sources. Failed/dropped music is not polled and restarted. No playback event
writes saves or advances gameplay.

## Reproduction and validation

1. `python tools/stage_m3_soundscape.py` (NumPy and SoundFile required) stages the manifest selection and preserves
   original hashes, converted hashes, formats and signal measurements.
2. Outside PIE with no dirty packages, run `tools/unreal/import_m3_soundscape.py`
   through the identity-checked MCP client. It refuses to overwrite existing clips.
3. Sync source, close the editor and run the independent UE5.8 build. Run all native
   Coastal tests and `tools/unreal/check_m3_soundscape.py` in a fresh disposable PIE
   campaign. The existing `/Game/Coastal/Audio` cook directory includes these clips.

The native build and **56 native tests** pass, along with **70 Python tests**.
The final `coastal_test_m3_soundscape_0909e` PIE campaign passes **49 assertions**:
all score regions, single-voice bounds, pause/resume, real collision cover,
thunder scheduling, recovery, campaign reload and synchronous routing teardown.
All **seven actual mixer checks** pass: score/thunder have nonzero signal, routed
mutes are silent, and Music 50% measures **0.4995** of baseline RMS. Score captures
span a complete loop. Captures use the main mixer with unrelated channels/sources
silenced temporarily; thunder signal measurements start after its source pre-roll.

All **82 pre-existing saves**, both host maps and project descriptors are unchanged
(86 SHA256 comparisons). Editor is outside PIE with 2,934 actors and no dirty
packages. Normal app-volume and background-throttle settings are restored.
Reports are under `../local-evidence/m3-soundscape-*`; earlier failed probe reports
remain retained separately. Changes are uncommitted. Listening, physical device
controls, performance and Windows-package acceptance remain separate gates.
