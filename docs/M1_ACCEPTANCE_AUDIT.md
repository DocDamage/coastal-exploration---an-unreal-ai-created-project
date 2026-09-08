# M1 acceptance evidence audit

Audit: 2026-09-07 UTC. M1 completion remains unproven. This audit reads the current source requirements and maps existing runtime evidence; it does not claim new gameplay tests were executed.

The [foundation milestone definitions](COASTAL_EXPLORATION_FOUNDATION.md) and [M1 working-room requirements](WHAT_REMAINS.md) are the scope authorities, together with the user's GitHub start prompt. M1 requires actual vendor compatibility, native build, the systems room, AGIS/Hyper/input wiring, native presentation consumers, real save/failure behavior and the launched coherent campaign loop. The full coastal route and final environment assembly belong to M2. Target-hardware performance and final presentation belong to M3; their absence is not by itself an M1 blocker. No later milestone has been started or declared complete.

## Current case mapping

The [297-row crosswalk](native-evidence/m1-acceptance-crosswalk.json) retains every procedure and expected result from the nine current JSON acceptance ledgers. It records source/evidence hashes and distinguishes:

- 20 bounded rows supported on the recorded setup.
- 43 rows with partial evidence and specific remaining conditions.
- 233 rows without case-complete evidence mapped in this audit. This does **not** mean 233 features are missing or that none of their behavior has ever been tested.
- 1 older write-schema expectation superseded by a later increment. M1.7's schema-2 write expectation must not force a downgrade of the current schema-3 writer. Its generation, inactive-slot and preserved-old-file invariants remain required in the M1.8 upgrade cases, which now have bounded packaged upgrade evidence.

These counts are not a completion percentage. A single row may contain several fault configurations or require physical-device observations. The 41 native automation passes do not automatically close actual-host rows. The original delivered acceptance files remain unchanged historical records.

Markdown M1-09 now has [packaged evidence](native-evidence/m1-campaign-ambiguous-progress.json) for blocking a retry after a failed write report leaves a newer valid generation, followed by validated relaunch and successful saving. This is separate from the 297 JSON-row counts.

The separate [58-case Markdown mapping](M1_REFERENCE_CASE_AUDIT.md) is now reconciled: 52 cases contain M1 requirements (26 supported M1 components on the recorded setup, 16 partial, 10 requiring evidence); six are later-only. Shared milestone rows preserve their M1 invariants. These counts overlap the JSON ledgers and must not be added together.

## Next work, in order

1. Verify required compatibility and failure evidence at the core M1 boundary: cabin-package compatibility record, visible missing-provider refusal, remaining stale target/input conditions; packaged range/occlusion and stale storage context now have bounded evidence. Preserve AGIS/Hyper as the actual authorities.
2. Finish remaining campaign-save ambiguity and storage errors. Terminal recovery/rollback rejection and preference no-write, truncated write, unreadable read-back and valid-write/false-return now have packaged evidence.
3. Finish remaining preference lifecycle/compatibility conditions. Legacy schema1/2 upgrades and the four write-fault forms now pass with exact file restoration and fresh-process resolution.
4. Complete the needed physical input/presentation observations and remaining audio/display ownership cases. Injected input and mixer captures remain useful but are not physical-device evidence.
5. Reconcile the original handoff deliverables and every remaining applicable M1 condition against current artifacts, using the completed Markdown scope mapping. Revalidate final build/source/package provenance before closing M1. Publishing remains unauthorized.

The latest [integration status](M1_NATIVE_INTEGRATION_STATUS.md) remains the concise runtime progress report. This crosswalk is a conservative retrieval and execution queue, not permission to redefine completion around the tests already passing.
