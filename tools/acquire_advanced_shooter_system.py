#!/usr/bin/env python3
"""Acquire and verify the Fab Advanced Shooter System BuildPatch payload.

The script uses Epic's BuildPatchTool for manifest/chunk validation and then
independently verifies every reconstructed file's size and SHA-256 digest.  It
keeps the raw BuildPatch layout and an exact, atomically replaced plugin stage
under the caller-provided vendor directory; it never installs into an engine.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import sys
import tempfile
from typing import Any


DEFAULT_MANIFEST = Path(
    "C:/ProgramData/Epic/EpicGamesLauncher/VaultCache/FabLibrary/"
    "Advanced_Networked_Shooter_System__Core_Plugin__V1_0-0a897de8/"
    "unreal-engine/manifest"
)
DEFAULT_TOOL = Path(
    "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/BuildPatchTool.exe"
)
DEFAULT_OUTPUT = Path("F:/coastline/LocalVendor/AdvancedShooterSystem")
VENDOR_ROOT = Path("F:/coastline/LocalVendor")
EXPECTED_APP = "Advanced22548e075794V1"
PLUGIN_PREFIX = PurePosixPath("Engine/Plugins/Marketplace") / EXPECTED_APP


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def require_within(path: Path, parent: Path) -> Path:
    resolved = path.resolve()
    resolved.relative_to(parent.resolve())
    return resolved


def run_tool(tool: Path, arguments: list[str], log_path: Path) -> None:
    process = subprocess.run(
        [str(tool), *arguments],
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    log_path.write_text(process.stdout, encoding="utf-8")
    if process.returncode != 0:
        raise RuntimeError(
            f"BuildPatchTool exited {process.returncode}; see {log_path}"
        )


def load_metadata(path: Path) -> dict[str, Any]:
    metadata = json.loads(path.read_text(encoding="utf-8-sig"))
    if metadata.get("AppName") != EXPECTED_APP:
        raise RuntimeError(
            f"Unexpected AppName {metadata.get('AppName')!r}; expected {EXPECTED_APP}"
        )
    if not isinstance(metadata.get("Files"), list) or not metadata["Files"]:
        raise RuntimeError("Manifest metadata has no files")
    if not isinstance(metadata.get("FileChunks"), list) or not metadata["FileChunks"]:
        raise RuntimeError("Manifest metadata has no chunks")
    return metadata


def verify_files(root: Path, files: list[dict[str, Any]]) -> list[str]:
    errors: list[str] = []
    for entry in files:
        relative = PurePosixPath(entry["Filename"])
        if relative.is_absolute() or ".." in relative.parts:
            errors.append(f"unsafe manifest path: {relative}")
            continue
        path = root.joinpath(*relative.parts)
        if not path.is_file():
            errors.append(f"missing: {relative}")
            continue
        expected_size = int(entry["FileSize"])
        actual_size = path.stat().st_size
        if actual_size != expected_size:
            errors.append(
                f"size mismatch: {relative} ({actual_size} != {expected_size})"
            )
            continue
        actual_hash = sha256_file(path)
        if actual_hash != str(entry["SHA256"]).upper():
            errors.append(f"SHA-256 mismatch: {relative}")
    return errors


def plugin_entries(files: list[dict[str, Any]]) -> list[dict[str, Any]]:
    prefix = str(PLUGIN_PREFIX).replace("\\", "/") + "/"
    selected = [
        entry
        for entry in files
        if str(entry["Filename"]).replace("\\", "/").startswith(prefix)
    ]
    if len(selected) != len(files):
        raise RuntimeError("Manifest contains files outside the expected plugin prefix")
    return selected


def make_exact_stage(
    output_root: Path, stage_root: Path, files: list[dict[str, Any]]
) -> None:
    stage_parent = stage_root.parent
    stage_parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="advanced-shooter-stage-", dir=stage_parent) as temp:
        temp_root = Path(temp) / EXPECTED_APP
        for entry in files:
            full_relative = PurePosixPath(entry["Filename"])
            plugin_relative = full_relative.relative_to(PLUGIN_PREFIX)
            source = output_root.joinpath(*full_relative.parts)
            destination = temp_root.joinpath(*plugin_relative.parts)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)

        staged_manifest_entries: list[dict[str, Any]] = []
        for entry in files:
            cloned = dict(entry)
            full_relative = PurePosixPath(entry["Filename"])
            cloned["Filename"] = str(full_relative.relative_to(PLUGIN_PREFIX))
            staged_manifest_entries.append(cloned)
        stage_errors = verify_files(temp_root, staged_manifest_entries)
        if stage_errors:
            raise RuntimeError("Staged payload verification failed:\n" + "\n".join(stage_errors))

        previous = stage_root.with_name(stage_root.name + ".previous")
        if previous.exists():
            shutil.rmtree(previous)
        if stage_root.exists():
            stage_root.replace(previous)
        temp_root.replace(stage_root)
        if previous.exists():
            shutil.rmtree(previous)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--build-patch-tool", type=Path, default=DEFAULT_TOOL)
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--verify-only",
        action="store_true",
        help="Verify and restage an existing payload without downloading missing files.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    manifest = args.manifest.resolve()
    tool = args.build_patch_tool.resolve()
    output_root = require_within(args.output_root, VENDOR_ROOT)
    if not manifest.is_file():
        raise FileNotFoundError(manifest)
    if not tool.is_file():
        raise FileNotFoundError(tool)

    evidence = output_root / "Evidence"
    evidence.mkdir(parents=True, exist_ok=True)
    named_manifest = evidence / "AdvancedShooterSystem.manifest"
    shutil.copy2(manifest, named_manifest)

    metadata_path = evidence / "manifest-metadata.json"
    run_tool(
        tool,
        [
            "-mode=ExtractMetaData",
            f"-InputFile={named_manifest}",
            f"-OutputFile={metadata_path}",
            "-OutputFormat=json",
        ],
        evidence / "metadata-extraction.log",
    )
    metadata = load_metadata(metadata_path)
    files = plugin_entries(metadata["Files"])

    enumeration_path = evidence / "manifest-chunks.txt"
    run_tool(
        tool,
        [
            "-mode=Enumeration",
            f"-InputFile={named_manifest}",
            f"-OutputFile={enumeration_path}",
            "-IncludeSizes",
        ],
        evidence / "chunk-enumeration.log",
    )
    enumerated_chunks = [
        line for line in enumeration_path.read_text(encoding="utf-8-sig").splitlines() if line
    ]
    if len(enumerated_chunks) != len(metadata["FileChunks"]):
        raise RuntimeError(
            "Chunk enumeration count does not match manifest metadata "
            f"({len(enumerated_chunks)} != {len(metadata['FileChunks'])})"
        )
    parsed_chunks: list[tuple[str, int]] = []
    for line in enumerated_chunks:
        fields = line.rsplit("\t", 1)
        if len(fields) != 2 or not fields[0].startswith("ChunksV"):
            raise RuntimeError(f"Malformed BuildPatch chunk enumeration: {line!r}")
        parsed_chunks.append((fields[0], int(fields[1])))
    if len({name for name, _ in parsed_chunks}) != len(parsed_chunks):
        raise RuntimeError("BuildPatch chunk enumeration contains duplicate paths")
    enumerated_download_bytes = sum(size for _, size in parsed_chunks)
    if enumerated_download_bytes != int(metadata.get("DownloadSize", -1)):
        raise RuntimeError(
            "Enumerated chunk bytes do not match manifest DownloadSize "
            f"({enumerated_download_bytes} != {metadata.get('DownloadSize')})"
        )
    if any(
        len(str(chunk.get("SHA1", ""))) != 40 for chunk in metadata["FileChunks"]
    ):
        raise RuntimeError("Manifest has a missing or malformed chunk SHA-1")

    errors = verify_files(output_root, files)
    if errors and not args.verify_only:
        base_urls = metadata.get("BaseUrl") or metadata.get("BaseUrls")
        if isinstance(base_urls, str):
            base_urls = [base_urls]
        if not base_urls:
            raise RuntimeError("Manifest metadata has no public cloud base URL")
        run_tool(
            tool,
            [
                "-mode=InstallManifest",
                f"-Manifest={named_manifest}",
                f"-OutputDir={output_root}",
                f"-CloudDirs={','.join(base_urls)}",
                "-RejectSymlinks",
            ],
            evidence / "install.log",
        )
        errors = verify_files(output_root, files)
    if errors:
        raise RuntimeError("Payload verification failed:\n" + "\n".join(errors))

    stage_root = output_root / "Staged" / EXPECTED_APP
    make_exact_stage(output_root, stage_root, files)

    source_files = [
        entry["Filename"] for entry in files if "/Source/" in entry["Filename"]
    ]
    report = {
        "schema": 1,
        "result": "verified",
        "app_name": metadata["AppName"],
        "version": metadata.get("VersionString"),
        "manifest_sha256": sha256_file(named_manifest),
        "file_count": len(files),
        "file_bytes": sum(int(entry["FileSize"]) for entry in files),
        "chunk_count": len(enumerated_chunks),
        "download_bytes": enumerated_download_bytes,
        "source_file_count": len(source_files),
        "source_files": source_files,
        "raw_install_root": str(output_root),
        "stage_root": str(stage_root),
        "chunk_verification": (
            "Epic BuildPatchTool InstallManifest reconstructs and validates manifest chunks; "
            "Enumeration count also matches FileChunks metadata."
        ),
        "file_verification": "Every manifest file independently matched FileSize and SHA256.",
    }
    report_path = evidence / "acquisition-report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
