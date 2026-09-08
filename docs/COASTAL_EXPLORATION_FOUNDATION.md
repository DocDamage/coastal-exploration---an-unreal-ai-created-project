# Coastal Exploration — Foundation and First Playable Plan

**Decision date:** September 5, 2026  
**Working project label:** Coastal Exploration  
**Opening chapter:** First Signal  
**Delivery status:** Source starter and implementation plan; no playable build yet.

## 1. Decisions now locked

The user selected coastal exploration, third-person, and no co-op, and delegated remaining starting choices. Carry these decisions forward without repeating those questions.

| Decision | Foundation choice |
|---|---|
| Experience | Grounded coastal exploration with a light mystery. |
| Perspective | Third-person, visible player character. |
| Players | Single-player only. No online accounts, lobby, network architecture, or co-op backlog in v0.1. |
| Engine | Unreal Engine; select the exact version after the core asset compatibility test. |
| Platform | Windows desktop first. |
| Input | Keyboard/mouse and gamepad, including menus. |
| World structure | One compact, handcrafted coastal area; no procedural-world requirement. |
| Progression | Discover useful equipment and clues, restore access, reach new places. |
| Opening objective | Recover a radio battery and marine fuse, restore the cabin radio, receive a lead. |
| Tone | Solitude, weathered infrastructure, curiosity, and a sense of place. Not automatically horror or post-apocalyptic. |
| First-build exclusions | Combat, enemies, hunger, thirst, crafting trees, construction, swimming, boats, dynamic tides, and an economy. |
| Graphics goal | Readable grounded art, stable camera, and coherent lighting before spectacle. |

All design numbers in this document are starting targets, not measured results or third-party product specifications.

## 2. What the player is doing

The player is reopening a neglected seasonal cabin and exploring its surrounding bay. The cabin is a reliable point of return: storage, journal review, and the radio all live there. Old waterfront facilities provide the first practical reason to leave it.

The playable loop is:

**Choose a lead at the cabin → follow the shoreline → discover equipment or information → return with a useful result → gain a new lead or access opportunity.**

Avoid making exploration a filler activity between maintenance bars. Walking to a new place must offer navigational decisions, a useful object, a view, a clue, or a route change. A large empty coastline is not an improvement over a small coherent one.

The mystery can begin with a mundane inconsistency: a relay at a supposedly decommissioned outer platform remains active. This does not establish supernatural forces, robots, combat, or a science-fiction plot. Those directions have not been selected.

## 3. Evidence and source boundaries

The supplied inventory reports 63 unique Cosmos assets and 41 Unity assets, with Unity limited to 2D, 3D, and Audio categories. Treat it as the user's inventory record, not proof of every package's installed version or API. The full supplied text is retained in `inventory_user_supplied.md`.

The separate Fab links supplied earlier establish additional user-identified assets, including the Nordic cabin and Advanced Grid Inventory System. Product pages can support advertised contents; they cannot prove local compatibility, animation quality, runtime performance, or a successful integration.

Selected checks made for this package:

- Epic's standard Third Person template provides a movable, jumping character and basic level geometry. Use the standard variant, not a combat sample, as this game's foundation. [S1]
- Hyper Interaction advertises interface-based interactions and prompts; its checked listing names UE 5.7. That is why 5.7 is a compatibility-test candidate, not a guarantee. [S2]
- AGIS advertises modular grid inventories and containers. It is the proposed single inventory authority; its actual local API is not available here. [S3]
- The Nordic cabin listing includes an assembled hut/dock level instance and a radio. It also distinguishes its included presentation setup from excluded Fluid Flux/Ultra Dynamic Sky effects. [S4]
- Ultimate Fishing Megapack advertises dock/fishing props and animated fish. That does not establish a complete fishing mechanic. [S5]

Everything else presented as a game rule, architecture choice, quest item, map layout, or milestone is an original proposed implementation for this project.

## 4. Asset selection: use a few packs well

