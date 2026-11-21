#!/usr/bin/env python3
"""Stage (do not publish) a checked HAPPY JARZ ESP32-S3 application release.

Run only after the exact 4MB production partition map and firmware version
have been independently verified on physical hardware. This tool does NOT
build firmware, flash hardware, commit, or modify the live release catalog.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAX_APP = 0x140000


def check_partitions(path: Path) -> None:
    data = path.read_bytes()
    parts = []
    for offset in range(0, min(len(data), 0x1000) - 31, 32):
        raw = data[offset:offset + 32]
        magic, kind, subtype, start, size, label, flags = struct.unpack('<HBBII16sI', raw)
        if magic == 0xFFFF:  # partition entries end here
            break
        if magic == 0xEBEB:  # MD5 checksum record, if present
            break
        if magic != 0x50AA:
            raise ValueError(f'Invalid partition entry at byte {offset}: 0x{magic:04X}')
        if not size or start + size > 0x400000:
            raise ValueError('Invalid or out-of-range 4 MB partition')
        parts.append((label.split(bytes([0]))[0].decode('ascii'), kind, subtype, start, size))
    if not parts:
        raise ValueError('No partition entries found')
    app = [p for p in parts if p[1] == 0 and p[2] in (0, 0x10)]
    if len(app) != 1 or app[0][3:] != (0x10000, MAX_APP):
        raise ValueError(f'Unexpected main application partition: {app}; stop, do not publish')
    for label in ('nvs', 'spiffs'):
        if not any(p[0] == label and p[1] == 1 for p in parts):
            raise ValueError(f'Missing expected {label} partition; do not publish')
    for i, a in enumerate(parts):
        for b in parts[i + 1:]:
            if max(a[3], b[3]) < min(a[3] + a[4], b[3] + b[4]):
                raise ValueError(f'Partition overlap: {a[0]} and {b[0]}')
    print('Verified 4MB layout:', ', '.join(f'{p[0]}@0x{p[3]:X}/0x{p[4]:X}' for p in parts))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, required=True, help="Verified Arduino .bin application image")
    parser.add_argument("--version", required=True, help="Firmware version, e.g. 0.18.13")
    parser.add_argument("--partitions", type=Path, required=True, help="Partition table .bin from the same compile")
    parser.add_argument("--out", type=Path, default=ROOT / "release-staging")
    parser.add_argument("--verified-layout", action="store_true",
                        help="Explicitly confirm production 4MB app layout: start 0x10000, length 0x140000")
    args = parser.parse_args()
    if not args.verified_layout:
        parser.error("Verify the production partition map first; pass --verified-layout only after checking it.")
    if not re.fullmatch(r"\d+\.\d+\.\d+", args.version):
        parser.error("Version must be numeric MAJOR.MINOR.PATCH")
    recorded = (ROOT / "firmware" / "VERSION").read_text().strip()
    if recorded != args.version:
        parser.error(f"Firmware VERSION is {recorded!r}, not {args.version!r}; release a matching build.")
    try:
        check_partitions(args.partitions)
    except (ValueError, OSError) as exc:
        parser.error(str(exc))
    data = args.app.read_bytes()
    if not 256 <= len(data) <= MAX_APP or data[0] != 0xE9:
        parser.error("Image is not a plausible ESP32 app or exceeds the application partition.")
    name = f"happyjarz-{args.version}-app.bin"
    args.out.mkdir(parents=True, exist_ok=True)
    dest = args.out / name
    if dest.resolve() == args.app.resolve():
        parser.error("Output file must differ from the source image.")
    shutil.copyfile(args.app, dest)
    shutil.copyfile(args.partitions, args.out / "verified-partitions.bin")
    release = {
        "channel": "stable", "model": "happyjarz-supermini-simple-4mb",
        "hw": "V1", "chip": "ESP32-S3",
        "layout": "arduino-4mb-default-app-0x10000-0x140000",
        "flashSize": 4194304, "version": args.version,
        "images": [{
            "url": "./releases/" + name, "address": 65536,
            "size": len(data), "sha256": hashlib.sha256(data).hexdigest()
        }]
    }
    (args.out / "release-entry.json").write_text(json.dumps(release, indent=2) + "\n")
    print("Staged:", dest)
    print("SHA-256:", release["images"][0]["sha256"])
    print("Release entry:", args.out / "release-entry.json")
    print("NOT PUBLISHED. Test physical browser flashing + NVS/SPIFFS preservation first.")


if __name__ == "__main__":
    main()
