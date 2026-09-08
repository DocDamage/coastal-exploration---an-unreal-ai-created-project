> Historical M1.3 source-only record, retained from the supplied package. See the current root README and validation for M1.4.

# Coastal Exploration — M1.3 Checked Startup & Integration Diagnostics

**Third-person • Single-player • Unreal • Windows first • First Signal**

This is the **complete updated source package**, based on the supplied M1.2 source. Plugin version: `0.1.3-m1-startup-source`. It retains the mission, coherent campaign persistence and native development menus, and adds a checked startup entry point, exact test-room authoring checks, real-provider read diagnostics, shared dry-checkpoint validation and a read-only editor audit.

**No Unreal-compiled game or Windows executable is included.** The host `.uproject` and paid AGIS/Hyper assets are not mounted here. Native startup, reflected types, purchased integrations and packaging remain untested in Unreal. M1 is not passed, and the coastal opening has not been assembled.

## What changed

Add one `CoastalSessionBootstrapComponent` to the same local PlayerController that owns `CoastalUISessionComponent`. Once the actual character is possessed and the real AGIS provisional session is ready, call:

```text
Bootstrap.StartTestRoom(ActualDirector, ActualAGISAdapter, ValidatedDryCheckpoint)
```

The component checks ownership, native input availability, the seven authored persistent objects, both required item mappings, provider export/view readiness and dry placement before calling the existing save configuration, interaction-bridge configuration and UI initialization in order. A read-only preflight failure can be explicitly retried after fixing it. A failure after binding begins requires exit/relaunch instead of pretending partial initialization is safe to reuse.

`Started` means the development session menu attached. It does **not** mean a campaign started, inventory transactions passed or anything was saved. Start New and Continue remain explicit player choices in the existing menu.

The editor audit inspects only the currently loaded world's persistent-object manifest. It does not generate/save maps, invoke vendor APIs or certify the packaged game. It can write an exclusive-new-file JSON report with all runtime gates explicitly `not_run`.

## Read these next

| File | Use |
|---|---|
| [M1.3 implementation](../../docs/M1_3_IMPLEMENTATION.md) | New source, lifecycle, limitations and retained guarantees. |
| [M1.3 local wiring](../../docs/M1_3_STARTUP_WIRING.md) | Installation, exact original plugin calls, startup troubleshooting and editor audit. |
| [Current validation](M1_3_VALIDATION_REPORT.md) | Executed checks and unrun gates. |
| [Startup acceptance ledger](../../data/m1_3_startup_acceptance.json) | 26 local scenarios; initially all not run. |
| [Integration worksheet](../../data/m1_3_integration_worksheet.json) | Record actual host/vendor details without invented asset paths. |

## Install

Back up/commit the host project and your custom adapter first. Replace only `Plugins/CoastalFoundation` with this package's complete plugin folder and rebuild the actual Development Editor / Win64 host. Keep purchased assets separate. Use the new bootstrap **instead of**, not alongside, the old three manual Configure/Initialize calls. Existing M1.2 initialization remains available for already-wired hosts.

AGIS is still the only inventory authority. The default adapter still returns `NotConfigured`. Hyper still owns target focus, prompts and the real pressed-once interaction event; this update contains no invented vendor bindings or substitute inventory.

## Run standalone checks

```sh
bash tools/run_core_tests.sh
CXX=clang++ bash tools/run_core_tests.sh
CXX=clang++ bash tools/run_sanitizers.sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests -p 'test_*.py' -v
python3 tools/verify_package.py
```

The Windows core-test runner is retained, but was not executed here. The editor audit requires the actual rebuilt Unreal plugin. Earlier M1/M1.2 evidence is retained under `evidence/archive/`. No repository was modified, no `.umap` was fabricated and no purchased content is included.
