"""Independent whole-source restoration plus cleanup and integration contracts."""
import hashlib
import json
from pathlib import Path
import unittest
from werewolf_reversion_source import without_werewolf_reversion_stage

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src"
def blob(text):
    data=text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()

class ReversionCatalog(unittest.TestCase):
    def test_entire_previous_modules_restore(self):
        for path,expected in (
            ("clan.c","32351c6512fcb5ebb2a5954a521e262d29a67d37"),
            ("clearclass.c","4f89fd197023d27531b8fc0aa18f7e4907771d72"),
        ):
            self.assertEqual(blob(without_werewolf_reversion_stage((SRC/path).read_text())),expected)

    def test_unreviewed_neighbor_edits_rejected(self):
        for path,new,changed in (
            ("clan.c","ch->pcdata->wolf = 0;","ch->pcdata->wolf = 1;"),
            ("clearclass.c","victim->max_hit                     = 10000;",
                            "victim->max_hit                     = 20000;"),
        ):
            text=(SRC/path).read_text()
            self.assertIn(new,text)
            with self.assertRaises(AssertionError):
                without_werewolf_reversion_stage(text.replace(new,changed))

    def test_off_is_not_training_gated_but_class_permission_stays(self):
        body=(SRC/"clan.c").read_text().split("void do_unwerewolf(",1)[1].split("\nvoid do_",1)[0]
        self.assertIn("if (!IS_WEREWOLF(ch)) return;",body)
        self.assertIn("REMOVE_BIT(ch->vampaff, VAM_NIGHTSIGHT)",body)
        self.assertNotIn("do_nightsight(",body)
        self.assertIn("!IS_POLYAFF(ch, POLY_ZULO)",body)
        self.assertIn("update_style(ch)",body)
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

    def test_reset_clears_only_the_owned_act_bit_and_releases_morph(self):
        body=(SRC/"clearclass.c").read_text()
        self.assertEqual(body.count("REMOVE_BIT(victim->act, PLR_WOLFMAN);"),1)
        self.assertNotRegex(body,r"victim->act\s*=")
        self.assertLess(body.index("free_string(victim->morph)"),
                        body.index('victim->morph                           = str_dup( "" )'))

    def test_engine_test_is_in_default_suite(self):
        make=(ROOT/"Makefile").read_text()
        for part in ("TESTS += tests/test_werewolf_reversion",
                     "python3 tests/test_werewolf_reversion_catalog.py",
                     "tests/test_werewolf_reversion.c tests/test_restored_spells.c $(ENGINE_OBJECTS)"):
            self.assertIn(part,make)

if __name__=="__main__":unittest.main()
