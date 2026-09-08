"""Validate source recipe data before Unreal creates any level. No engine or collision claim."""
from __future__ import annotations
import math
import re

def validate_layout(data: object) -> list[dict]:
    if not isinstance(data, dict) or set(data) != {"schema_version", "status", "map_id", "units", "timings_seconds", "volumes"}:
        raise ValueError("Unexpected safety layout fields")
    if type(data["schema_version"]) is not int or data["schema_version"] != 1:
        raise ValueError("Unsupported safety layout schema")
    if data["status"] != "editor_recipe_not_an_included_map" or data["map_id"] != "level.systems_test" or data["units"] != "centimeters":
        raise ValueError("Wrong source status, map or units")
    timings = {"fade_out": 0.18, "fade_in": 0.22, "ground_stability": 0.35, "settle_timeout": 2.0, "checkpoint_dwell": 0.35}
    if data["timings_seconds"] != timings:
        raise ValueError("Recipe timings must match the runtime source; JSON does not configure the component")
    rows = data["volumes"]
    if not isinstance(rows, list) or not 1 <= len(rows) <= 32:
        raise ValueError("Expected one to 32 authored safety volumes")
    labels: set[str] = set()
    for row in rows:
        if not isinstance(row, dict) or set(row) != {"label", "kind", "center_cm", "half_extent_cm"}:
            raise ValueError("Invalid safety row fields")
        label = row["label"]
        if not isinstance(label, str) or not re.fullmatch(r"DEV_Safety_[A-Za-z0-9_]{1,64}", label) or label in labels:
            raise ValueError("Invalid or duplicate development label")
        labels.add(label)
        if not isinstance(row["kind"], str) or row["kind"] not in {"DEEP_WATER", "OUT_OF_BOUNDS", "DRY_CHECKPOINT"}:
            raise ValueError("Unknown safety kind")
        for key in ("center_cm", "half_extent_cm"):
            v = row[key]
            if not isinstance(v, list) or len(v) != 3 or any(type(x) not in (int, float) or abs(x) > 1000000 or not math.isfinite(x) for x in v):
                raise ValueError("Expected three finite coordinates")
            if any(abs(x) > 1000000 for x in v) or (key == "half_extent_cm" and any(x <= 0 for x in v)):
                raise ValueError("Coordinate or extent outside source recipe bounds")
    return rows