| Asset | v0.1 role | Boundary |
|---|---|---|
| Nordic Fishing Hut | Cabin, radio, storage presentation, immediate dock area. | Inspect actual contents, collision, material dependencies, and performance. |
| Ultimate Fishing Megapack | A curated subset of dock modules and waterfront dressing. | No claim of ready-to-play fishing or driveable boats. |
| Coastal Wetland & Railroad Bridge | Candidate trail scenery and rail-overlook landmark. | Listed in the user's inventory; actual selection and import remain untested. |
| Hyper Scalable Interaction System V4 | Detect/select interactions and provide input prompts. | Connect its real interface to game-owned actions; do not invent API names. |
| Advanced Grid Inventory System | Items, grid occupancy, inventory and storage transfers. | Only inventory authority; no parallel prototype inventory in production. |
| Hyper Outliner and Symbol System V3 | Optional restrained interaction emphasis. | Evaluate after the core interaction loop works. |
| Hyper Footstep System V4 | Candidate surface footsteps. | Add only after character and surfaces are settled. |
| Hyper Mesh to Icon Creator V4 | Candidate consistent icon-generation workflow. | Verify installed version and workflow. |

Keep Industrial Harbour for a later district and Abandoned Sea Platform for a later destination. A distant landmark is optional in v0.1; do not import an entire expensive environment merely to show a silhouette. Keep camping animations for a subsequent animation/activities pass unless one is immediately suitable.

Do not mix realistic cabin art with the stylized fantasy packs just because both are owned. Do not buy missing-looking assets before inspecting the existing packages.

**Unresolved visuals:** a final human character, the exact battery and fuse meshes, appropriate footsteps/ambience, and final inventory icons. A fuse box listed in a pack does not prove an individual fuse mesh exists. Explicitly labeled development proxies are allowed in the test room; they do not count as finished art.

## 5. First playable map

Use one persistent playable level for the opening. The separate systems test room remains a test map, not an additional player destination. Do not introduce a streaming architecture before this small map demonstrates a need for it.

### Four connected spaces

**Cabin Cove:** spawn outside the cabin facing a clear entrance and waterfront view. Place the radio and storage chest where a third-person camera can see them. Opening the door, inspecting the radio, and storing one object establish the basic verbs.

**Shoreline Trail:** connect cabin and dock with visible path edges, a small bend, and a landmark that makes the direction intelligible. Add one optional fork, not a maze. The route should still make sense with objective markers disabled.

**Old Dock:** place the required radio parts in two distinguishable, unlocked containers. Keep both reachable without a key, jumping challenge, or optional note. One container might be inside a maintenance shelter; the other can be under a covered workbench. Inventory capacity failures must leave the parts retrievable.

**Rail Overlook:** an optional side route with a postcard or maintenance observation and a view toward later destinations. Completing this route should not be mandatory for the radio objective.

### Initial layout targets

| Element | Starting target |
|---|---|
| Map planning footprint | Approximately 260 × 200 metres, including nonplayable scenery. |
| Total opening walking route | Approximately 500–700 metres including return travel and the optional fork; measure after blockout. |
| First-play duration | 15–25 minutes including searching and reading; not a timed mission. |
| Optional discoveries | One substantial side discovery. |
| Required item sources | Two guaranteed containers, with stable world IDs. |
| Traversable path width | Aim for at least 2.2 metres in routine third-person walking areas where the art allows. |
| Water | Visual shoreline and a forgiving recovery boundary; not a swimming system. |

`data/level_blockout.json` contains suggested centres in metres, not generated geometry. Convert to Unreal centimetres when placing actors. Its direct distances do not prove the target route length; bends and final traversal must be measured.

Do not stretch an entire cabin mesh to solve camera problems. Test the camera boom, collision, interior layout, and door clearances first.

## 6. Player, movement, and camera

Begin with the standard Third Person template's character while the gameplay loop is being assembled. This is explicitly temporary development presentation. A final character is a later art-integration requirement, not something already supplied by this starter.

Suggested tuning for the first movement pass:

| Parameter | Starting value or policy |
|---|---|
| Normal move speed | 330 cm/s. |
| Sprint speed | 520 cm/s; no stamina drain in v0.1. |
| Camera boom | Start around 300 cm and adjust for the cabin. |
| Horizontal field of view | Start around 85 degrees; make adjustable after testing. |
| Interaction reach | About 180 cm from the character, not from a camera poking through a wall. |
| Camera effects | No mandatory shake, motion blur, head bob, or forced cinematic camera changes. |
| Sprint input | Support hold/toggle choice. |
| Traversal | Walk, sprint, turn, modest jump; no vaulting, climbing, swimming, or ledge-hang system. |

