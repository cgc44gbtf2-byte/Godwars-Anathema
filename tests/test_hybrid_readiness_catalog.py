"""Read-only prerequisite wiring, not conversion or native combat changes."""
import hashlib
import re
import unittest
from test_class_layout import ROOT, SRC, make_values
from test_hybrid_vision_catalog import without_vision_info_adapter


class HybridReadinessCatalog(unittest.TestCase):
    def test_native_style_stance_unchanged(self):
        for name, expected in {
            "style.c": "28df6f81834a829e35677728c66d274ed6897fb0",
            "stance.c": "ef4bba8581d1184d3e6a858448e0d6aa6ac46e42",
        }.items():
            data = (SRC / name).read_bytes()
            self.assertEqual(hashlib.sha1(b"blob " + str(len(data)).encode() +
                                         b"\0" + data).hexdigest(), expected)

    def test_no_mutating_effective_getters_or_transaction(self):
        text = (SRC / "classes/hybrid/readiness.c").read_text()
        self.assertNotRegex(text, r"\b(?:get_style|raw_stat|char_stat|improve_\w+|"
                            r"hybrid_player_commit|hybrid_training_commit|"
                            r"hybrid_player_attach|do_\w+|fopen|fwrite|rename)\s*\(")
        self.assertNotRegex(text, r"\bch->\w+(?:\[[^]]+\])?\s*(?:=(?!=)|\+=|-=|\+\+|--)")
        self.assertNotRegex(text, r"\b(?:SET_BIT|REMOVE_BIT|WAIT_STATE|number_range)\s*\(")
        self.assertIn("stat_max((CHAR_DATA *)ch, i)", text)
        self.assertIn("memset(report, 0", text)
        self.assertIn("_Static_assert", text)

    def test_preview_stays_disabled(self):
        text = (SRC / "classes/hybrid/selfbrid.c").read_text()
        self.assertEqual(text.count("hybrid_readiness_show(ch)"), 1)
        self.assertIn("conversion is not enabled", text)
        self.assertNotRegex(text, r"\b(?:hybrid_player_attach|hybrid_player_commit|"
                            r"hybrid_training_commit|do_autoclass)\s*\(")
        self.assertNotRegex(text, r"ch->(?:class|pcdata->quest)\s*(?:=|-=|\+=)")

    def test_actual_engine_test_and_caps(self):
        text = (ROOT / "tests/test_hybrid_readiness.c").read_text()
        for fragment in ("improve_style(&actor)", "improve_weapon(&actor, i)",
                         "improve_stance(&actor)", "stat_max(&actor, i)",
                         "LONG_MIN", "LONG_MAX", "do_selfbrid(&actor",
                         "memcmp", "number_range", "immortal < 2"):
            self.assertIn(fragment, text)
        self.assertNotRegex(text, r"\b(?:long|int|bool|void)\s+(?:stat_max|"
                            r"improve_style|improve_weapon|improve_stance)\s*\(")
        sources = make_values(ROOT, "ENGINE_SOURCES")
        self.assertIn("Genesis/src/classes/hybrid/readiness.c", sources)
        self.assertIn("tests/test_hybrid_readiness", make_values(ROOT, "TESTS"))
        self.assertIn("tests/test_hybrid_readiness: tests/test_hybrid_readiness.c "
                      "$(ENGINE_OBJECTS)", (ROOT / "Makefile").read_text())

    def test_partial_status_documented(self):
        doc = (ROOT / "docs/HYBRIDS.md").read_text()
        for fragment in ("not** eligibility", "above", "staff adjustment",
                         "claws happen", "Record4/save epoch3/handoff2"):
            self.assertIn(fragment, doc)
        self.assertIn("nothing was normalized",
                      (SRC / "classes/hybrid/readiness.c").read_text())

    def test_parent_filter_precedes_output(self):
        text = (SRC / "classes/hybrid/selfbrid.c").read_text()
        self.assertIn("if (!hybrid_has_parent(hybrid, ch->class)) continue;", text)
        guard = "if (!hybrid || !hybrid_has_parent(hybrid, ch->class))"
        self.assertIn(guard, text)
        self.assertLess(text.index(guard), text.index('"[%ld] %s:'))
        tests = (ROOT / "tests/test_hybrids.c").read_text()
        for fragment in ("choices == 9", "No hybrid choices", "!strstr(output, row)",
                         "!strstr(output, h->name)", "!strstr(output, \"Planned conversion price\")"):
            self.assertIn(fragment, tests)


    def test_palette_is_help_class(self):
        data = (ROOT / "Genesis/area/class.hlp").read_text()
        body = re.search(r"0 Classes~(.*?)\n~", data, re.S).group(1)
        colors = dict(re.findall(r'case (CLASS_\w+): return "(#[^"]+)";',
                                 (SRC / "classes/hybrid/registry.c").read_text()))
        self.assertEqual(colors["CLASS_DRAGON"], "#n")
        self.assertRegex(body, r"(?m)^Dragon\s+-")
        for key, name in {"DROW": "Drow", "ELEM": "Elementalist",
                          "HIGHLANDER": "Highlander", "MAGE": "Mage",
                          "MONK": "Monk", "NINJA": "Ninja", "SORCERER": "Sorcerer",
                          "VAMPIRE": "Vampire", "WEREWOLF": "Werewolf"}.items():
            self.assertIn(colors["CLASS_" + key] + name + "#n", body)
        registry = (SRC / "classes/hybrid/registry.c").read_text()
        self.assertNotRegex(registry, r"\b(?:number_range|number_percent|rand|srand)\s*\(")
        self.assertIn("hybrid_display_name", registry)

    def test_each_hybrid_has_its_own_visible_fixed_pattern(self):
        registry=(SRC/"classes/hybrid/registry.c").read_text()
        motifs=re.findall(r'^\s*"([01]{8})", /\* (\d+): ([^*]+) \*/',registry,re.M)
        self.assertEqual(len(motifs),45)
        self.assertEqual([int(i) for _,i,_ in motifs],list(range(1,46)))
        visible=["".join(pattern[i%8] for i in range(sum(c.isalpha() for c in name)))
                 for pattern,_,name in motifs]
        self.assertEqual(len(set(visible)),45)
        for pattern in visible:self.assertTrue(pattern.startswith("0") and "1" in pattern)
        self.assertNotIn("uint32_t hash",registry)
        for path in ("selfbrid.c","help.c"):
            self.assertIn("hybrid_display_name_for_parent", (SRC/"classes/hybrid"/path).read_text())
        test=(ROOT/"tests/test_hybrids.c").read_text()
        for token in ("color_signature","strcmp(signatures[i],signatures[j])",
                      "strcmp(signatures[i],reverse)"):
            self.assertIn(token,test)

    def test_help_does_not_rewrite_world_or_existing_topics(self):
        text = (SRC / "classes/hybrid/help.c").read_text()
        self.assertNotRegex(text, r"\b(?:fopen|fwrite|rename|free_string|str_dup|"
                            r"alloc_perm|number_range|WAIT_STATE|SET_BIT)\s*\(")
        self.assertNotRegex(text, r"(?:pHelp->|help_first\s*=|help_last\s*=)")
        native = without_vision_info_adapter((SRC / "act_info.c").read_text())
        native = native.replace('#include "classes/hybrid/hybrid.h"\n', "")
        native = native.replace("hybrid_help_parent_links(ch, pHelp->keyword);\n", "")
        native = native.replace("    if (hybrid_help_topic(ch, argall)) return;\n", "")
        data = native.encode()
        self.assertEqual(hashlib.sha1(b"blob " + str(len(data)).encode() +
                                     b"\0" + data).hexdigest(),
                         "88a763c1963755f79dfb618f347d275e0f5a40f3")
        self.assertIn("Genesis/src/classes/hybrid/help.c", make_values(ROOT, "ENGINE_SOURCES"))

    def test_book_policy_does_not_enable_readonly_conversion(self):
        text = (SRC / "classes/hybrid/help.c").read_text()
        self.assertIn("native command gates remain. Conversion is disabled.", text)
        tests = (ROOT / "tests/test_hybrids.c").read_text()
        for fragment in ("cap == 6", "cap == 4", "cap == 3", "rows == 90",
                         "CUSTOM HYBRID HELP TAKES PRIORITY", "cosmetics();", "help_topics();"):
            self.assertIn(fragment, tests)


if __name__ == "__main__":
    unittest.main()
