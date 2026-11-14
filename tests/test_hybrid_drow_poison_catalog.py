"""Exact two-predicate adapter, native prices and mandatory real-engine cases."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from drow_poison_source import without_drow_poison_stage

class DrowPoisonCatalog(unittest.TestCase):
    def test_exact_native_body_inverses_and_negative_edits(self):
        for file,token in (("magic.c","else if (hybrid_power_drow(victim, DROW_PIMM)) return;"),
                           ("classes/drow/drow.c","if (hybrid_power_drow(victim, DROW_PIMM))")):
            text=(SRC/file).read_text()
            old=without_drow_poison_stage(text)
            self.assertNotEqual(text,old)
            with self.assertRaisesRegex(AssertionError,"Unreviewed"):
                without_drow_poison_stage(text.replace(token,token.replace("DROW_PIMM","DROW_EYES")))
            with self.assertRaisesRegex(AssertionError,"Unreviewed"):
                without_drow_poison_stage(text.replace(token,token+"\n/* unreviewed */"))

    def test_existing_grants_cost_and_state_unchanged(self):
        magic=(SRC/"magic.c").read_text()
        body=magic.split("void spell_poison(",1)[1].split("void spell_protection(",1)[0]
        self.assertIn("IS_VAMPIRE(victim)",body)
        self.assertIn("get_tribe(victim, TRIBE_BONE_GNAWERS) >= 1",body)
        self.assertIn("saves_spell( level, victim )",body)
        self.assertIn('{"poisonimmunity",DROW_PIMM,5000}',
                      (SRC/"classes/hybrid/drow_training.c").read_text())
        self.assertIn("{DROW_PIMM,5000,0}",(SRC/"classes/hybrid/training.c").read_text())
        self.assertIn('arg2, "poisonimmunity")) {grant = DROW_PIMM; cost = 5000;}',
                      (SRC/"classes/drow/drow.c").read_text())
        for bad in ("affect_strip","REMOVE_BIT","hybrid_training_commit","WAIT_STATE"):
            self.assertNotIn(bad,body)

    def test_mandatory_engine_and_checked_lessons(self):
        self.assertIn("tests/test_hybrid_drow_poison",make_values(ROOT,"TESTS"))
        test=(ROOT/"tests/test_hybrid_drow_poison.c").read_text()
        for token in ("count==288","bad<=6","spell_poison(gsn_poison",
                      'shoot("north target")',"actual.hit==plain.hit",
                      "ammunition_original_main","--native-dump"):
            self.assertIn(token,test)
        training=(ROOT/"tests/test_hybrid_drow_training.c").read_text()
        self.assertIn('utility_save_faults("drow poisonimmunity learn",DROW_PIMM,5000)',training)
        self.assertIn("static void poison_lessons",training)
        self.assertIn("load_char_short",training)
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        for token in ("5,000 secondary DP","does not grant general sleep",
                      "neither cures existing poison"):
            self.assertIn(token,docs)

if __name__=="__main__":unittest.main()
