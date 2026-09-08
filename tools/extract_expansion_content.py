"""Safely import selected Unreal Content roots from a local vendor archive."""
import argparse
import hashlib
import json
import os
import stat
import tempfile
import zlib
import zipfile
from dataclasses import dataclass
from pathlib import Path, PurePosixPath


CHUNK_SIZE = 1024 * 1024


class ImportSafetyError(RuntimeError):
    """Raised when an archive or destination cannot be imported safely."""


@dataclass(frozen=True)
class Digest:
    bytes: int
    crc32: int
    sha256: str


@dataclass(frozen=True)
class PlannedEntry:
    info: zipfile.ZipInfo
    relative: PurePosixPath
    group: str


def _selector_parts(value: str, label: str, *, single_name: bool) -> tuple[str, ...]:
    if not value or "\\" in value or "\x00" in value:
        raise ImportSafetyError(f"Unsafe {label}: {value!r}")
    path = PurePosixPath(value)
    parts = path.parts
    if (path.is_absolute() or not parts or any(part in ("", ".", "..") or ":" in part for part in parts)
            or (single_name and len(parts) != 1)):
        raise ImportSafetyError(f"Unsafe {label}: {value!r}")
    return parts


def _archive_relative(name: str) -> PurePosixPath | None:
    if "\x00" in name or "\\" in name:
        raise ImportSafetyError(f"Unsafe archive path: {name!r}")
    path = PurePosixPath(name)
    parts = path.parts
    if path.is_absolute() or ".." in parts or "." in parts or any(":" in part for part in parts):
        raise ImportSafetyError(f"Unsafe archive path: {name!r}")
    if "Content" not in parts:
        return None
    if parts.count("Content") != 1:
        raise ImportSafetyError(f"Ambiguous Content path: {name!r}")
    relative = parts[parts.index("Content") + 1:]
    if not relative:
        return None
    return PurePosixPath(*relative)


def _matches_prefix(relative: PurePosixPath, prefix: tuple[str, ...]) -> bool:
    return relative.parts[:len(prefix)] == prefix


def _is_link(path: Path) -> bool:
    return path.is_symlink() or path.is_junction()


def _reject_link_ancestors(path: Path) -> None:
    cursor = Path(path.anchor)
    for part in path.parts[1:]:
        cursor = cursor / part
        if _is_link(cursor):
            raise ImportSafetyError(f"Refusing symlink or junction path: {cursor}")


def _plan_entries(archive: zipfile.ZipFile, roots: list[str], prefixes: list[str]) -> list[PlannedEntry]:
    root_names = [_selector_parts(root, "root", single_name=True)[0] for root in roots]
    prefix_parts = [_selector_parts(prefix, "include prefix", single_name=False) for prefix in prefixes]
    if len(set(name.casefold() for name in root_names)) != len(root_names):
        raise ImportSafetyError("Duplicate requested root")
    if len(set("/".join(part.casefold() for part in value) for value in prefix_parts)) != len(prefix_parts):
        raise ImportSafetyError("Duplicate requested include prefix")

    planned: list[PlannedEntry] = []
    matched_roots = set()
    matched_prefixes = set()
    planned_paths = {}
    for info in archive.infolist():
        relative = _archive_relative(info.filename)
        if relative is not None and stat.S_ISLNK(info.external_attr >> 16):
            raise ImportSafetyError(f"Refusing archive symlink: {info.filename}")
        if relative is None or info.is_dir():
            continue
        group = None
        if root_names and relative.parts[0] in root_names:
            group = relative.parts[0]
            matched_roots.add(group)
        for original, prefix in zip(prefixes, prefix_parts):
            if group is None and _matches_prefix(relative, prefix):
                group = original
                matched_prefixes.add(original)
        if not root_names and not prefix_parts:
            group = relative.parts[0]
        if group is None:
            continue

        key = "/".join(part.casefold() for part in relative.parts)
        if key in planned_paths:
            raise ImportSafetyError(f"Duplicate archive destination: {relative.as_posix()}")
        planned_paths[key] = relative
        planned.append(PlannedEntry(info, relative, group))

    missing_roots = sorted(set(root_names) - matched_roots)
    missing_prefixes = sorted(set(prefixes) - matched_prefixes)
    if missing_roots:
        raise ImportSafetyError("Requested root not present: " + ", ".join(missing_roots))
    if missing_prefixes:
        raise ImportSafetyError("Requested include prefix not present: " + ", ".join(missing_prefixes))
    if not planned:
        raise ImportSafetyError("No Content files selected")

    for key in planned_paths:
        components = key.split("/")
        for index in range(1, len(components)):
            ancestor = "/".join(components[:index])
            if ancestor in planned_paths:
                raise ImportSafetyError(f"Archive file conflicts with child destination: {planned_paths[ancestor].as_posix()}")
    return planned


def _destination(host: Path) -> Path:
    _reject_link_ancestors(host)
    if _is_link(host) or not host.exists() or not host.is_dir():
        raise ImportSafetyError(f"Host must be an existing non-symlink directory: {host}")
    content = host / "Content"
    if _is_link(content) or not content.exists() or not content.is_dir():
        raise ImportSafetyError(f"Host Content must be an existing non-symlink directory: {content}")
    return content.resolve(strict=True)


