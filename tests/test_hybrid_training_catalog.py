#!/usr/bin/env python3
"""Guard native build mappings without importing or executing a game server."""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "Genesis/src"
BUILD = (SRC / "classes/hybrid/build.c").read_text()
NATIVE = (SRC / "clan.c").read_text().split("void do_autoclass(", 1)[1]
ROWS = [
    (int(i), parent, name, tuple(v.strip() for v in branches.split(",")), color)
    for i, parent, name, branches, color in re.findall(
        r'\{(\d+),(CLASS_\w+),"([^"]+)",\{([^}]+)\},(\w+)\}', BUILD)
]


class NativeBuildCatalogTests(unittest.TestCase):
    def native_clans(self, parent):
        result = {}
        for name, body in re.findall(r'clan = "([^"]+)";(.*?)(?=\n\s*\})',
                                     NATIVE, re.S):
            branches = dict(re.findall(
                r"cland\[(\d)\]\s*=\s*(DISC_\w+|SPHERE_\w+|-1)", body))
            if not branches:
                continue
            vampire = branches["0"].startswith("DISC_")
            if vampire == (parent == "CLASS_VAMPIRE"):
                result[name] = tuple(branches.get(str(i), "-1") for i in range(3))
        return result

    def test_exact_native_vampire_clans_and_disciplines(self):
        self.assertEqual({name: branches for _, parent, name, branches, _ in ROWS
                          if parent == "CLASS_VAMPIRE"},
                         self.native_clans("CLASS_VAMPIRE"))
        self.assertEqual(len(self.native_clans("CLASS_VAMPIRE")), 9)

    def test_exact_native_mage_traditions_and_fixed_spheres(self):
        self.assertEqual({name: branches for _, parent, name, branches, _ in ROWS
                          if parent == "CLASS_MAGE"},
                         self.native_clans("CLASS_MAGE"))
        self.assertEqual(len(self.native_clans("CLASS_MAGE")), 10)

    def test_exact_native_dragon_colors(self):
        native = dict(re.findall(
            r'!str_cmp\(arg2, "([^"]+)"\)\) ch->pcdata->dragcol = (\w+);', NATIVE))
        selected = {name.lower(): color for _, parent, name, _, color in ROWS
                    if parent == "CLASS_DRAGON"}
        self.assertEqual(selected, native)
        self.assertEqual(len(selected), 6)

    def test_stable_unique_build_ids(self):
        self.assertEqual(len(ROWS), 25)
        self.assertEqual(len({row[0] for row in ROWS}), 25)
        self.assertEqual([row[0] for row in ROWS],
                         list(range(1, 10)) + list(range(101, 111)) + list(range(201, 207)))

    def test_no_primary_or_social_clan_initializers(self):
        staged = BUILD + (SRC / "classes/hybrid/training.c").read_text()
        self.assertNotRegex(staged, r"\bdo_(?:autoclass|clanname|clans|embrace)\s*\(")
        self.assertNotRegex(staged, r"ch->(?:class|clan)\s*=(?!=)")
        self.assertNotRegex(staged, r"\bclan_(?:join|create|promote)\s*\(")

    def test_training_is_explicit_secondary_only_and_conversion_stays_disabled(self):
        preview = (SRC / "classes/hybrid/selfbrid.c").read_text()
        self.assertNotIn("hybrid_training_commit(", preview)
        self.assertNotIn("hybrid_player_attach(", preview)
        self.assertIn("hybrid_monk_training_command(ch, argument)", preview)
        self.assertIn("hybrid_werewolf_training_command(ch, argument)", preview)
        self.assertIn("hybrid_colors_command(ch, argument)", preview)
        self.assertIn("conversion is not enabled", preview)
        trainer = (SRC / "classes/hybrid/training.c").read_text()
        self.assertIn("hybrid_player_commit(", trainer)
        self.assertNotRegex(trainer, r"\b(?:fopen|fwrite|fprintf|rename)\s*\(")
        wolf = (SRC / "classes/hybrid/werewolf_training.c").read_text()
        self.assertIn("HYBRID_LESSON_SPECIALIZE", wolf)
        self.assertIn("HYBRID_LESSON_GIFT_SELECT", wolf)
        self.assertIn("HYBRID_LESSON_RANK", wolf)
        self.assertIn("hybrid_training_commit(ch,&request)", wolf)
        self.assertIn("one permanent full family", wolf)
        self.assertIn("the other two cap at 3", wolf)


if __name__ == "__main__":
    unittest.main()
