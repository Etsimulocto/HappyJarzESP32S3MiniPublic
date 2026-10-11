import unittest
from datetime import date

from app import HappyJarzApp


class BirthdayProfileTest(unittest.TestCase):
    def test_birthday_accepts_month_day_and_leap_day(self):
        self.assertEqual(HappyJarzApp._clean_birthday("6-11"), "06-11")
        self.assertEqual(HappyJarzApp._clean_birthday("02-29"), "02-29")
        self.assertEqual(HappyJarzApp._clean_birthday(""), "")

    def test_birthday_rejects_invalid_dates(self):
        for value in ("13-01", "04-31", "02-30", "birthday"):
            with self.subTest(value=value):
                with self.assertRaises(ValueError):
                    HappyJarzApp._clean_birthday(value)

    def test_next_birthday_includes_today(self):
        self.assertEqual(
            HappyJarzApp._next_birthday("10-10", date(2026, 10, 10)),
            date(2026, 10, 10),
        )

    def test_next_birthday_rolls_to_next_year(self):
        self.assertEqual(
            HappyJarzApp._next_birthday("06-11", date(2026, 10, 10)),
            date(2027, 6, 11),
        )

    def test_february_29_uses_march_first_in_non_leap_year(self):
        self.assertEqual(
            HappyJarzApp._next_birthday("02-29", date(2026, 2, 28)),
            date(2026, 3, 1),
        )


if __name__ == "__main__":
    unittest.main()