def _safe_target(destination: Path, relative: PurePosixPath) -> Path:
    target = destination.joinpath(*relative.parts)
    cursor = destination
    for part in relative.parts[:-1]:
        cursor = cursor / part
        if _is_link(cursor):
            raise ImportSafetyError(f"Unsafe destination parent: {cursor}")
        if cursor.exists():
            if not cursor.is_dir():
                raise ImportSafetyError(f"Unsafe destination parent: {cursor}")
    if _is_link(target):
        raise ImportSafetyError(f"Refusing symlink destination: {target}")
    if not target.resolve(strict=False).is_relative_to(destination):
        raise ImportSafetyError(f"Destination escapes Content: {target}")
    return target


def _digest_stream(source, output=None) -> Digest:
    digest = hashlib.sha256()
    crc = 0
    length = 0
    while chunk := source.read(CHUNK_SIZE):
        digest.update(chunk)
        crc = zlib.crc32(chunk, crc)
        length += len(chunk)
        if output is not None:
            output.write(chunk)
    return Digest(length, crc & 0xFFFFFFFF, digest.hexdigest())


def _digest_file(path: Path) -> Digest:
    with path.open("rb") as source:
        return _digest_stream(source)


def _verify_source(info: zipfile.ZipInfo, digest: Digest) -> None:
    if digest.bytes != info.file_size or digest.crc32 != info.CRC:
        raise ImportSafetyError(f"Archive content checksum failed: {info.filename}")


def _same_content(left: Digest, right: Digest) -> bool:
    return left.bytes == right.bytes and left.crc32 == right.crc32 and left.sha256 == right.sha256


def _ensure_parent(target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    if _is_link(target.parent) or not target.parent.is_dir():
        raise ImportSafetyError(f"Unsafe destination parent: {target.parent}")


def _publish_new(archive: zipfile.ZipFile, entry: PlannedEntry, target: Path) -> tuple[str, Digest]:
    _ensure_parent(target)
    temporary_path = None
    try:
        with tempfile.NamedTemporaryFile(mode="xb", delete=False, dir=target.parent,
                                         prefix=".extract-", suffix=".tmp") as temporary:
            temporary_path = Path(temporary.name)
            with archive.open(entry.info) as source:
                source_digest = _digest_stream(source, temporary)
            temporary.flush()
            os.fsync(temporary.fileno())
        _verify_source(entry.info, source_digest)
        try:
            os.link(temporary_path, target)
        except FileExistsError:
            existing = _digest_file(target)
            if not _same_content(source_digest, existing):
                raise ImportSafetyError(f"Existing content differs: {entry.relative.as_posix()}")
            return "verified_existing", source_digest
        return "written", source_digest
    finally:
        if temporary_path is not None:
            temporary_path.unlink(missing_ok=True)


def _process_entry(archive: zipfile.ZipFile, entry: PlannedEntry, target: Path) -> tuple[str, Digest]:
    if target.exists():
        if not target.is_file():
            raise ImportSafetyError(f"Destination exists but is not a file: {entry.relative.as_posix()}")
        with archive.open(entry.info) as source:
            source_digest = _digest_stream(source)
        _verify_source(entry.info, source_digest)
        if not _same_content(source_digest, _digest_file(target)):
            raise ImportSafetyError(f"Existing content differs: {entry.relative.as_posix()}")
        return "verified_existing", source_digest
    return _publish_new(archive, entry, target)


def extract(archive_path: Path, host: Path, roots: list[str] | None = None,
            prefixes: list[str] | None = None) -> dict:
    roots = roots or []
    prefixes = prefixes or []
    destination = _destination(host)
    summaries = {}
    with zipfile.ZipFile(archive_path) as archive:
        plan = _plan_entries(archive, roots, prefixes)
        for entry in plan:
            summary = summaries.setdefault(entry.group, {
                "planned_files": 0, "planned_bytes": 0,
                "files_written": 0, "bytes_written": 0,
                "files_verified_existing": 0, "bytes_verified_existing": 0,
            })
            summary["planned_files"] += 1
            summary["planned_bytes"] += entry.info.file_size
        for entry in plan:
            target = _safe_target(destination, entry.relative)
            action, digest = _process_entry(archive, entry, target)
            summary = summaries[entry.group]
            if action == "written":
                summary["files_written"] += 1
                summary["bytes_written"] += digest.bytes
            else:
                summary["files_verified_existing"] += 1
                summary["bytes_verified_existing"] += digest.bytes

    return {
        "archive": archive_path.name,
        "archive_path": str(archive_path.resolve()),
        "destination": str(destination),
        "selected_roots": roots,
        "included_prefixes": prefixes,
        "files_planned": len(plan),
        "bytes_planned": sum(entry.info.file_size for entry in plan),
        "files_written": sum(value["files_written"] for value in summaries.values()),
        "bytes_written": sum(value["bytes_written"] for value in summaries.values()),
        "files_verified_existing": sum(value["files_verified_existing"] for value in summaries.values()),
        "bytes_verified_existing": sum(value["bytes_verified_existing"] for value in summaries.values()),
        "root_summaries": summaries,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("host", type=Path)
    parser.add_argument("--root", action="append", default=[], help="top-level Content root to import")
    parser.add_argument("--include-prefix", action="append", default=[],
                        help="additional Content-relative vendor-owned prefix to import")
    arguments = parser.parse_args()
    try:
        report = extract(arguments.archive, arguments.host, arguments.root, arguments.include_prefix)
    except (ImportSafetyError, OSError, zipfile.BadZipFile) as error:
        parser.error(str(error))
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
