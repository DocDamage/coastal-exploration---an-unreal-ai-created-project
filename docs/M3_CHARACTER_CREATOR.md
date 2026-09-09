# M3 Mutable character creator

## Implemented integration

The independent UE5.8.2 host at `G:/coastline/LocalHost58/CoastalExploration`
uses the installed Mutable Sample art for a player-facing creator. Open
**Pause → Customize character** after starting or loading a campaign. Select
a setting, change its option, rotate the portrait, then **Apply and save
appearance**. Back discards unapplied changes. The existing campaign menu,
painted-command guard, keyboard/gamepad navigation and input blockers own the UI.

Sixteen controls cover body shape, skin tone, age, hair, eye color, eye/nose/lip/
chin shape, shirts, shirt color, jackets, trousers, two trouser palettes, and
shoes. Conditional clothing colors affect their corresponding garment. The
two body-shape parameters are linked. The initial palette excludes the sample's
cyborg and costume options.

The native character, CharacterMovement, controller, AGIS, interaction bridge,
camera, recovery, swimming, camping and combat owners remain authoritative.
Mutable generates Body and Head as collision-free child mesh components. The
original player mesh keeps evaluating its native animation and is hidden only
when a compatible generated presentation is ready. A native retarget animation
instance evaluates that source pose on the generated skeleton. Unsupported
source skeletons fall back to the original mesh. Groom usages are attached
before generation so HairStrandsMutable creates hair.

The square creator portrait places controls alongside its image, frames facial
settings close up, and uses two temporary lights on a dedicated lighting
channel. It excludes world fog, sky lighting and global illumination. Closing
the creator releases captures and lights and restores normal lighting channels.
The existing combat weapon follows the ready generated hand, including during
reactions and regeneration; the combat owner still owns weapon/ammo/damage.

## Private assets and reproducibility

`tools/stage_m3_mutable.py` copied and hash-verified the expanded dependency
closure: 43 character graphs and 774 files, 2,297,491,546 bytes. It includes
reverse-linked clothing/hair graphs. `stage_m3_mutable_default.py` separately
copied the source default instance needed for asset duplication. Source paths
stay intact under `/Game/Character`; purchased payloads remain outside Git.

The host enables Mutable, HairStrands, HairStrandsMutable, MutableClothing and
IKRig. Expansion58 declares its Mutable/IKRig dependencies.
`enable_m3_mutable.py` retains a descriptor backup. The UE5.7 rollback host
remains separate.

Project-owned assets are under `/Game/Coastal/Character`:

| Asset | Purpose |
| --- | --- |
| `COI_CoastalDefault` | Human coastal-clothing preset cloned from the sample |
| `DA_CoastalCharacter` | Versioned controls, source-skeleton map and action clips |
| `Rigs/IK_CoastalPlayer` | Existing Quinn/Manny source rig |
| `Rigs/IK_CoastalMutable` | Generated `SKEL_BaseBody` target rig |
| `Rigs/RTG_PlayerToMutable` | 28 mapped chains, automatic FBIK and aligned target pose |
| `Animations/CA_AS_Collects` | Retargeted pickup presentation |
| `Animations/CA_AS_Get_Hit_Front` | Retargeted player hit reaction |

Authoring tools under `tools/unreal` inspect/compile the graph, build the rigs,
clone the default, build the definition and retarget the two actions. Graph
compilation and default Body/Head generation passed. Editor cold starts may
need asynchronous Mutable compilation; the pause menu reports preparation
while the existing player remains available.

`configure_m3_character_cook.py` adds `/Game/Coastal/Character` and
`/Game/Character` to the private host's packaging settings and retains its
previous config. This includes the dynamically loaded definition and
reverse-linked child graphs. Cook configuration is not a completed Windows
package or packaged runtime acceptance.

## Persistence contract

Campaign and preferences schemas are unchanged. Cosmetic state lives in two
sidecars named `CoastalAppearance_<campaign GUID>_0/1`, with schema 1, appearance
version `coastal.human.v1`, generation and ordered selection indices. Control
ordering/options are part of that version's contract; incompatible changes
require an explicit version/migration decision.

