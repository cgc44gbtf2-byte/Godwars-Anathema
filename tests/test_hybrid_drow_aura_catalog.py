"""Strict source scope and mandatory real-engine tests for Drow Read Aura."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from drow_aura_source import without_drow_aura_stage

class DrowAuraCatalog(unittest.TestCase):
    def test_previous_whole_shared_module_restores(self):
        raw=(SRC/"clan.c").read_text()
        restored=without_drow_aura_stage(raw).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(restored)).encode()+b"\0"+restored).hexdigest(),
                         "9ab3c1a67fce0c7263ef2d0461521db7ab3f7328")
        for old,new in (("const bool free_aura = monk_ready || drow_ready;",
                         "const bool free_aura = drow_ready;"),
                        ("ch->mana -= 500","ch->mana -= 1"),
                        ("DROW_READ);","DROW_EYES);")):
            with self.assertRaisesRegex(AssertionError,"Unreviewed"):
                without_drow_aura_stage(raw.replace(old,new))

    def test_independent_source_only(self):
        raw=(SRC/"clan.c").read_text()
        body=raw.split("void do_readaura(",1)[1].split("\nvoid do_shield(",1)[0]
        self.assertIn("hybrid_power_drow(ch, DROW_READ)",body)
        self.assertIn("const bool free_aura = monk_ready || drow_ready",body)
        self.assertEqual(body.count("!free_aura"),11)
        self.assertEqual(body.count("spell_identify("),1)
        self.assertNotIn("save_char",body)
        self.assertNotIn("SET_BIT(",body)

    def test_native_price_and_checked_lesson(self):
        ui=(SRC/"classes/hybrid/drow_training.c").read_text()
        self.assertIn('{"readaura",DROW_READ,6000}',ui)
        self.assertIn("hybrid_training_commit(ch,&request)",ui)
        self.assertIn("{DROW_READ,6000,0}",(SRC/"classes/hybrid/training.c").read_text())
        self.assertIn('arg2, "readaura")) {grant = DROW_READ; cost = 6000;}',
                      (SRC/"classes/drow/drow.c").read_text())

    def test_tests_are_required_and_docs_honest(self):
        self.assertIn("tests/test_hybrid_drow_aura",make_values(ROOT,"TESTS"))
        make=(ROOT/"Makefile").read_text()
        self.assertIn("python3 tests/test_hybrid_drow_aura_catalog.py",make)
        tests=(ROOT/"tests/test_hybrid_drow_aura.c").read_text()
        for fragment in ("pairs==3456","cases==216","LONG_MIN",
                         '#include "test_hybrid_aura.c"'):
            self.assertIn(fragment,tests)
        training=(ROOT/"tests/test_hybrid_drow_training.c").read_text()
        self.assertIn('utility_save_faults("drow readaura learn",DROW_READ,6000)',training)
        doc=(ROOT/"docs/HYBRIDS.md").read_text()
        self.assertIn("6,000 secondary DP",doc)
        self.assertIn("playable conversion\nand live hybrid forms remain disabled",doc)

if __name__=="__main__":unittest.main()