Use a camera-facing target selection and a character-origin reach/visibility validation. Otherwise third-person distance can let the camera see a container the character cannot reach.

If the player enters deep water or falls below the valid map, fade out briefly and return to the last validated dry-ground checkpoint. Keep inventory and progress. Show a clear explanatory message. Check the destination is not obstructed and prevent a repeated recovery loop. The recovery logic is specified here but not implemented in the plugin.

## 7. Input and UI

Use action-based mappings and separate exploration/UI ownership. Epic's Enhanced Input supports mapping contexts and prioritization; our implementation should use those capabilities to prevent a menu input from also interacting with the world. [S6]

The full draft mapping is in `data/input_map.json`. Keyboard defaults are WASD, mouse look, Space jump, Shift sprint, E interact, Tab inventory, J journal, and Escape pause. Gamepad defaults use sticks for movement/look, face-bottom for jump, face-left for interaction, face-top for inventory, and View/Select for the journal.

The HUD needs only a small current-objective panel, the focused interaction prompt, and brief item/discovery feedback. Do not display health, stamina, hunger, cash, or ammunition for systems the game does not contain.

Required UI screens for the first playable are title/start/continue, pause, settings, inventory with storage view, and journal. These widgets are implementation work, not included binary assets.

Inventory gamepad support means selecting slots and executing inspect/transfer/back actions without a mouse. Drag-and-drop cannot be the only way to transfer an item. Focus must be restored when returning from a child view. Escape/B closes the topmost layer once, not every layer in a single input frame.

The journal stores the opening objective, the maintenance note, the optional discovery, and the final radio transcript. All essential information must be readable without audio. Text size, camera sensitivity, invert-Y, subtitles, and volume controls are part of the basic accessibility pass.

## 8. A deliberately small item set

Begin with four item definitions rather than inventing filler materials to populate an empty grid.

| Item ID | Use | Rules |
|---|---|---|
| `item.radio_battery` | Required repair item. | One guaranteed copy; consumed only on successful radio repair. |
| `item.marine_fuse` | Required repair item. | One guaranteed copy; consumed only on successful radio repair. |
| `item.dock_key` | Opens an optional locker. | Never gates required radio parts; not consumed. |
| `item.old_postcard` | Optional discovery. | Adds journal content; no mechanical power or quest prerequisite. |

Start by testing a 6 × 4 backpack grid and 8 × 6 cabin storage grid. These are design settings, not assumptions about AGIS defaults. Battery size is 1 × 2; the other items are initially 1 × 1. Change these only if the real inventory UI needs it.

Critical items cannot be discarded, sold, destroyed, or consumed by another action. They may be stored at the cabin, but repairs require them in the player's backpack. The HUD must recalculate readiness after a storage transfer so it does not keep saying “return to repair” when the player has stored the parts.

No random drops for required items. No carrying-weight simulation. No food/water item functionality until those actions have a designed purpose.

## 9. First Signal: exact progression

### Stage A — inspect

The radio is visible at the cabin. Inspecting it records `radioInspected`. A short interaction description names the missing battery and fuse and points toward the old dock.

### Stage B — recover equipment

The player can find either part before or after inspecting the radio. The note and dock trigger only enrich the journal. They never determine whether a legitimate item pickup counts.

### Stage C — return and repair

When both required items are in carried inventory, the objective changes to return to the radio. At the radio, a validated interaction asks the inventory adapter to commit one transaction: consume one battery and one fuse, all or none.

Only a `Committed` or valid `AlreadyCommitted` result changes `radioRepaired`. Missing/unwired inventory is an explicit development error, not success. This mission logic is represented in the included source.

### Stage D — receive the message

After repair, the player can listen to the signal. Show the full transcript. An explicit “continue” after the transcript is readable may count as completion even when the player skips audio. Do not gate progress on the existence or duration of a voice file.

### Stage E — complete and continue exploring

Record the North Reach lead in the journal, save the full campaign, and leave the player in the world. Do not load a nonexistent platform level or present a fake “travel” button. The outer station is a future lead, not completed content.

### Quest invariants

`messageHeard` implies `radioRepaired`. `radioRepaired` implies `radioInspected`. Optional facts are independent. Replaying the radio message does not consume anything or create duplicate journal entries. Revisiting the radio after repair never calls inventory consumption again.

