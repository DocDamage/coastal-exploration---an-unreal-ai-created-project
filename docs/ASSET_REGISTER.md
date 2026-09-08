# Asset register and unresolved content

The machine-readable register is `data/asset_register.json`. Ownership is based on the user's supplied inventory and Fab links. Downloaded versions, license records, content paths, and compatibility results are deliberately blank until inspected locally.

## Import order

**First, systems:** test Hyper Scalable Interaction System V4 and Advanced Grid Inventory System independently, then test the project adapter and source plugin together in a small room.

**Second, opening art:** Nordic Fishing Hut, selected Ultimate Fishing Megapack assets, and selected Coastal Wetland & Railroad Bridge assets. Do not migrate every demonstration map as a single combined game.

**Third, finishing candidates:** Hyper Outliner, Hyper Footstep, and Hyper Mesh to Icon Creator. Add one at a time and retain the baseline behavior/performance checks.

**Later:** camping animations, Industrial Harbour, and Abandoned Sea Platform. Their ownership is not a mandate to use them in v0.1.

## Per-pack local record

Record the acquired package filename/version, installed engine version, plugin dependencies, original content root, relevant game-owned subclasses, exact assets used, demo-open result, migration result, package result, and licensing/acquisition record. Never put passwords, auth tokens, cookies, or full account exports in this register.

## Missing or unresolved content

| Need | Current evidence | Action |
|---|---|---|
| Final human player character | Not identified in the provided selected inventory. | Use the clearly labeled template mannequin for systems testing; select a real owned character before final art approval. |
| Radio | Advertised in the Nordic cabin listing. | Locate and inspect its actual mesh/Blueprint; add game logic in a project-owned actor. |
| Separate battery mesh | Not established. | Inspect owned props; do not invent an asset path or assert it exists. |
| Separate fuse mesh | Not established by the presence of a fuse box. | Inspect; use an explicit test proxy until resolved. |
| Storage chest/containers | Candidate cabin props, exact local assets unverified. | Select suitable models and add project interaction/state. |
| Ambient coast/radio/footstep audio | Suitable sounds not yet verified. | Audit owned audio before adding a new dependency. |
| Final icons | Not generated or delivered. | Generate/capture from selected items with the actual installed workflow. |
| Open-world water/boat system | Not part of the first build. | Keep bounded visual water; no purchase required for v0.1. |

Public product descriptions and their limitations are summarized in the foundation plan. The register labels whether a product was checked in this turn or is only supported by inventory/earlier discussion. No asset package was opened or imported here.
