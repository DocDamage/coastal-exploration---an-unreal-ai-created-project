# Authoritative combat shot integration

The standalone wrapper renders a tracer only when the actual vendor shot returns
one matching receipt and consumes exactly one round from the same weapon in the
same campaign epoch. The receipt contains the final muzzle start, blocking impact
or full-range endpoint, and the vendor's real-time admission timestamp. The
wrapper no longer performs a separate unspread trace. Reentrant fire attempts
are rejected while the synchronous vendor call is in progress.

Fire cooldown compares `GetRealTimeSeconds` with the receipt timestamp, matching
the vendor's clock under time dilation. Reload still uses game time, matching the
vendor timer and remaining paused with the native menu.

The private `AdvancedShooterSystem` working copy and isolated host require this
project integration extension (the original purchased payload is preserved):

- Public native `OnAuthoritativeShot` multicast with signature
  `(AWeaponBase*, const FVector& Start, const FVector& End, double RealTime)`.
- Emit exactly once after accepted damage, historical-transform restoration and
  multicast fire effects. Capture the shot weapon and endpoint before damage
  callbacks. Do not emit for a rejected request.
- Validate the pawn/controller before consuming ammunition or rewinding actors,
  and skip invalid actors when restoring historical transforms.

This repository does not redistribute vendor source. Private pre-edit backups
are under `../local-evidence/combat-vendor-before-resume`; the installed working
patch is in `../LocalVendor/AdvancedShooterSystem/Working`. An unpatched vendor
does not satisfy this compile-time contract.

`CoastalHostEditor::ProbeCombatShot` is restricted to the independent standalone
PIE host with a `coastal_test_` campaign. It observes the vendor receipt, invokes
the real wrapper, then reconstructs each new tracer's endpoints from its actual
mesh transform. `check_coastal_combat.py` requires one receipt, one tracer, one
round consumed, and endpoint error at most 0.1 cm. It also exercises repeated
fire at 0.1× game time, fire at 4×, and rejection without ammunition/effect changes.
The original input, reload/pause, occlusion, sentry and recovery cases are retained.

The sentry checks Visibility world cover while ignoring the player and attached
weapon, then requires a real Pawn-object hit on the player before applying point
damage. This supports the standard capsule without changing its global collision
responses. A disposable cover test moves the existing wall temporarily and restores
its transform and mobility; the rendering gate is isolated during that test only.

The latest executed run passes all 16 live cases, with 50 native tests passing.
These files describe implementation and test intent. Consult the newest
`M3_IMPLEMENTATION.md` checkpoint for executed results and remaining acceptance.
