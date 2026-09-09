# M3 private staging import plan

## Authorized source and target

The user authorized migration of selected content from the UE5.7 staging
project at `G:/coastline/M3DestinationStaging57` into the active independent
UE5.8.2 host at `G:/coastline/LocalHost58/CoastalExploration`. The source has a
UE5.7 descriptor and is a private staging project. The target project and its
UE5.8.2 descriptor remain authoritative.

The newly present source content includes these animation folders:

- `Free_Crawl_Animation`
- `FreeLadderAnimationSet`
- `FreeAnimationSet`
- `MultiSportAnimations`
- `CampingAnimations`
- `MotoInteractionAnims`
- `RamsterZ_FreeAnims_Volume1`

The source also contains existing environment folders for
`Atlantis_Ruins`, `ModularSciFiStation`, `CarpentersWorkshop`, `RetropunkSaloon`,
and `IndustrialSlums`, plus external actor/object roots. Folder names establish
presence only; they do not establish Fab-product identity, asset compatibility,
or gameplay readiness.

`MotoInteractionAnims` is selected for private import now because the user
plans to add motorcycle gameplay later. Preserve any mount, riding, and
dismount coverage found during actual inspection, but do not infer that those
clips exist from the folder or product name. Keep all motorcycle animation
binding reserved until a motorcycle actor and its gameplay authority are
present. `RamsterZ_FreeAnims_Volume1` is also present in the source; its exact
product mapping and useful clip set remain to be inspected.

Read-only source inspection has confirmed filename families in
`MotoInteractionAnims` for left/right mount and dismount, riding idle/start and
mounted/ride transitions, `_Bike` variants, rider/bike/passenger turn
start/loop/end variants, mirror and front/back tyre checks, helmet handling,
and pistol/punch/get-hit extras. Preserve the complete folder for the future
motorcycle increment. This is filename-level coverage only; no animation has
been imported or played, and no playback acceptance is claimed.

## Migration boundary

The read-only preflight compares all source files against the active UE5.8
Content tree before any copy. The planned migration is a private, additive
content copy with per-file size and SHA256 evidence. It must not replace the
target `.uproject`, Config, controller/input assets, saved maps, existing
environment roots, external actor/object roots, or any host-owned authoritative
gameplay source. Existing saves and the UE5.7 rollback host remain preserved.

Do not copy a source project descriptor or controller setup into the active
project. Resolve name collisions explicitly and refuse changed target files
unless the import procedure has an evidence-backed mapping. Keep purchased
payloads and generated evidence outside GitHub. The completed preflight found
2,408 missing source files, 746 identical files, 10,971,245,578 missing bytes,
and no conflicts. The completed additive private copy added 2,408 files and
verified every new-file SHA256, with 0 conflicts. The source inventory remained
unchanged during the copy.

## Evidence and acceptance boundary

The preflight and planned file mapping are recorded under
`../local-evidence/m3-staging-new-content-plan.json`; completion is recorded in
`../local-evidence/m3-staging-additions-copy.json` (`complete: true`). The
completed migration added 2,408 files totaling 10,971,245,578 bytes and kept
746 existing files identical with no conflicts. The follow-up registry audit
loaded 2,406 registered assets; one 732-byte Retropunk Saloon material package
is unregistered but matches the source. The registry report records one missing
package, 15 missing game references, and `passed: false`; three hard references
are the motorcycle demo's old `CarInteractAnimVol1` ControlRig paths and the
remaining 12 are soft references. Registry closure remains an open follow-up.

The completed migration evidence records source and target roots, selected
folders, file counts, byte totals, per-file hashes, skipped/conflicting paths,
and final target identity.

The final source sync matched 198 native source files across three plugins to
the active host by SHA256 (`../local-evidence/m3-staging-preview-source-sync.json`).
The editor was left outside PIE with 2,935 actors, no dirty maps or content
packages, and restored background throttling (`../local-evidence/m3-staging-preview-editor-final.json`).

Imported content is not gameplay integration. Animation acceptance still
requires skeleton/retarget verification, root-motion and contact review, and
binding to an existing owner. Environment content still requires map references,
collision, route traversal, save preservation, and packaged acceptance. No
native, PIE, physical-device, performance, or Windows-package result is implied
by the source preflight or a successful private copy.

The native test wrapper has since completed with exit code 0, and rollback
critical files remain unchanged. This does not change the migration or
animation playback boundary.
