"""Original native mastery inventory: no conversion and no native cap changes."""
import hashlib
import re
import unittest
from test_class_layout import ROOT, SRC, make_values


class HybridMasteryCatalog(unittest.TestCase):
    def test_real_engine_suite_and_module(self):
        self.assertIn("Genesis/src/classes/hybrid/mastery.c", make_values(ROOT, "ENGINE_SOURCES"))
        self.assertIn("tests/test_hybrid_mastery", make_values(ROOT, "TESTS"))
        text = (ROOT / "tests/test_hybrid_mastery.c").read_text()
        for symbol in ("do_discipline", "do_gifts", "do_advance", "do_spheres",
                       "do_arete", "do_principles", "do_monklearn", "do_increase",
                       "do_colors", "do_element", "do_prayer", "do_dragonskill",
                       "hybrid_build_count()==25", "LONG_MIN", "LONG_MAX",
                       "memcmp", "number_range", "player_load_failed"):
            self.assertIn(symbol, text)
        self.assertNotRegex(text, r"\b(?:void|long|int|bool)\s+(?:do_\w+|get_\w+)\s*\(")

    def test_read_only_not_effective_ranks(self):
        text = (SRC / "classes/hybrid/mastery.c").read_text()
        self.assertNotRegex(text, r"\b(?:get_\w+|raw_stat|char_stat|do_\w+|"
                            r"hybrid_player_(?:attach|commit)|hybrid_training_commit|"
                            r"fopen|fwrite|rename|SET_BIT|REMOVE_BIT|WAIT_STATE|"
                            r"number_range|number_percent|alloc_perm|str_dup)\s*\(")
        self.assertNotRegex(text, r"\b(?:ch|p|pc)->[\w\[\].> -]+(?:\+\+|--|[+\-]=)")
        for fragment in ("HYBRID_DEFAULT_QUEST_COST", "player_load_failed", "hybrid_present",
                         "!r.numeric.review", "!r.mastery.review", "default:return 0;",
                         "Conversion remains disabled.", "count>=HYBRID_MASTERY_ITEMS"):
            self.assertIn(fragment, text)

    def test_all_native_trainers_and_cap_policies_unchanged(self):
        hashes = {
            "clan.c": "bf76a113a1025f153744556f9f6b0276d64c4a6b",
            "classes/dragon/dragon.c": "d49760816c58db4303b9bf7200e2b52fa4b87016",
            "classes/drow/drow.c": "7e844520f4580f784b9d81538e8f4eae5c314853",
            "classes/elemental/elemental.c": "90fa65b2627a1f25c9ef29de916427dfb33aa074",
            "classes/highlander/highland.c": "4556c381e7ae9dfcdba82b0526237b43d86c25fa",
            "classes/mage/mage.c": "1c21a771559e734d81496509ba4f3aacd8722850",
            "classes/monk/monk.c": "051fe16da87e9b7647c819512e833893c22762b0",
            "classes/ninja/ninja.c": "34629867a429749bdf5b90ee7388415115833cea",
            "classes/sorcerer/sorcerer.c": "e44a6b703dbf47456776c2207ac4ee70ff7e8956",
            "classes/werewolf/werewolf.c": "7dc28aead30f5407cc3ec2f3e2c657ed0fe0c788",
            "classes/hybrid/specialization.c": "0a657acaaa18ded6364851fcd7bdf51655f8b839",
        }
        for path, expected in hashes.items():
            data = (SRC / path).read_bytes()
            from werewolf_source import without_werewolf_stage
            data = without_werewolf_stage(data.decode()).encode()
            if path == "clan.c":
                from scry_source import without_scry_stage
                data = without_scry_stage(data.decode()).encode()
            self.assertEqual(hashlib.sha1(b"blob " + str(len(data)).encode() +
                                         b"\0" + data).hexdigest(), expected, path)

    def test_learnable_unlock_inventory_matches_native_commands(self):
        drow = (SRC / "classes/drow/drow.c").read_text()
        native = drow[drow.index("void do_prayer("):drow.index("void do_prayer(")+6000]
        grants = re.findall(r"\{grant = (DROW_\w+); cost = \d+;\}", native)
        self.assertEqual(len(set(grants)), 17)
        self.assertNotIn("DROW_GLOBE", grants)
        dragon = (SRC / "classes/dragon/dragon.c").read_text()
        skills = re.findall(r"skill = (DRAG_\w+);", dragon)
        self.assertEqual(len(set(skills)), 13)
        self.assertNotIn("DRAG_WINGSOUT", skills)

    def test_class_menu_not_a_converter(self):
        source = (SRC / "classes/hybrid/selfbrid.c").read_text()
        self.assertEqual(source.count("hybrid_mastery_show(ch)"), 1)
        self.assertLess(source.index("if (!hybrid || !hybrid_has_parent(hybrid, ch->class))"),
                        source.index("hybrid_mastery_show(ch)"))
        self.assertNotRegex(source, r"\b(?:hybrid_player_commit|hybrid_player_attach|"
                            r"do_autoclass|hybrid_secondary_initialize)\s*\(")
        self.assertIn("conversion is not enabled", source)
        before = source.replace('#include "mastery.h"\n', "", 1).replace(
            "    hybrid_mastery_show(ch);\n", "", 1)
        data = before.encode()
        self.assertEqual(hashlib.sha1(b"blob " + str(len(data)).encode() +
                                     b"\0" + data).hexdigest(),
                         "682477bd07a8b05ee45e176350a5a632c956a3af")
        # The old numeric report changes only its now-stale status sentence.
        before = (SRC / "classes/hybrid/readiness.c").read_text().replace(
            "Class/build power mastery is reported separately; safe conversion is still pending.",
            "Class/build power mastery and safe conversion are still pending.", 1)
        data = before.encode()
        self.assertEqual(hashlib.sha1(b"blob " + str(len(data)).encode() +
                                     b"\0" + data).hexdigest(),
                         "0737842987439b3f02f0ea24c1f32f004e63cae5")

    def test_scope_and_live_save_versions_documented(self):
        text = " ".join((ROOT / "docs/HYBRIDS.md").read_text().split())
        for fragment in ("Original-class mastery preview", "staff-only Ancient Wyrm",
                         "one full branch", "not conversion authorization",
                         "Record4/save epoch3/handoff2", "unavailable disciplines",
                         "50,000 QP", "Original-parent"):
            self.assertTrue(fragment in text, fragment)


if __name__ == "__main__":
    unittest.main()
