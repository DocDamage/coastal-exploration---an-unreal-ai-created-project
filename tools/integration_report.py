#!/usr/bin/env python3
"""Exclusive-new-file report output for the read-only editor audit; no Unreal import."""
from __future__ import annotations
from datetime import datetime, timezone
import json
from pathlib import Path
from typing import Any

NOT_RUN = ("runtime_startup", "vendor_transactions", "controller_gameplay", "windows_package")

def make_report(world_path: str, engine_version: str, issues: list[dict[str, str]],
                dirty_packages: list[str]) -> dict[str, Any]:
    if not isinstance(world_path, str) or not world_path.strip():
        raise ValueError("The actual editor world path is required.")
    if not isinstance(engine_version, str) or not engine_version.strip():
        raise ValueError("The actual editor engine version is required.")
    if not isinstance(issues, list) or len(issues) > 512:
        raise ValueError("Invalid or oversized issue list.")
    copied = []
    for issue in issues:
        if not isinstance(issue, dict) or set(issue) != {"code", "subject", "detail"}:
            raise ValueError("Issue must contain only code, subject, detail.")
        if any(not isinstance(issue[k], str) or not issue[k].strip() or len(issue[k]) > 8192
               for k in issue):
            raise ValueError("Issue fields must be nonempty bounded text.")
        copied.append(dict(issue))
    if (not isinstance(dirty_packages, list) or len(dirty_packages) > 10000
            or any(not isinstance(p, str) or not p.strip() for p in dirty_packages)):
        raise ValueError("Invalid dirty-package list.")
    return {"schema_version": 1, "tool_version": "m1.3", "scope": "loaded_editor_world_manifest_only",
            "observed_utc": datetime.now(timezone.utc).isoformat(), "world_path": world_path,
            "engine_version": engine_version, "room_manifest_passed": not copied,
            "issues": copied, "dirty_packages": sorted(set(dirty_packages)),
            "disk_map_verified": False, "runtime_gates": {key: "not_run" for key in NOT_RUN}}

def write_new_report(path: Path, report: dict[str, Any]) -> None:
    # Validate before creating anything. Never overwrite a previous evidence file.
    if path.suffix.lower() != ".json":
        raise ValueError("Report destination must have a .json extension.")
    if not path.parent.is_dir():
        raise ValueError("Create/select the report directory yourself first.")
    checked = make_report(report["world_path"], report["engine_version"], report["issues"], report["dirty_packages"])
    for key in ("schema_version", "tool_version", "scope", "room_manifest_passed", "disk_map_verified", "runtime_gates"):
        if report.get(key) != checked[key]:
            raise ValueError(f"Report has an unsupported claim: {key}")
    if set(report) != set(checked) or not isinstance(report["observed_utc"], str):
        raise ValueError("Report has unexpected fields or no timestamp.")
    timestamp = datetime.fromisoformat(report["observed_utc"])
    if timestamp.tzinfo is None or timestamp.utcoffset() is None:
        raise ValueError("Report observation time must include a timezone.")
    serialized = json.dumps(report, indent=2, ensure_ascii=False, allow_nan=False) + "\n"
    with path.open("x", encoding="utf-8") as handle:
        handle.write(serialized)
