"""Exact Shield-only source scope and real-engine regression linkage."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from shield_source import without_shield_stage

class ShieldCatalog(unittest.TestCase):
    def body(self):
        return (SRC/"clan.c").read_text().split("void do_shield(",1)[1].split(
            "\nvoid do_serpent(",1)[0]

    def test_entire_shared_module_restores_exact_parent(self):
        text=without_shield_stage((SRC/"clan.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(text)).encode()+b"\0"+text).hexdigest(),
                         "ab3dd3a935c10385065bc219707b75610c007fc0")
        with self.assertRaisesRegex(AssertionError,"Unreviewed"):
            without_shield_stage((SRC/"clan.c").read_text().replace(
                'send_to_char("You shield your aura','send_to_char("Changed shield',1))

    def test_missing_context_and_unused_arguments(self):
        body=self.body()
        self.assertIn("ch == NULL || IS_NPC(ch) || ch->pcdata == NULL",body)
        self.assertNotIn("one_argument(",body)
        self.assertNotIn("in_room",body)
        self.assertLess(body.index("ch == NULL"),body.index("IS_MORE("))

    def test_dismissal_precedes_all_activation_gates(self):
        body=self.body()
        self.assertLess(body.index("REMOVE_BIT("),body.index("hybrid_power_rank("))
        self.assertEqual(body.count("SET_BIT("),1)
        self.assertEqual(body.count("REMOVE_BIT("),1)
        for forbidden in ("ch->mana","ch->primal","quest","WAIT_STATE","number_range",
                          "ch->class =","pcdata->monk"):
            self.assertNotIn(forbidden,body)

    def test_single_owned_monk_grant_and_independent_native_grants(self):
        body=self.body()
        self.assertEqual(body.count("hybrid_power_rank("),1)
        self.assertIn("HYBRID_TRAIN_MONK, MONK_MANTRA) >= 2",body)
        for cls in ("VAMPIRE","DRAGON","WEREWOLF","MONK","MAGE"):
            self.assertIn("else if (!monk_ready && IS_"+cls+"(ch)",body)
        for requirement in ("DISC_OBFUSCATE) < 6","DRAG_SHIELD",
                            "TRIBE_SILVER_FANGS) < 4","MONK_MANTRA) < 2","MMIN] < 3"):
            self.assertIn(requirement,body)

    def test_real_engine_oracles_pairs_rejections_and_loaders(self):
        make=(ROOT/"Makefile").read_text()
        self.assertIn("tests/test_hybrid_shield",make_values(ROOT,"TESTS"))
        rule=make.split("tests/test_hybrid_shield: ",1)[1].split("\n\n",1)[0]
        self.assertIn("$(ENGINE_OBJECTS)",rule)
        self.assertIn("tests/test_hybrid_aura.c",rule)
        fixture=(ROOT/"tests/test_hybrid_shield.c").read_text()
        for fragment in ("interpret(ch,input)","0xbe4caaeb550f75e7","count==11664",
                         "player_load_failed=1","LONG_MAX","hybrid_present=2",
                         'load_char_short(&d,"Shieldfixture")',"save_char_obj_checked(ch)",
                         "count==144","Native Read Aura is not blocked"):
            self.assertIn(fragment,fixture)

    def test_help_is_honest_about_one_shared_effect_and_pending_conversion(self):
        helptext=(SRC/"classes/hybrid/help.c").read_text()
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        self.assertIn("Mantra2 also enables shield/mindblock",helptext)
        for fragment in ("one shared","No automatic eligibility","does **not**",
                         "playable conversion remain separate","ordinary timing"):
            self.assertIn(fragment,docs)

if __name__=="__main__":
    unittest.main()
