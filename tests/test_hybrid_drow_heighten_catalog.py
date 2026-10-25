"""Bounded runtime inverse, native price and mandatory real-engine coverage."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from drow_heighten_source import without_drow_heighten_stage

class DrowHeightenCatalog(unittest.TestCase):
    def test_exact_previous_whole_module_and_negative_mutations(self):
        raw=(SRC/"clan.c").read_text()
        restored=without_drow_heighten_stage(raw).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(restored)).encode()+b"\0"+restored).hexdigest(),
                         "b1461d20594c93384bc5e4c482b5bfc46c9545d2")
        for old,new in (("heighten_ready = monk_ready || drow_ready","heighten_ready = drow_ready"),
                        ("DROW_SIGHT);","DROW_READ);"),("WAIT_STATE(ch, 6);","WAIT_STATE(ch, 3);")):
            with self.assertRaisesRegex(AssertionError,"Unreviewed"):
                without_drow_heighten_stage(raw.replace(old,new,1))

    def test_independent_grant_and_unchanged_toggle(self):
        body=(SRC/"clan.c").read_text().split("void do_truesight(",1)[1].split("\nvoid do_soulmask(",1)[0]
        self.assertIn("hybrid_power_drow(ch, DROW_SIGHT)",body)
        self.assertIn("heighten_ready = monk_ready || drow_ready",body)
        self.assertEqual(body.count("!heighten_ready && "),9)
        self.assertLess(body.index("REMOVE_BIT("),body.index("hybrid_power_drow("))
        self.assertEqual(body.count("SET_BIT("),1)
        self.assertEqual(body.count("WAIT_STATE(ch, 6)"),1)
        self.assertNotIn("ch->mana",body)
        self.assertNotIn("save_char",body)

    def test_native_price_existing_transaction_and_compat(self):
        ui=(SRC/"classes/hybrid/drow_training.c").read_text()
        self.assertIn('{"heighten",DROW_SIGHT,20000}',ui)
        self.assertIn("hybrid_training_commit(ch,&request)",ui)
        self.assertIn("{DROW_SIGHT,20000,0}",(SRC/"classes/hybrid/training.c").read_text())
        self.assertIn('arg2, "heighten")) {grant = DROW_SIGHT; cost = 20000;}',
                      (SRC/"classes/drow/drow.c").read_text())
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        for text in ("20,000 secondary DP","one toggle","six-pulse wait",
                     "not automatically revoked","live hybrid forms remain disabled"):
            self.assertIn(text,docs)

    def test_real_engine_checks_are_mandatory(self):
        self.assertIn("tests/test_hybrid_drow_heighten",make_values(ROOT,"TESTS"))
        make=(ROOT/"Makefile").read_text()
        self.assertIn("python3 tests/test_hybrid_drow_heighten_catalog.py",make)
        test=(ROOT/"tests/test_hybrid_drow_heighten.c").read_text()
        for text in ('#include "test_hybrid_heighten.c"',"count==2592","count==324",
                     "LONG_MIN","ht_run(actors","dh_sources_and_loss"):
            self.assertIn(text,test)
        training=(ROOT/"tests/test_hybrid_drow_training.c").read_text()
        self.assertIn('utility_save_faults("drow heighten learn",DROW_SIGHT,20000)',training)
        self.assertIn("purchases==54",training)
        self.assertEqual(training.count("loaded_heighten(reload.character)"),2)

if __name__=="__main__":unittest.main()
