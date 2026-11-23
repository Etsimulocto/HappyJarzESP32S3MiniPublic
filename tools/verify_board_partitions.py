#!/usr/bin/env python3
"""Compare the real ESP32 board partition table against a staged release.

Board table must be read from flash (offset 0x8000, length 0x1000) on
the EXACT board being updated, with all controllers closed.
"""
import argparse
from pathlib import Path
from stage_web_release import check_partitions


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--built", required=True, type=Path)
    p.add_argument("--board", required=True, type=Path)
    a = p.parse_args()
    expected = check_partitions(a.built)
    actual = check_partitions(a.board)
    if expected != actual:
        print("BUILT:", expected)
        print("BOARD:", actual)
        p.error("Installed partition layout differs. DO NOT FLASH.")
    print("BOARD PARTITION MATCH CONFIRMED. Application-only test may proceed.")
    print("This check does not back up settings or prove app binary compatibility.")


if __name__ == "__main__":
    main()
