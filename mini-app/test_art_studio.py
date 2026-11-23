import unittest

from art_studio import (
    HEIGHT, PIXELS, STAMP_NAMES, WIDTH, HappyJarzArtStudio,
    circle_points, gallery_transfer_commands, pack_xbm, stamp_points,
)


class PixelArtHelpersTest(unittest.TestCase):
    def test_xbm_is_1024_bytes_and_packs_lsb_first_by_row(self):
        pixels = [0] * PIXELS
        pixels[0] = 1
        pixels[7] = 1
        pixels[8] = 1
        pixels[WIDTH] = 1
        packed = pack_xbm(pixels)
        self.assertEqual(len(packed), 1024)
        self.assertEqual(packed[0], 0b10000001)
        self.assertEqual(packed[1], 0b00000001)
        self.assertEqual(packed[16], 0b00000001)

    def test_circle_is_clipped_and_symmetric(self):
        points = circle_points(64, 32, 8)
        self.assertIn((72, 32), points)
        self.assertIn((56, 32), points)
        self.assertIn((64, 24), points)
        self.assertIn((64, 40), points)
        self.assertTrue(all(0 <= x < WIDTH and 0 <= y < HEIGHT for x, y in points))
        self.assertEqual({(x - 64, y - 32) for x, y in points},
                         {(-(x - 64), y - 32) for x, y in points})

    def test_every_stamp_draws_a_clipped_shape(self):
        for name in STAMP_NAMES:
            with self.subTest(name=name):
                points = stamp_points(name, 64, 32, 1)
                self.assertTrue(points)
                self.assertTrue(all(0 <= x < WIDTH and 0 <= y < HEIGHT for x, y in points))
                larger = stamp_points(name, 64, 32, 3)
                self.assertGreater(len(larger), len(points))

    def test_stamps_clip_at_canvas_edge(self):
        for name in STAMP_NAMES:
            points = stamp_points(name, 0, 0, 3)
            self.assertTrue(all(0 <= x < WIDTH and 0 <= y < HEIGHT for x, y in points))

    def test_legacy_art_loads_as_one_frame(self):
        legacy = {"name": "Old face", "category": "Faces", "pixels": [1, 0, 1]}
        art = HappyJarzArtStudio._clean_asset(legacy)
        self.assertEqual(len(art["frames"]), 1)
        self.assertEqual(art["frames"][0][:3], [1, 0, 1])
        self.assertEqual(len(art["frames"][0]), PIXELS)

    def test_two_frame_art_is_bounded_and_keeps_both_frames(self):
        first = [0] * PIXELS
        second = [0] * PIXELS
        first[0] = 1
        second[1] = 1
        art = HappyJarzArtStudio._clean_asset({"name": "blink", "frames": [first, second, first]})
        self.assertEqual(len(art["frames"]), 2)
        self.assertEqual(len(pack_xbm(art["frames"][0]) + pack_xbm(art["frames"][1])), 2048)
        self.assertEqual(art["frames"][0][0], 1)
        self.assertEqual(art["frames"][1][1], 1)

    def test_two_frame_transfer_uses_small_sequential_chunks_and_verifies_gallery(self):
        first = [0] * PIXELS
        second = [0] * PIXELS
        commands = gallery_transfer_commands([{"pixels": first, "frames": [first, second]}])
        data = [command for command in commands if command.startswith("ART DATA ")]
        self.assertEqual(len(data), 43)
        self.assertEqual(data[0].split()[3], "0")
        self.assertEqual(data[-1].split()[3], "2016")
        self.assertTrue(all(len(command) < 192 for command in commands))
        self.assertEqual(commands[-2:], ["ART TRIM 1", "ART COUNT"])

    def test_transfer_rejects_more_than_fifty_pictures(self):
        with self.assertRaises(ValueError):
            gallery_transfer_commands([{"pixels": [0] * PIXELS}] * 51)


if __name__ == "__main__":
    unittest.main()
