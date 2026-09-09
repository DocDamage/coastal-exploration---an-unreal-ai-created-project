# M3 additional assets — September 8, 2026

## September 9 interaction/audio checkpoint

The first ordered increment now imports ten selected clips from three archives
and connects them to the existing UI/interaction/audio owners. All 54 native
tests, 69 scripted gameplay assertions and the mixer mute/half-gain checks pass.
See [interaction audio](M3_INTERACTION_AUDIO.md) and `data/m3_interaction_audio.json`.
Bonus Vol.01 remains reserved for tonal review; no cartoon cue is forced into
the coast. Listening/tonal and physical-device acceptance remain open.

The remaining twelve archives are still not imported. Continue with music and
thunder, then merchant animation, isolated pickup/rotation review and fishing/
combat audio. The encrypted merchant ZIP and readable FBX alternative are unchanged.
The original inventory paragraphs below describe the September 8 planning state.

User-selected source folder: `G:/coastline/even newer assets and animations`.
All 16 ZIP directories were inspected without extracting or importing content.
These are planned additions, not completed integration or gameplay acceptance.
The archive inventory is recorded in `data/m3_new_asset_batch.json`.

## Integration order

1. **Interaction and UI audio.** Audition and select a small consistent set for
   existing menus, inventory, cabin objects, doors and investigation records.
   Bind playback to successful actions through the existing audio and interaction
   owners. Keep error/cancel cues distinct and avoid double playback.
2. **Destination music and coastal weather audio.** Select restrained exploration,
   village, camp, prison and ruins tracks; add appropriate thunder ambience.
   Verify loop boundaries, fades, indoor attenuation and music/effects volume
   controls. Thunder audio alone does not implement a weather simulation.
3. **Merchant animation.** Inspect skeletons and retarget selected idle, bartering
   and transition clips for an authored village or harbour merchant presentation.
   Connect animation to interaction state and restore idle after interruption.
   This pack does not by itself supply a trading economy or crafting system.
4. **Pickup and rotation inspection.** Review the supplied Blueprint project in
   isolated staging, then integrate a bounded inspect/rotate action for selected
   props using the current Hyper interaction, AGIS inventory and UI/input owners.
   Preserve item identity and ownership; prohibit duplicate pickups or a second
   save/inventory authority. Restore camera/input on close, pause and recovery.
5. **Fishing and combat audio.** Bind suitable fishing cues to implemented fishing
   events and combat cues to confirmed gameplay events. Where an event/system is
   still absent, track its integration explicitly rather than treating the audio
   pack as a completed mechanic. Audition stylized sounds for tonal fit.

## Selected packs and intended use

| Archives | Observed payload | M3 work |
|---|---|---|
| `CANDLE_LIGHT_Horror_Interaction_SFX_100_Sounds.zip` | 100 WAV files | Doors, handles and investigation props in prison, ruins and darker interiors. |
| `Candle_Light_Bonus_Vol_01_FINAL_30_SFX.zip` | 30 WAV files; cartoon/motion examples | Audition for occasional light interactions; select only cues that fit the coast. |
| `Candle_Light_Bonus_Vol_02_FINAL_30_SFX.zip` | 30 WAV files; everyday object examples | Cabin drawers, boxes, paper, lamps and other existing interactions. |
| `Fantasy UI Essentials - 100 SFX.zip` | 100 WAV files | Menu focus/confirm/cancel, inventory and journal feedback. |
| `Complete_Game_Music_Library_100_Seamless_Loops.zip` | 100 OGG files | Exploration, menu and destination music selection and looping. |
| `Fantasy_Workshop_Crafting_Music_100_Part_1.zip` through `Part_4.zip` | 25 WAV files per archive; 100 total | Forge/workshop, alchemy, village market and traveller camp music candidates. No new crafting mechanic is implied by track names. |
| `horror music.zip` | 6 WAV files | Restrained prison/ruins tension and transitions back to open-coast music. |
| `Thunder_Elements_-_CDanSantana_Itch_IO.zip` | 51 WAV files, 1 MP3, metadata and artwork | Coastal thunder selection, distance variation and indoor/outdoor mix. Audition MP3 before classifying it as usable game audio. |
| `Fishing_Minigame_SFX_120_Sounds.zip` | 120 WAV files | Cast, reel, catch/reward and failure cues where supported by the fishing implementation. |
| `Pixel_Combat_SFX_200_Full.zip` | 200 WAV files | Audition combat impacts and feedback against the existing combat presentation. |
| `MerchantVendorAnim-V1.1.zip` | 100 UAssets, 1 map, project/config files; all 108 entries encrypted | Native merchant animation package; payload/engine compatibility inspection awaits readable staging. |
| `MerchentVendorAnimFBXFiles.zip` | 54 FBX files with UE4 animation paths observed | Readable alternative for skeleton inspection and retargeting; establish actual clip/mesh counts and UE5 compatibility before import. |
| `Pickupable&RotatableItems.zip` | 239 UAssets, 10 FBX files, 1 map and project/config files | Item inspection/rotation integration. Project declares UE5.6 and GameplayStateTree; review dependencies and Blueprints before adapting to the UE5.8.2 trial. |

Counts describe archive entries, not unique usable assets, validated loops or
successful imports. Original ZIPs remain intact. Included license/readme files
are identified in the inventory; carry applicable attribution into the project
credits during integration. Missing documentation remains a provenance task.

## Acceptance and scope

Use the independent UE5.8.2 trial with a map backup and disposable campaigns.
Import selected content into private host folders; keep purchased payloads out of
the public source repository. Preserve the UE5.7 rollback host and existing saves.

For audio, verify actual playback, event timing, loop/fade transitions, volume and
mute controls, pause/resume, cancellation, spatial attenuation and no duplicate
voices. For animation, verify skeleton/retargeting, feet/contact, transitions and
interruption cleanup in gameplay. For item inspection, test keyboard and controller
rotation, focus/camera restoration, distance/line-of-sight rules, inventory
ownership and save/reload without duplication or loss.

After integration, repeat affected native/gameplay regressions, profile audio and
animation cost, and verify cooked asset references in a Windows package. These
additions join the remaining M3 art polish, physical-device and performance gates;
the previously passed swimming, shelter, companion and destination checks remain
recorded separately. Packaging is still an outstanding release acceptance step.
