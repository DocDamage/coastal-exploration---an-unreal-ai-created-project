# Original M0 README (historical text)

```text
# Coastal Exploration — First Signal

**Third-person. Single-player. Windows first. Unreal Engine.**

This is an original **source-and-design starter**, not a playable Unreal project or a packaged game. It does not contain the user's purchased assets, a `.uproject`, maps, Blueprint assets, a character, an inventory implementation, or disk-save implementation.

The Unreal plugin contains the opening mission's rules and Blueprint-callable integration points. Its inventory adapter intentionally returns `NotConfigured` until the real purchased inventory system is connected. The mission cannot pretend that a radio repair succeeded.

## Start here

1. Read [the foundation plan](docs/COASTAL_EXPLORATION_FOUNDATION.md) for the game and scope.
2. Follow [Unreal setup](docs/UNREAL_SETUP.md) to create the actual Third Person project and install the source plugin.
3. Follow [integration contracts](docs/INTEGRATION_CONTRACTS.md) to connect Hyper Interaction, AGIS, and unified saving.
4. Use [acceptance tests](docs/ACCEPTANCE_TESTS.md) as the completion gate, not just the existence of files.
5. Read [the validation report](VALIDATION_REPORT.md) before interpreting the test results.

## Included

| Path | Purpose |
|---|---|
| `Plugins/CoastalFoundation/` | Original Unreal runtime-plugin source; objective component and fail-closed inventory adapter contract. |
| `data/` | Item, mission, input, asset-register, and metric blockout design JSON. **These are not auto-imported Unreal assets.** |
| `tests/first_signal_core_tests.cpp` | Standalone tests of the exact engine-independent rules used by the plugin. |
| `tools/` | C++ test runners and a structural package validator. |
| `docs/` | Implementation plan, local setup, contracts, asset register, and acceptance criteria. |
| `AGENTS.md` | Execution rules for a coding assistant working on the eventual project. |
| `inventory_user_supplied.md` | The inventory text supplied in this conversation; not a re-audit of account entitlements. |

## Current validation boundary

The standalone rules were compiled and executed in a Linux C++ environment. The Unreal wrapper was **not compiled by Unreal Header Tool or UnrealBuildTool**. Editor automation tests are included but were not run. The Windows script, downloaded asset versions, vendor integrations, save persistence, gamepad UI, and performance are untested.

## Local standalone checks

Linux/macOS with an appropriate C++17 compiler:

```sh
bash tools/run_core_tests.sh
python3 tools/verify_package.py
```

Windows: open Developer PowerShell for Visual Studio with the C++ toolchain available:

```powershell
.\tools\run_core_tests.ps1
py .\tools\verify_package.py
```

The standalone checks do not require Unreal and do not prove Unreal compatibility.

## Engine decision

Unreal is selected. UE 5.7 is an initial **compatibility-test candidate**, not a certified or mandatory version: the checked Hyper Interaction listing names 5.7, but the actual installed AGIS and cabin packages must also be tested. Record the eventual engine patch and toolchain in the asset register. Do not downgrade or overwrite the only copy of any vendor project.

No repository was created or changed. No paid asset files or engine template assets are redistributed in this package.
```
