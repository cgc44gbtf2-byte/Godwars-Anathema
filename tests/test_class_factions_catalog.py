"""The new consent gate is scoped; historical source pins still remain exact."""
from pathlib import Path
import unittest
from class_factions_source import RECRUIT_GUARD, without_class_recruitment_guard

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Genesis/src"

class ClassFactionsCatalog(unittest.TestCase):
    def test_dispatch_guard_is_exactly_one_narrow_block(self):
        text = (SRC / "interp.c").read_text()
        self.assertEqual(text.count(RECRUIT_GUARD), 1)
        restored = without_class_recruitment_guard(text)
        self.assertNotEqual(text, restored)
        self.assertEqual(restored.replace(
            "      (*cmd_table[cmd].do_fun) ( ch, argument );",
            RECRUIT_GUARD + "      (*cmd_table[cmd].do_fun) ( ch, argument );", 1), text)
        self.assertEqual(without_class_recruitment_guard(restored), restored)
        with self.assertRaises(AssertionError):
            without_class_recruitment_guard(text + RECRUIT_GUARD)

    def test_recovery_does_not_change_player_format_or_class_implementation(self):
        source = (SRC / "clans.c").read_text()
        for part in ("save_char_obj_checked(ch)", "recruit_project(ch,",
                     "fsync(fileno(fp))", "fsync(directory)",
                     "next.members[t].role = MEMBER_ROLE",
                     "next.members[t].recruit_recipe = p->recruit_recipe",
                     "next.members[m].recruit_recipe = 0"):
            self.assertIn(part, source)
        self.assertIn("do_autoclass(&projected, command)", source)
        self.assertNotIn("system(", source)
        self.assertNotIn("sprintf(", source)

    def test_real_engine_coverage_and_pending_imports_are_explicit(self):
        source = (ROOT / "tests/test_clans.c").read_text()
        for part in ("native_recruitment();", "enrollment_recovery();",
                     "SAVE_FAULT_DIRSYNC", '"autoclass monk"',
                     "Classing is a specific exception", "DISC_DOMINATE] == 4"):
            self.assertIn(part, source)
        docs = (ROOT / "docs/CLASS_FACTIONS.md").read_text()
        for part in ("clan settle", "Exiled", "Loner", "not implemented",
                     "Selfbrid development remains paused", "not a\n"
                     "general inventory-trade journal"):
            self.assertIn(part, docs)

if __name__ == "__main__":
    unittest.main()