The starter includes tests of these state rules. It does not prove that the actual AGIS implementation removes items atomically; that is a separate mandatory integration test.

## 10. One owner per system

| Responsibility | Owner |
|---|---|
| Character movement/camera | Project character and controller, based on standard Third Person template. |
| Interaction focus and prompt presentation | Hyper Interaction after compatibility test. |
| Actual interact/use decision | Game-owned interaction bridge validating world/UI state. |
| Item instances, quantities, grid positions | AGIS only. |
| Translation between mission requirements and AGIS | `BP_CoastalAGISAdapter`, to be implemented against the installed package. |
| Mission facts | Included `UFirstSignalComponent`, on one persistent mission-director actor. |
| Pickup/door/container world state | Project world-state registry using stable IDs. |
| Journal/UI display | Read-only view of game state, plus original journal text. |
| Durable campaign save | Project save coordinator; not implemented by the plugin. |

Vendor Blueprints should not be edited in place unless a documented limitation forces it. Prefer project-owned subclasses, interfaces, and adapter components. Keep the migration path and version record so a vendor update does not erase the game's behavior.

The supplied plugin deliberately has no direct AGIS or Hyper dependency because their local symbols are unknown. It is not a substitute for those packages, and it does not ship an alternative inventory.

## 11. Saving: one campaign, one coherent snapshot

Epic's SaveGame facilities provide serialization entry points; the game must still define and restore its state. The design here uses a custom campaign snapshot, rather than pretending that separate vendor saves automatically remain consistent. [S7]

The unified snapshot must include a schema version, campaign ID, monotonically increasing save generation, map ID, player/checkpoint transform, AGIS item/container state, transaction ledger, world-object state, journal discoveries, and the First Signal snapshot.

A save coordinator holds interaction/mutation work during a load. For saving, capture a consistent in-memory snapshot only after the repair transaction and mission update have both completed. Queue writes; do not allow overlapping writes to reorder generations. Persist inventory and its committed-transaction ledger in the same generation as the objective flags.

The preferred resilience design is two alternating save slots. Write the inactive slot, read back and validate it, and retain the previous valid generation. On load choose the highest complete valid generation; fall back explicitly when the latest is damaged. This policy is implementation work and must be tested under write failure. Do not claim crash-safe disk atomicity merely because an API returned success.

On load: validate schema and required adapter availability; restore inventory/ledger; restore world and objective facts; place the player at a validated transform; refresh UI; then re-enable input. Do not restore only the quest and call that a campaign load.

The included `ExportSnapshot`/`RestoreSnapshot` functions operate on objective data in memory only. `SaveGame` property annotations do not write a file by themselves.

## 12. Visual and sound direction

Aim for weathered wood, salt-worn metal, reeds, muted water, and a cabin interior warmer than the outside. Treat this as one location with one lighting setup, not several showcase levels pasted together.

Use a fixed, readable time of day in v0.1. Water is visual and bounded. Do not require Fluid Flux, Ultra Dynamic Sky, storm simulation, tide-dependent routes, or volumetric fog to complete the opening.

Atmosphere comes from intelligible sound layers: shoreline water, restrained wind, interior room tone, footsteps, a few interactions, and the radio. Whether suitable audio exists in the owned collection remains unverified. Missing audio can be silent in a clearly labeled systems test, but not silently counted as finished polish.

Use actual purchased environment art when making the first public-looking playable. A mannequin and gray boxes are acceptable for M1 testing and should be listed as temporary. Do not advertise a blockout as the final visual game.

## 13. Performance and scope controls

The baseline target is the user's previously identified RTX 3060 12 GB desktop class, at 1920 × 1080. Target approximately 60 frames per second in the packaged opening; this is not a measured result or guarantee.

Keep a repeatable route through cabin, shore, dock, and overlook. Record CPU/GPU frame time, memory, asset streaming behavior, and conspicuous hitches before/after visual changes. Separate first shader warm-up from repeated traversal measurements. Profile a standalone packaged build, not just the editor viewport.

Start without hardware ray tracing. Evaluate more expensive lighting/reflection choices only after the baseline route is stable. Reduce excessive texture resolutions, material cost, foliage density, and shadow cost based on evidence rather than disabling every visual feature blindly.

