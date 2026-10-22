"""Source boundaries and live-engine reset coverage, independent of outcomes."""
import hashlib
import json
from pathlib import Path
import unittest
from class_reset_source import without_class_reset_stage

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/"Genesis/src/clearclass.c"
class ResetCatalog(unittest.TestCase):
    def test_whole_previous_module_restores(self):
        data=without_class_reset_stage(SOURCE.read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "6a24244a24bc2595f729bf788a3cdcfc41d05af8")

    def test_neighboring_rule_edit_rejected(self):
        text=SOURCE.read_text()
        altered=text.replace("victim->class                           = 0;",
                             "victim->class                           = CLASS_MAGE;")
        self.assertNotEqual(text,altered)
        with self.assertRaises(AssertionError):without_class_reset_stage(altered)

    def test_preflight_rejects_invalid_state_before_mutating(self):
        text=SOURCE.read_text()
        self.assertLess(text.index("!hybrid_player_save_valid(victim)"),
                        text.index("release_character_links(victim,FALSE)"))
        self.assertIn("if (ch == NULL) return;",text)
        self.assertIn('argument != NULL ? argument : ""',text)
        self.assertNotIn("player_load_failed =",text)

    def test_reset_releases_all_nine_strings_and_inline_bank(self):
        text=SOURCE.read_text()
        for field in ("pcdata->conding","pcdata->love","pcdata->covenant",
                      "pcdata->cov_time","pcdata->leader","pcdata->vleader",
                      "lord","clan","morph"):
            self.assertEqual(text.count("free_string(victim->"+field+");"),1)
        self.assertIn("victim->pcdata->hybrid_present = 0;",text)
        self.assertLess(text.index("memset(&victim->pcdata->hybrid"),
                        text.index('do_save(victim, "")'))
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

    def test_default_engine_suite_preserves_real_free_and_loader(self):
        make=(ROOT/"Makefile").read_text()
        for part in ("TESTS += tests/test_class_reset",
                     "python3 tests/test_class_reset_catalog.py",
                     "-Dfree_string=class_reset_free_string",
                     "tests/reset_clearclass.o"):
            self.assertIn(part,make)
        fixture=(ROOT/"tests/test_class_reset.c").read_text()
        for part in ("free_string(text);","load_char_obj(&shell","save_char_obj_checked(victim)",
                     "hybrid_count()","effects_case()","PLR_AUTOEXIT"):
            self.assertIn(part,fixture)

if __name__=="__main__":unittest.main()
