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
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAX_APP = 0x140000


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, required=True, help="Verified Arduino .bin application image")
    parser.add_argument("--version", required=True, help="Firmware version, e.g. 0.18.13")
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
    data = args.app.read_bytes()
    if not 256 <= len(data) <= MAX_APP or data[0] != 0xE9:
        parser.error("Image is not a plausible ESP32 app or exceeds the application partition.")
    name = f"happyjarz-{args.version}-app.bin"
    args.out.mkdir(parents=True, exist_ok=True)
    dest = args.out / name
    if dest.resolve() == args.app.resolve():
        parser.error("Output file must differ from the source image.")
    shutil.copyfile(args.app, dest)
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
