"""No name-derived staff elevation path may re-enter the player command table."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Genesis/src"


class StaffGateChecks(unittest.TestCase):
    def test_relevel_is_not_registered(self):
        commands = (SRC / "interp.c").read_text()
        self.assertNotRegex(commands, r'\{\s*"relevel"\s*,')
        self.assertNotIn("do_relevel", commands)

    def test_name_derived_handler_is_removed(self):
        self.assertNotIn("do_relevel", (SRC / "kav_wiz.c").read_text())
        self.assertNotIn("do_relevel", (SRC / "merc.h").read_text())

    def test_staging_and_delevel_permissions_remain_staff_only(self):
        commands = (SRC / "interp.c").read_text()
        self.assertRegex(commands, r'\{\s*"delevel"\s*,\s*do_delevel\s*,'
                         r'[^}]*\bLEV_IMP\s*,\s*LOG_ALWAYS')
        self.assertIn("if (IS_NPC(ch)) return;", (SRC / "kav_wiz.c").read_text())

    def test_no_reserved_name_grants_staff_level(self):
        source = (SRC / "kav_wiz.c").read_text()
        self.assertNotRegex(source, r'(?s)is_name\s*\(\s*ch->name\s*,'
                            r'.{0,300}\bLEV_(?:ST|IMP)\b')


if __name__ == "__main__":
    unittest.main()
