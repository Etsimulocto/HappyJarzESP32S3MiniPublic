"""Safety checks for binary 4MB HAPPY JARZ partition layout."""
from pathlib import Path
from tempfile import TemporaryDirectory
import importlib.util
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("stage_web_release", ROOT / "tools" / "stage_web_release.py")
stage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(stage)


def entry(kind, subtype, address, size, label):
    name = label.encode("ascii").ljust(16, bytes([0]))
    return struct.pack("<HBBII16sI", 0x50AA, kind, subtype, address, size, name, 0)


def table(app_size=0x140000, spiffs=True):
    rows = [
        entry(1, 2, 0x9000, 0x5000, "nvs"),
        entry(1, 0, 0xE000, 0x2000, "otadata"),
        entry(0, 0x10, 0x10000, app_size, "app0"),
        entry(0, 0x11, 0x150000, 0x140000, "app1")
    ]
    if spiffs:
        rows.append(entry(1, 0x82, 0x290000, 0x160000, "spiffs"))
    return b"".join(rows) + bytes([255]) * 32


class PartitionSafetyTests(unittest.TestCase):
    def verify(self, data):
        with TemporaryDirectory() as td:
            path = Path(td) / "partitions.bin"
            path.write_bytes(data)
            return stage.check_partitions(path)

    def test_arduino_4mb_default_layout_is_valid(self):
        self.verify(table())

    def test_reject_wrong_main_app_size(self):
        with self.assertRaises(ValueError):
            self.verify(table(app_size=0x180000))

    def test_reject_missing_art_filesystem(self):
        with self.assertRaises(ValueError):
            self.verify(table(spiffs=False))

    def test_reject_invalid_magic(self):
        with self.assertRaises(ValueError):
            self.verify(b"bad partition table")

    def test_reject_overlapping_partitions(self):
        raw = table().replace(
            entry(0, 0x11, 0x150000, 0x140000, "app1"),
            entry(0, 0x11, 0x140000, 0x140000, "app1")
        )
        with self.assertRaises(ValueError):
            self.verify(raw)


if __name__ == "__main__":
    unittest.main()
