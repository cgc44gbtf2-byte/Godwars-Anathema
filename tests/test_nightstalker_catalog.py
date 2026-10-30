"""Nightstalker identity, reviewed source boundaries and native preservation."""
import hashlib
from pathlib import Path
import unittest
from nightstalker_source import PROOFS, without_nightstalker_stage
from gargoyle_source import without_gargoyle_stage

ROOT = Path(__file__).resolve().parents[1]

def blob(text):
    data = text.encode()
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()

class NightstalkerCatalog(unittest.TestCase):
    def test_all_six_native_modules_restore_exact_base(self):
        for path, expected, _ in PROOFS:
            with self.subTest(path=path):
                text = (ROOT / path).read_text()
                old = without_nightstalker_stage(text)
                self.assertEqual(blob(old), expected)
                self.assertEqual(without_nightstalker_stage(old), old)
                self.assertNotEqual(blob(without_nightstalker_stage(text + "\n/* unrelated */\n")), expected)

    def test_changed_cost_or_identity_is_not_normalized_away(self):
        for path, expected, hunks in PROOFS:
            for new, _ in hunks:
                needle = "60" if "60" in new else "1001" if "1001" in new else None
                if needle is not None:
                    # Mutate the earlier stage after undoing the later,
                    # separately pinned edits whose context may overlap it.
                    text = without_gargoyle_stage((ROOT / path).read_text())
                    self.assertIn(new, text)
                    changed = text.replace(new, new.replace(needle, "9999", 1), 1)
                    self.assertNotEqual(blob(without_nightstalker_stage(changed)), expected)

    def test_native_build_and_registry_not_clan_labels(self):
        src = ROOT / "Genesis/src"
        header = (src / "merc.h").read_text()
        predicate = header.split("static inline bool is_nightstalker(", 1)[1].split(
            "#define IS_NIGHTSTALKER", 1)[0]
        self.assertIn("IS_NINJA(ch)", predicate)
        self.assertIn("player_load_failed", predicate)
        self.assertIn("cland[0] == NINJA_BUILD_NIGHTSTALKER", predicate)
        self.assertNotIn("ch->clan", predicate)
        self.assertIn('IS_NIGHTSTALKER(&preview) != IS_NIGHTSTALKER(sponsor)',
                      (src / "clans.c").read_text())
        self.assertIn('victim->pcdata->cland[0]', (src / "clearclass.c").read_text())
        save = (src / "save.c").read_text()
        self.assertIn('Clandisc', save)

    def test_real_engine_boundary_coverage_is_retained(self):
        tests = (ROOT / "tests/test_clans.c").read_text()
        for part in ('nightstalker();', 'ki <= 60', '"autoclass ninja nightstalker"',
                     '"clan leave"', '"clan kick Other"', 'load_char_obj(&reloaded, "Other")',
                     'AFF_BLIND', 'AFF_SHADOWPLANE', 'BLINDFOLDED', 'LONG_MAX'):
            self.assertIn(part, tests)
        self.assertIn("movement pool does not exist", (ROOT / "docs/NIGHTSTALKER.md").read_text())

if __name__ == "__main__":
    unittest.main()
