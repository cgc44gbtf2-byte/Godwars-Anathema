"""Keep the new passive narrowly scoped to existing avoidance penalties."""
import unittest
from test_class_layout import ROOT,SRC,make_values
from drow_blindfight_source import without_drow_blindfight_stage,OLD
import hashlib

class BlindFightingCatalog(unittest.TestCase):
    def test_exact_adapter_and_negative_edits(self):
        source=(SRC/"combat_defense.c").read_text()
        restored=without_drow_blindfight_stage(source)
        data=restored.encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "c2cfcede0de3729fd2b4b52beda34545bed26cba")
        body=restored.split("bool check_avoid(",1)[1].split("long avoidblow(",1)[0]
        self.assertEqual(hashlib.sha256(("bool check_avoid("+body).encode()).hexdigest(),OLD)
        self.assertEqual(source.count("hybrid_power_drow("),2)
        for bad in (source.replace("DROW_BFIGHT","DROW_PIMM"),
                    source.replace("av *= .75","av *= .50"),
                    source.replace("br *= .75","br *= .50")):
            with self.assertRaisesRegex(AssertionError,"Unreviewed"):
                without_drow_blindfight_stage(bad)

    def test_native_cost_and_checked_trainer(self):
        self.assertIn('{"blindfight",DROW_BFIGHT,5000}',
                      (SRC/"classes/hybrid/drow_training.c").read_text())
        self.assertIn("{DROW_BFIGHT,5000,0}",(SRC/"classes/hybrid/training.c").read_text())
        self.assertIn('arg2, "blindfight")) {grant = DROW_BFIGHT; cost = 5000;}',
                      (SRC/"classes/drow/drow.c").read_text())
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        self.assertIn("It does not grant sight",docs)
        self.assertIn("5,000 secondary DP",docs)

    def test_real_engine_routes_and_durable_lessons(self):
        self.assertIn("tests/test_hybrid_drow_blindfight",make_values(ROOT,"TESTS"))
        test=(ROOT/"tests/test_hybrid_drow_blindfight.c").read_text()
        for token in ("count==18432","count==6912","check_avoid(&a,&b,weap)",
                      "can_see(&a,&b)","--native-dump",
                      "a.beginners_luck==1","calculation_original_main(1,argv)"):
            self.assertIn(token,test)
        lessons=(ROOT/"tests/test_hybrid_drow_training.c").read_text()
        self.assertIn('utility_save_faults("drow blindfight learn",DROW_BFIGHT,5000)',lessons)
        self.assertIn("static void blindfight_lessons",lessons)
        self.assertIn("check_avoid(ch,loaded,TRUE)",lessons)

if __name__=="__main__":unittest.main()
