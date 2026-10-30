"""Acid Bolt keeps native combat and exposes only its exact paid flag."""
import json
import unittest
from test_class_layout import ROOT, SRC, make_values


class HybridAcidBoltCatalog(unittest.TestCase):
    def test_owned_access_and_callback_contract(self):
        body=(SRC/"classes/drow/drow.c").read_text().split(
            "void do_acidbolt(",1)[1].split("void do_faeriefire(",1)[0]
        self.assertIn("hybrid_power_parent(ch,CLASS_DROW)",body)
        self.assertIn("hybrid_power_drow(ch,DROW_ACID)",body)
        self.assertIn("is_safe(ch, victim)",body)
        self.assertIn("ch->pcdata == NULL || ch->in_room == NULL",body)
        self.assertEqual(body.count('one_hit(ch, victim, TYPE_HIT, "acid bolt")'),1)
        self.assertIn("combat_character_in_room(combat_room, ch)",body)
        self.assertIn("WAIT_STATE(ch, 8)",body)
        self.assertNotRegex(body,r"ch->(?:class|mana|primal|exp)\s*[-+*/]?=(?!=)")
        self.assertNotIn("get_drow(",body)

    def test_real_engine_and_native_oracles_are_mandatory(self):
        text=(ROOT/"tests/test_hybrid_acidbolt.c").read_text()
        self.assertIn("__real_one_hit(ch,victim,dt,message)",text)
        self.assertIn("13561486456634868849",text)
        self.assertIn("pairs==864",text)
        self.assertIn("paired();fail_closed();",text)
        self.assertEqual(make_values(ROOT,"TESTS").count("tests/test_hybrid_acidbolt"),1)
        make=(ROOT/"Makefile").read_text()
        self.assertIn("tests/test_hybrid_acidbolt.c $(ENGINE_OBJECTS)",make)
        self.assertIn("-Wl,--wrap=one_hit",make)

    def test_no_implicit_other_power_or_compatibility_change(self):
        ui=(SRC/"classes/hybrid/drow_training.c").read_text()
        self.assertIn('{"acidbolt",DROW_ACID,5000}',ui)
        self.assertIn('{"llothsweb",DROW_WEB,8000}',ui)
        self.assertNotIn("DROW_SPIDER",ui)
        compat=json.loads((ROOT/"deploy/update_compat.json").read_text())
        self.assertEqual(compat["save_epoch"],4)
        self.assertEqual(compat["handoff"],2)
        preview=(SRC/"classes/hybrid/selfbrid.c").read_text()
        self.assertNotIn("hybrid_player_attach(",preview)


if __name__=="__main__":
    unittest.main()