Do not import the complete harbour, complete platform, multiple weather stacks, a combat framework, a procedural world generator, or all inventory demos into the production map. No feature enters v0.1 merely because an owned pack contains it.

## 14. Milestones and completion gates

### M0 — source foundation delivered in this package

Mission design, data specifications, state-rule source, Blueprint-facing component and adapter contract, standalone tests, and implementation guidance exist. No engine build or vendor integration is represented as done.

### M1 — compatibility and functional test room

Create the clean Third Person project. Verify the candidate engine against the cabin, AGIS, and Hyper Interaction separately. Compile the source plugin. Build a room with a door, two pickups, a storage chest, a radio proxy, and a mission director. Wire actual AGIS transactions and the save coordinator.

**Gate:** pickup/transfer/repair/load works without duplication, and missing providers fail visibly. Package and launch the test room outside the editor before importing the coastline.

### M2 — opening map and objective

Assemble Cabin Cove, Shoreline Trail, Old Dock, and the optional overlook. Replace development proxies with verified owned assets. Place guaranteed radio parts, optional discovery, and journal text. Integrate the mission component with real actors.

**Gate:** a new player can finish the objective without developer commands, including collecting parts out of order and resuming a partial save.

### M3 — input, presentation, and stability

Finish gamepad UI, camera collision, water recovery, footsteps, ambient sound, text readability, settings, and performance passes. Remove any buttons for missing systems. Maintain a list of remaining temporary art.

**Gate:** the complete acceptance checklist passes in the Windows package. Record what hardware and engine build were tested.

### M4 — v0.1 playable delivery

Deliver the Windows executable/build folder, a short controls/readme document, source revision, dependency/version register, test evidence, known issues, and a save migration/recovery note. The binary build must work without opening the editor.

**Gate:** clean launch, start, continue, full objective, save/reload, exit/relaunch, and no missing critical dependencies. Completion is based on this behavior, not a percentage inferred from file count.

## 15. Expansion after v0.1

v0.2 can add one fishing spot with a rod, a bite cue, a simple response, and a catch that enters inventory. Catch delivery must still be transactional: a full backpack cannot silently destroy the reward. Add one or two species first, with clear catches recorded in the journal.

v0.3 can add a small harbour district and one access-restoration objective. v0.4 can make the offshore platform explorable, using either a scripted journey or a separately developed boat system. A boat mesh does not supply boat gameplay.

Cabin improvements, additional discoveries, selected camping interactions, weather, and a companion remain optional future design decisions. They are not promised as part of the opening. Co-op stays out unless the user explicitly reverses that choice.

## 16. Immediate local execution order

Create the standard Third Person C++ project, record the engine/toolchain, compile the included plugin, then test the purchased inventory and interaction packs in isolation. Connect one real pickup and one real storage transfer before spending time on environmental dressing. Complete the combined repair-and-save loop in the test room, then build the coastal route around it.

The project is ready to grow when a player can leave the cabin, retrieve the parts, use them, and return after a relaunch with a coherent world. That is the foundation; the rest is expansion.

## Sources

Checked September 5, 2026. These sources support the specific product/engine observations above, not the original game design or completed integration claims.

- [S1 — Epic Third Person template](https://dev.epicgames.com/documentation/en-us/unreal-engine/third-person-template-in-unreal-engine)
- [S2 — Hyper Scalable Interaction System V4](https://cosmos.leartesstudios.com/tools/gbh-scalable-interaction-system)
- [S3 — Advanced Grid Inventory System](https://www.fab.com/listings/16b82fb0-7ea5-4627-adcc-95f23a387b61)
- [S4 — Nordic Fishing Hut](https://www.fab.com/listings/e56e7e02-01b8-4a15-a108-aa5d76ea0e10)
- [S5 — Ultimate Fishing Megapack](https://cosmos.leartesstudios.com/environments/sb-ultimate-fishing-megapack)
- [S6 — Epic Enhanced Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine)
- [S7 — Epic saving/loading](https://dev.epicgames.com/documentation/en-us/unreal-engine/saving-and-loading-your-game-in-unreal-engine)

Ownership references for Cosmos selections: supplied inventory lines 24, 27, 32, 35, 41, 50, 51, and 65. The source inventory is included alongside this document.
