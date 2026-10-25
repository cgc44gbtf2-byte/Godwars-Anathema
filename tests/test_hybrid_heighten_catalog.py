"""Reviewed Heighten scope, immutable native inverse and real-engine coverage."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from heighten_source import without_heighten_stage
from drow_heighten_source import without_drow_heighten_stage

class HeightenCatalog(unittest.TestCase):
    def body(self):
        return without_drow_heighten_stage((SRC/"clan.c").read_text()).split("void do_truesight(",1)[1].split(
            "\nvoid do_soulmask(",1)[0]

    def test_shared_module_restores_exact_parent(self):
        source=(SRC/"clan.c").read_text()
        restored=without_heighten_stage(source).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(restored)).encode()+b"\0"+restored).hexdigest(),
                         "a7891874909d7ff1fdeb4b4587bcbc03f058c640")
        with self.assertRaisesRegex(AssertionError,"Unreviewed"):
            without_heighten_stage(source.replace("WAIT_STATE(ch, 6);",
                "WAIT_STATE(ch, 5);",1))

    def test_context_and_cleanup_precede_all_grants(self):
        body=self.body()
        self.assertIn("ch == NULL || IS_NPC(ch) || ch->pcdata == NULL",body)
        self.assertLess(body.index("ch == NULL"),body.index("IS_SET("))
        self.assertLess(body.index("REMOVE_BIT("),body.index("hybrid_power_rank("))
        self.assertNotIn("one_argument(",body)
        self.assertNotIn("in_room",body)

    def test_owned_rank_and_independent_native_grants(self):
        body=self.body()
        self.assertEqual(body.count("hybrid_power_rank("),1)
        self.assertIn("HYBRID_TRAIN_MONK, MONK_MANTRA) >= 1",body)
        for cls in ("DRAGON","DROW","HIGHLANDER","MONK","NINJA","VAMPIRE","WEREWOLF","MAGE"):
            self.assertIn("if (!monk_ready && IS_"+cls+"(ch)",body)
        self.assertEqual(body.count("!monk_ready && "),9)

    def test_native_cost_timing_and_one_shared_toggle(self):
        body=self.body()
        self.assertEqual(body.count("WAIT_STATE(ch, 6)"),1)
        self.assertEqual(body.count("SET_BIT("),1)
        self.assertEqual(body.count("REMOVE_BIT("),1)
        for token in ("ch->mana","ch->primal","quest","number_range","EXTRA_SORC",
                      "pcdata->monk","ch->class ="):
            self.assertNotIn(token,body)

    def test_real_engine_native_pairs_loaders_and_sources(self):
        self.assertIn("tests/test_hybrid_heighten",make_values(ROOT,"TESTS"))
        rule=(ROOT/"Makefile").read_text().split("tests/test_hybrid_heighten: ",1)[1].split("\n\n",1)[0]
        self.assertIn("$(ENGINE_OBJECTS)",rule)
        self.assertIn("tests/test_hybrid_aura.c",rule)
        fixture=(ROOT/"tests/test_hybrid_heighten.c").read_text()
        for token in ("0x4055552fe329d8c3","count==1428","count==15552","count==432",
                      "count==144","interpret(ch,input)","effective_holylight(actors)",
                      "effective_shadowsight(actors)","effective_nightsight(actors)",
                      'load_char_short(&d,"Heightfixture")',"save_char_obj_checked(ch)"):
            self.assertIn(token,fixture)

    def test_help_discloses_native_wait_and_unfinished_conversion(self):
        helptext=(SRC/"classes/hybrid/help.c").read_text()
        self.assertIn("Mantra1 enables heighten",helptext)
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        for token in ("six-pulse wait","manual dismissal after training",
                      "not an automatic source migration","playable conversion"):
            self.assertIn(token,docs)

if __name__=="__main__":
    unittest.main()