Apply validates every selection against the compiled graph, writes the next
alternate slot, reads back and compares bytes, then verifies the deserialized
identity and selections before publishing it as applied. The previous slot is
retained. Preview changes never auto-save. Loading chooses the newest valid
generation. A damaged or unsupported slot is preserved and disables writing;
a valid older slot may still provide the appearance. Conflicting equal
generations or two invalid slots fall back to the native player with a status
message. New campaigns without sidecars use the preset.

Asynchronous results must match the current instance, campaign GUID, session
epoch and requested selections before replacing meshes. Closing the creator,
campaign switching and recovery invalidate or discard stale work. Regeneration
keeps the previous complete presentation until both new mesh parts are ready.

## Animation allocation

| Supported behavior | Playback owner / selected source |
| --- | --- |
| Idle, movement, jump | Existing `ABP_Unarmed`, runtime retargeted to Mutable |
| Swimming | Existing SwimmingAnimationPack idle/forward owner, runtime retargeted |
| Warm hands, rest by fire | Existing CampingAnimations owner, runtime retargeted |
| Rest in shelter | Existing MorbidMotions sleeping owner, runtime retargeted |
| Successful pickup | Camping `AS_Collects`, retargeted owned clip; 0.5–3.3 seconds |
| Accepted nonfatal player damage | Motorcycle extras `AS_Get_Hit_Front`, retargeted owned clip; 0.8 seconds |

The two new actions run on the `CoastalAction` slot of both generated parts.
They have root motion disabled and source notifies removed to avoid duplicate
gameplay callbacks. They are cosmetic responses to successful authoritative
events. Movement, jumping, menus, generation, recovery, swimming and camping
prevent or interrupt them; they never consume input or change inventory/damage.
No redundant base locomotion controller was imported. Vehicle, ladder, crawl,
acrobatics, magic and motorcycle riding remain unbound because those gameplay
actions are not implemented. Shooting/reloading remain with the existing
combat system; no new firing or reload clip is claimed.

## Validation and evidence

The native integration builds successfully. All 60 native tests pass (59 clean,
one existing AGIS warning), including independent appearance serialization.
The real PIE smoke verifies generated art, native owner preservation, exact
shape changes, verified save, discard, reload and pre-existing save preservation.
The variant test changes all 16 controls, verifies generated parameters and
grooms, saves, reloads and compares every parameter. The action test requires
substantial generated-bone movement for pickup/hit reactions and verifies
movement, jump, camp/shelter poses, interruption/idempotence, and weapon
attachment across regeneration and discard.

The final swimming route passes with 824 generated-character swimming
samples. Campaign-isolation and invalid-sidecar checks pass for future
appearance versions, wrong save classes, read-only fallback and two invalid
slots. Invalid files remain byte-identical; only disposable test files are
restored after fault injection. All 111 pre-existing saves, both maps and the
rollback descriptor remain unchanged. The active host descriptor change is
the planned plugin enablement. The combined final suite passes all six scripts:
18 creator assertions, 39 action assertions, 16 combat assertions, 56 variant
assertions, the swimming route and invalid-sidecar guards. The 70 Python tests
and structural source checks also pass.

Private reports are under `../local-evidence/m3-character-*`, including build,
native tests, smoke, variants, action playback, graph/default generation,
retarget rig/action manifests, images and preservation. The final suite uses
`coastal_test_m3_character_0909g` and the separate disposable guard campaign.
`m3-character-final-suite.json` is the combined result. Earlier portrait and
weak motion checks are superseded: final images show the dedicated-light
portrait and actual in-game pickup motion, and the root-node correction is
covered by the stronger action checks. The editor is outside PIE with 2,935
actors, no dirty maps/content, and restored background throttling. Source
changes remain uncommitted; Windows package testing remains open.

Scripted PIE, source loadability, visual inspection, physical device testing,
performance and Windows-package acceptance are distinct. A broad art review
of every combinatorial appearance and physical-device/package testing are not
implied by these checks. Earlier staging and pre-creator checkpoints elsewhere
are historical; this document records the implemented boundary.
