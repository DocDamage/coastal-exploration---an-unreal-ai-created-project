"""Read ONLY the currently loaded editor world. Run after building CoastalFoundation.
Never loads/saves a map, creates an actor, invokes AGIS, or modifies project settings.
"""
from pathlib import Path
import sys

TOOLS = Path(__file__).resolve().parents[1]

def audit(report_path: str | None = None):
    import unreal
    # Load the original helper without persistently changing the editor's import path.
    import importlib.util
    spec = importlib.util.spec_from_file_location("coastal_integration_report", TOOLS / "integration_report.py")
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if level.is_in_play_in_editor():
        raise RuntimeError("Stop PIE first. This command audits the editor world, not a playing campaign.")
    if not hasattr(unreal, "CoastalIntegrationLibrary"):
        raise RuntimeError("Rebuild/enable the M1.3 CoastalFoundation plugin before running the audit.")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world:
        raise RuntimeError("There is no currently loaded editor world to inspect.")
    observed = unreal.CoastalIntegrationLibrary.inspect_test_room(world)
    issues = [{"code": str(i.code), "subject": str(i.subject) or "<empty world ID>", "detail": str(i.detail)}
              for i in observed.issues]
    if bool(observed.passed) != (not issues):
        raise RuntimeError("Native report pass/issue fields disagree; no report written.")
    dirty = list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
    dirty += list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    report = helper.make_report(world.get_path_name(), unreal.SystemLibrary.get_engine_version(),
                                issues, [p.get_path_name() for p in dirty])
    unreal.log("Coastal M1.3 loaded-world manifest: " + ("PASS" if report["room_manifest_passed"] else "BLOCKED"))
    for issue in issues:
        unreal.log_warning(f'{issue["code"]} | {issue["subject"]}: {issue["detail"]}')
    unreal.log("This audit does not verify disk map contents, AGIS/Hyper, startup, gameplay, or packaging.")
    if report_path:
        helper.write_new_report(Path(report_path), report)
        unreal.log(f"Wrote a new manifest-only report: {report_path}")
    return report

if __name__ == "__main__":
    audit()
