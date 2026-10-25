"""Expose only the reviewed, already-existing secondary soak passive."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values

class ToughnessCatalog(unittest.TestCase):
    def test_no_combat_formula_changes(self):
        # Whole runtime files, not weakened or normalized source pins.
        for path,digest in RUNTIME_SHA.items():
            data=(SRC/path).read_bytes()
            actual=hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()
            self.assertEqual(actual,digest,path)

    def test_native_price_and_checked_lesson(self):
        self.assertIn('{"toughness",DROW_TOUGH,8000}',
                      (SRC/"classes/hybrid/drow_training.c").read_text())
        self.assertIn("{DROW_TOUGH,8000,0}",(SRC/"classes/hybrid/training.c").read_text())
        self.assertIn('arg2, "toughness")) {grant = DROW_TOUGH; cost = 8000;}',
                      (SRC/"classes/drow/drow.c").read_text())
        doc=(ROOT/"docs/HYBRIDS.md").read_text()
        self.assertIn("8,000 secondary DP",doc)
        self.assertIn("300 soak once",doc)
        self.assertIn("Brittle Bones halves",doc)

    def test_real_engine_coverage_and_save_faults(self):
        self.assertIn("tests/test_hybrid_drow_toughness",make_values(ROOT,"TESTS"))
        test=(ROOT/"tests/test_hybrid_drow_toughness.c").read_text()
        for token in ("pairs==3456","fallbacks==5184","get_soak(&a)",
                      "brittle.modifier==99","--native-dump",
                      "assert(number_mm()==expected)","calculation_original_main(1,argv)"):
            self.assertIn(token,test)
        lesson=(ROOT/"tests/test_hybrid_drow_training.c").read_text()
        self.assertIn('utility_save_faults("drow toughness learn",DROW_TOUGH,8000)',lesson)
        self.assertIn("static void toughness_lessons",lesson)
        self.assertIn("get_soak(loaded)==trained-300",lesson)

RUNTIME_SHA={
    "combat_defense.c":"85870c3e786e30f58a484a6fbf6f8d4b53b2cd78",
    "classes/hybrid/passives.c":"a7796a45e703e5efb1505875884ec29cc512257a",
    "classes/hybrid/access.c":"022f106c05eca29030cb50565f78e95b92766ef5",
}

if __name__=="__main__":unittest.main()
