#!/usr/bin/env python3
"""Keep the hybrid Werewolf UI aligned with the native gift inventory."""
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "Genesis/src"
UI = (SRC / "classes/hybrid/werewolf_training.c").read_text()
CAPS = (SRC / "classes/hybrid/specialization.c").read_text()
NATIVE = (SRC / "classes/werewolf/werewolf.c").read_text()
MODULES = (SRC / "classes/modules.mk").read_text()
SELFBRID = (SRC / "classes/hybrid/selfbrid.c").read_text()


def ui_catalog(array):
    body = UI.split(f"{array}[]={{", 1)[1].split("};", 1)[0]
    return re.findall(r'\{"([^"]+)","([^"]+)"\}', body)


def native_catalog(function, first, last):
    body = NATIVE.split(f"char *{function}", 1)[1]
    body = body.split("\nchar *", 1)[0]
    return re.findall(r'case \w+: strcpy\(disc, "([^"]+)"\);', body)


class HybridWerewolfTrainingCatalogTests(unittest.TestCase):
    def test_ui_names_match_native_gift_families(self):
        self.assertEqual([label for _, label in ui_catalog("breed_gifts")],
                         native_catalog("gift_breed_name", "BREED_HOMID", "BREED_METIS"))
        self.assertEqual([label for _, label in ui_catalog("auspice_gifts")],
                         native_catalog("gift_auspice_name", "AUSPICE_RAGABASH", "AUSPICE_AHROUN"))
        self.assertEqual([label for _, label in ui_catalog("tribe_gifts")],
                         native_catalog("gift_tribe_name", "TRIBE_BLACK_FURIES", "TRIBE_WENDIGOS"))

    def test_unique_keys_and_complete_bounded_families(self):
        for array, expected in (("breed_gifts", 3), ("auspice_gifts", 5),
                                ("tribe_gifts", 13)):
            rows = ui_catalog(array)
            self.assertEqual(len(rows), expected)
            self.assertEqual(len({key for key, _ in rows}), expected)
            self.assertTrue(all(re.fullmatch(r"[a-z]+(?:-[a-z]+)*", key)
                                for key, _ in rows))

    def test_real_module_and_atomic_training_route(self):
        self.assertIn("classes/hybrid/werewolf_training.c", MODULES)
        self.assertIn("hybrid_werewolf_training_command(ch, argument)", SELFBRID)
        self.assertIn("hybrid_training_commit(ch,&request)", UI)
        self.assertNotRegex(UI, r"\bdo_(?:gifts|advance|autoclass)\s*\(")
        self.assertNotRegex(UI, r"ch->pcdata->(?:breed|auspice|tribes)\s*\[")
        self.assertIn("HYBRID_LESSON_SPECIALIZE", UI)
        self.assertIn("HYBRID_LESSON_GIFT_SELECT", UI)
        self.assertIn("HYBRID_LESSON_RANK", UI)
        self.assertIn("conversion remains disabled", UI)

    def test_backend_caps_still_bind_native_ceilings(self):
        self.assertIn("parent == CLASS_WEREWOLF ? 5 : native", CAPS)
        self.assertIn("UMIN(native,ceiling)", CAPS)
        self.assertIn("state->secondary_progression.full_branch", CAPS)


if __name__ == "__main__":
    unittest.main()
