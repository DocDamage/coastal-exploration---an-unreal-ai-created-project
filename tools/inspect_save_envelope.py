#!/usr/bin/env python3
"""Inspect outer save integrity without deserializing Unreal objects.
Optional --corrupt-copy creates a NEW broken test copy; never edits a source save.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import struct
import sys
import zlib

HEADER = struct.Struct('<8sIII')
LIMIT = 16 * 1024 * 1024


def inspect(data: bytes) -> dict:
    result = {'status': 'corrupt', 'file_bytes': len(data)}
    if len(data) < HEADER.size or len(data) > LIMIT + HEADER.size:
        return {**result, 'reason': 'invalid_length'}
    magic, version, size, crc = HEADER.unpack_from(data)
    if magic != b'COASTSAV':
        return {**result, 'reason': 'invalid_magic'}
    result.update(envelope_version=version, payload_bytes=size, stored_crc32=f'{crc:08x}')
    if version != 1:
        return {**result, 'status': 'unsupported', 'reason': 'unsupported_envelope_version'}
    if not 0 < size <= LIMIT or size != len(data) - HEADER.size:
        return {**result, 'reason': 'payload_length_mismatch'}
    actual = zlib.crc32(data[HEADER.size:])
    result['computed_crc32'] = f'{actual:08x}'
    if actual != crc:
        return {**result, 'reason': 'checksum_mismatch'}
    return {**result, 'status': 'integrity_valid',
            'scope': 'Outer envelope only. No validation of Unreal object, inventory, campaign, or quest.'}


def corrupt_copy(source: Path, destination: Path) -> None:
    if source.resolve() == destination.resolve():
        raise ValueError('Refusing to overwrite the source save.')
    if source.stat().st_size > LIMIT + HEADER.size:
        raise ValueError('Source exceeds the save size limit.')
    data = bytearray(source.read_bytes())
    if inspect(data)['status'] != 'integrity_valid':
        raise ValueError('The source must have a valid outer envelope before creating a corruption fixture.')
    data[HEADER.size + (len(data) - HEADER.size) // 2] ^= 1
    # Exclusive creation protects an existing destination, including a live save.
    with destination.open('xb') as output:
        output.write(data)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('save', type=Path)
    parser.add_argument('--corrupt-copy', type=Path, metavar='NEW_TEST_FILE')
    args = parser.parse_args()
    try:
        if args.save.stat().st_size > LIMIT + HEADER.size:
            raise ValueError('Save exceeds size limit; not read.')
        report = inspect(args.save.read_bytes())
        if args.corrupt_copy:
            corrupt_copy(args.save, args.corrupt_copy)
            report['corruption_fixture_created'] = str(args.corrupt_copy)
        print(json.dumps(report, indent=2))
        return 0 if report['status'] == 'integrity_valid' else 1
    except (OSError, ValueError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
