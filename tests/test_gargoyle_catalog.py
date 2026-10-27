"""Gargoyle replacement boundaries and strict native-source preservation."""
import hashlib
from pathlib import Path
import unittest
from gargoyle_source import PROOFS, without_gargoyle_stage
from gargoyle_reflection_source import PROOFS as REFLECTION_PROOFS, without_gargoyle_reflection
from gargoyle_menu_source import PROOFS as MENU_PROOFS, without_gargoyle_menu

ROOT = Path(__file__).resolve().parents[1]

def blob(text):
    data = text.encode()
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()

class GargoyleCatalog(unittest.TestCase):
    def test_menu_only_inverse_preserves_native_modules(self):
        self.assertEqual(len(MENU_PROOFS), 2)
        for path, expected, hunks in MENU_PROOFS:
            text = (ROOT / path).read_text()
            self.assertEqual(blob(without_gargoyle_menu(text)), expected)
            for new, _ in hunks:
                self.assertEqual(text.count(new), 1)
                mutated = text.replace(new, new + "/* mutation */\n", 1)
                self.assertNotEqual(blob(without_gargoyle_menu(mutated)), expected)

    def test_native_vampire_relic_slots_match_gear_cases(self):
        text = (ROOT / "Genesis/area/relic.are").read_text()
        gear = (ROOT / "tests/test_class_gear.c").read_text()
        self.assertIn("const long masks[] = {ITEM_WEAR_NECK, ITEM_WEAR_ABOUT, ITEM_WEAR_BODY}", gear)
        self.assertIn("const long shipped_masks[] = {5, 1025, 9}", gear)
        for vnum, mask in ((29630, 5), (29631, 1025), (29632, 9)):
            record = text.split(f"#{vnum}\n", 1)[1]
            header = record.split("~", 4)[4].splitlines()[1].split()
            self.assertEqual(int(header[0]), 9)  # ITEM_ARMOR
            self.assertEqual(int(header[2]), mask)

    def test_reflection_native_inverse_and_mutation_guards(self):
        self.assertEqual(len(REFLECTION_PROOFS), 3)
        for path, expected, hunks in REFLECTION_PROOFS:
            text = (ROOT / path).read_text()
            self.assertEqual(blob(without_gargoyle_reflection(text)), expected)
            self.assertEqual(without_gargoyle_reflection(
                without_gargoyle_reflection(text)), without_gargoyle_reflection(text))
            for new, _ in hunks:
                self.assertEqual(text.count(new), 2 if path.endswith("/comm.c") else 1)
                mutated = text.replace(new, new + "\n/* mutation */\n", 1)
                self.assertNotEqual(blob(without_gargoyle_reflection(mutated)), expected)

    def test_reflection_is_rank_five_actual_damage_and_deferred(self):
        damage = (ROOT / "Genesis/src/combat_damage.c").read_text()
        for part in ("gargoyle_visceratika(victim) == 5", "(hit_before - victim->hit) / 4",
                     "!gargoyle_dispatching_reflection", "gargoyle_reflection_live",
                     "entry->source_id == source_id", "LONG_MAX - amount"):
            self.assertIn(part, damage)
        comm = (ROOT / "Genesis/src/comm.c").read_text()
        self.assertEqual(comm.count("        gargoyle_reflection_pulse();"), 2)
        self.assertIn("gargoyle_reflection();", (ROOT / "tests/test_clans.c").read_text())

    def test_thirteen_native_modules_restore_exact_base(self):
        self.assertEqual(len(PROOFS), 13)
        for path, expected, _ in PROOFS:
            with self.subTest(path=path):
                text = (ROOT / path).read_text()
                old = without_gargoyle_stage(text)
                self.assertEqual(blob(old), expected)
                self.assertEqual(without_gargoyle_stage(old), old)
                self.assertNotEqual(blob(without_gargoyle_stage(text + "\n/* unrelated */\n")), expected)

    def test_guard_mutations_remain_visible(self):
        mutations = 0
        for path, expected, hunks in PROOFS:
            for new, _ in hunks:
                for needle in ("1002", "rank <= 5", "Visceratika", "IS_GARGOYLE"):
                    if needle not in new:
                        continue
                    text = (ROOT / path).read_text()
                    mutated = text.replace(new, new.replace(needle, "MUTATION", 1), 1)
                    self.assertNotEqual(blob(without_gargoyle_stage(mutated)), expected)
                    mutations += 1
        self.assertGreater(mutations, 10)

    def test_identity_and_shared_slot_are_not_social_labels(self):
        header = (ROOT / "Genesis/src/merc.h").read_text()
        predicate = header.split("static inline bool is_gargoyle(", 1)[1].split(
            "#define IS_GARGOYLE", 1)[0]
        for part in ("IS_VAMPIRE(ch)", "player_load_failed", "VAMPIRE_BUILD_GARGOYLE"):
            self.assertIn(part, predicate)
        self.assertNotIn("ch->clan", predicate)
        info = (ROOT / "Genesis/src/act_info.c").read_text()
        self.assertEqual(info.count(
            "if (IS_GARGOYLE(ch) && disc == DISC_THAUMATURGY) return 0;"), 2)
        clans = (ROOT / "Genesis/src/clans.c").read_text()
        self.assertEqual(clans.count('!strcmp(recruit_recipes[registry.members[m].recruit_recipe - 1].command,\n'
                                    '                     "vampire gargoyle")'), 1)

    def test_real_engine_and_wire_tests_are_kept(self):
        tests = (ROOT / "tests/test_clans.c").read_text()
        for part in ("gargoyle();", "LONG_MAX", "__int128", "teachdisc accept Leader visceratika",
                     '"punch Leader"', '"clan leave"', "load_char_short(&reload",
                     'damage(&actors[1], &actors[0], 1000', '"clearclass Other"'):
            self.assertIn(part, tests)
        ci = (ROOT / ".github/workflows/ci.yml").read_text()
        self.assertEqual(ci.count("python3 tests/smoke_gargoyle.py"), 2)
        docs = (ROOT / "docs/GARGOYLE.md").read_text()
        for part in ("generation 9", "15 × next rank", "pre-Gargoyle", "not imported"):
            self.assertIn(part, docs)

if __name__ == "__main__":
    unittest.main()
