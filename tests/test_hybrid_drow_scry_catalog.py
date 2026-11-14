"""Bounded independent Spider Eyes Scry and existing native-priced lessons."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from drow_scry_source import without_drow_scry_stage

class DrowScryCatalog(unittest.TestCase):
    def test_exact_previous_module_and_negative_mutations(self):
        raw=(SRC/"clan.c").read_text()
        restored=without_drow_scry_stage(raw).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(restored)).encode()+b"\0"+restored).hexdigest(),
                         "3726e34cc734b282637601f7701bfe4efd76307a")
        for old,new in (("free_scry = monk_ready || drow_ready","free_scry = drow_ready"),
                        ("hybrid_power_drow(ch, DROW_EYES)","hybrid_power_drow(ch, DROW_READ)"),
                        ("if (IS_MAGE(ch) && !free_scry)","if (IS_MAGE(ch))")):
            with self.assertRaisesRegex(AssertionError,"Unreviewed"):
                without_drow_scry_stage(raw.replace(old,new,1))

    def test_grant_only_not_native_policy_or_state(self):
        body=(SRC/"clan.c").read_text().split("void do_scry(",1)[1].split("\nvoid do_readaura(",1)[0]
        self.assertEqual(body.count("!free_scry"),3)
        self.assertIn("hybrid_power_drow(ch, DROW_EYES)",body)
        for token in ("MORE_SHIELDED","ITEMA_VISION","guardian_message( victim )",
                      "get_breed(ch,BREED_HOMID) < 5","ch->in_room = chroom",
                      "ch->mana -= 500","get_disc(ch, DISC_AUSPEX) >= 6"):
            self.assertIn(token,body)
        for token in ("save_char","hybrid_training_commit","WAIT_STATE"):
            self.assertNotIn(token,body)

    def test_native_price_transaction_and_help(self):
        ui=(SRC/"classes/hybrid/drow_training.c").read_text()
        self.assertIn('{"spidereyes",DROW_EYES,7500}',ui)
        self.assertIn("hybrid_training_commit(ch,&request)",ui)
        self.assertIn("{DROW_EYES,7500,0}",(SRC/"classes/hybrid/training.c").read_text())
        self.assertIn('arg2, "spidereyes")) {grant = DROW_EYES; cost = 7500;}',
                      (SRC/"classes/drow/drow.c").read_text())
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        for token in ("7,500 secondary DP","not an item-vision grant or shield bypass",
                      "avoidance passive remains unchanged","native prayer price"):
            self.assertIn(token,docs)

    def test_engine_replays_and_lesson_failure_checks_mandatory(self):
        self.assertIn("tests/test_hybrid_drow_scry",make_values(ROOT,"TESTS"))
        self.assertIn("python3 tests/test_hybrid_drow_scry_catalog.py",(ROOT/"Makefile").read_text())
        fixture=(ROOT/"tests/test_hybrid_drow_scry.c").read_text()
        for token in ("count==5184","count==216","LONG_MIN","ds_sources_and_loss",
                      "scry_run(actors","scry_safety(i)"):
            self.assertIn(token,fixture)
        training=(ROOT/"tests/test_hybrid_drow_training.c").read_text()
        self.assertIn('utility_save_faults("drow spidereyes learn",DROW_EYES,7500)',training)
        self.assertIn("purchases==63",training)
        self.assertEqual(training.count("loaded_spidereyes(reload.character)"),2)

if __name__=="__main__":unittest.main()
