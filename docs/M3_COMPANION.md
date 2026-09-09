# German Shepherd companion

The UE5.8 trial uses one map-owned `ACoastalCompanionDirector`, which creates a
transient `ACoastalCompanionCharacter` for the existing configured local player.
The existing pause menu offers **Dog: wait here** or **Dog: follow me** through
a Foundation-owned optional interface. The expansion installs no input mappings
and adds no inventory owner or campaign-save fields.

The dog uses the imported German Shepherd mesh and supplied idle, walk, and run
clips. CharacterMovement receives input every frame, with capsule sweeps and floor
probes steering around nearby obstacles. Pawn collision is ignored so the dog
cannot obstruct the player. This is local steering; complex maze navigation is
not claimed. Long-distance catchup chooses a tested dry floor near the player.
Both candidate placement and current-water displacement reject water volumes.

Campaign changes reset transient wait state. Player recovery suspends the dog,
then permits dry regroup through the existing recovery authority. Commands check
the current player, save epoch, campaign readiness, and recovery state. Pause
readiness permits showing the choice; the commit requires the menu to close.

Private content stays in the local host. Authoring recipe:
`tools/unreal/configure_coastal_companion.py`. Live harness:
`tools/unreal/check_coastal_companion.py` with an active `coastal_test_` save set.

Validation evidence lives outside the source repository under `../local-evidence`:
- `m3-outer-coast-build.execution.json`: native build passed.
- `m3-outer-coast-native-tests.json`: all 52 native tests passed.
- `m3-coastal-companion-authoring.json`: authored director and real asset bindings.
- `m3-coastal-companion-live.json`: all seven live cases pass, including the rendered
  pause-menu commands and campaign epoch reset. Other campaign hashes are preserved.

Physical controller testing, visual polish, complex-route acceptance, and packaged
performance remain separate gates.
