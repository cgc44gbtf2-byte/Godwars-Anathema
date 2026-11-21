"""Globe lifecycle scope, native-source preservation and negative mutations."""
import hashlib
import unittest
from test_class_layout import ROOT, SRC, make_values
from drow_globe_source import without_drow_globe_stage
from gargoyle_death_source import without_gargoyle_death

BASE = {
    "classes/drow/drow.c": "27c68cdbad00a1fc523212c5d8d94f3bdd9e3c45",
    "clan.c": "2f1e0ea9492bfda2f157849e2303313450d98e66",
    "handler.c": "a9387450455381bb240fc00f7961d0074d733f5a",
    "act_info.c": "9a67bebbe7eb3bf4bfee487c66b2cda6f4a8f6d2",
    "update.c": "32fccb75067cd3d70d66dfa818b2e7d29d8d1ca3",
    "classes/werewolf/wwauspice.c": "2e51472a39c9d8e7baea7b7852d0777606d10b9e",
    "combat_death.c": "1d31296b9825f496438777a02a0769f67ec533e2",
}

def blob(text):
    data = text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()

class DarknessCatalog(unittest.TestCase):
    def test_only_reviewed_engine_edits(self):
        for path, expected in BASE.items():
            with self.subTest(path=path):
                self.assertEqual(blob(without_drow_globe_stage(
                    without_gargoyle_death((SRC/path).read_text()))), expected)

    def test_inverse_does_not_hide_cost_or_target_mutations(self):
        for path, old, new in (
            ("classes/drow/drow.c", "hybrid_drow_points_charge(ch,500)",
             "hybrid_drow_points_charge(ch,1)"),
            ("update.c", "hybrid_drow_globe_upkeep(ch,150)",
             "hybrid_drow_globe_upkeep(ch,1)"),
            ("act_info.c", "hybrid_drow_globe_active(vch)",
             "hybrid_drow_globe_active(ch)"),
            ("combat_death.c", "hybrid_drow_globe_set(victim,0)",
             "hybrid_drow_globe_set(victim,1)"),
        ):
            text=(SRC/path).read_text()
            self.assertIn(old,text)
            self.assertNotEqual(blob(without_drow_globe_stage(
                text.replace(old,new,1))), BASE[path])

    def test_lifecycle_module_and_real_interpreter_tests_are_linked(self):
        self.assertIn("Genesis/src/classes/hybrid/drow_globe.c",
                      make_values(ROOT, "ENGINE_SOURCES"))
        fixture=(ROOT/"tests/test_hybrid_drow_training.c").read_text()
        for token in ('interpret(ch,"globedarkness")', "darkness_lessons();",
                      "hybrid_drow_globe_upkeep(ch,150)==0",
                      'do_look(&watcher,"auto")', "update_pos(ch)",
                      "load_char_short", "load_char_obj",
                      'utility_save_faults("drow darkness learn",DROW_DARK,9500)'):
            self.assertIn(token, fixture)

if __name__=="__main__":
    unittest.main()
