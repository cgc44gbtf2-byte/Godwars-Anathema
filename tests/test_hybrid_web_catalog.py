"""Only the reviewed Web access/guards change; native source pins stay strict."""
import hashlib
import json
import unittest
from test_class_layout import ROOT,SRC,make_values
from web_source import without_web_stage


class HybridWebCatalog(unittest.TestCase):
    def body(self):
        return (SRC/"clan.c").read_text().split("void do_web(",1)[1].split(
            "\nvoid do_entrancement(",1)[0]

    def test_entire_previous_shared_module_restores(self):
        text=without_web_stage((SRC/"clan.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(text)).encode()+b"\0"+text).hexdigest(),
                         "cdf5f7fe943a3a89ee0f31eac8f7d172c28ccb40")
        for old,new in (("WAIT_STATE(ch, 12);","WAIT_STATE(ch, 6);"),
                        ("(sn, 50, ch, victim)","(sn, 60, ch, victim)"),
                        ("NINJA_KI] -= 10","NINJA_KI] -= 5")):
            with self.assertRaisesRegex(AssertionError,"Unreviewed"):
                without_web_stage((SRC/"clan.c").read_text().replace(old,new))

    def test_one_real_spell_independent_grants_no_double_charge(self):
        body=self.body()
        self.assertIn("hybrid_power_drow(ch, DROW_WEB)",body)
        self.assertIn("hybrid_power_parent(ch,CLASS_DROW)",body)
        self.assertEqual(body.count("!drow_ready && "),3)
        self.assertEqual(body.count("(*skill_table[sn].spell_fun) (sn, 50, ch, victim)"),1)
        self.assertEqual(body.count("NINJA_KI] -= 10"),1)
        self.assertEqual(body.count("WAIT_STATE(ch, 12)"),1)
        for token in ("ch->class =","ch->mana -=","ch->primal -=","save_char","affect_to_char"):
            self.assertNotIn(token,body)

    def test_context_and_callback_checks_precede_pointer_access(self):
        body=self.body()
        self.assertLess(body.index("ch == NULL"),body.index("one_argument("))
        self.assertLess(body.index("combat_character_in_room(ch->in_room, victim)"),
                        body.index("is_safe(ch, victim)"))
        self.assertIn("skill_table[sn].spell_fun == NULL",body)
        self.assertLess(body.index("combat_character_in_room(combat_room, ch)"),
                        body.index("NINJA_KI] -= 10"))

    def test_training_help_and_compatibility(self):
        ui=(SRC/"classes/hybrid/drow_training.c").read_text()
        self.assertIn('{"llothsweb",DROW_WEB,8000}',ui)
        for token in ("DROW_DARK","DROW_SPIDER","DROW_SKIP"):
            self.assertNotIn(token,ui)
        helptext=(SRC/"classes/hybrid/help.c").read_text()
        self.assertIn("Lloth's Web costs",helptext)
        self.assertIn("including Ninja originals",helptext)
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})
        self.assertNotIn("hybrid_player_attach(",(SRC/"classes/hybrid/selfbrid.c").read_text())

    def test_real_engine_native_pairs_and_loaders_are_required(self):
        self.assertIn("tests/test_hybrid_web",make_values(ROOT,"TESTS"))
        make=(ROOT/"Makefile").read_text()
        self.assertIn("python3 tests/test_hybrid_web_catalog.py",make)
        self.assertIn("-Wl,--wrap=spell_web",make)
        test=(ROOT/"tests/test_hybrid_web.c").read_text()
        for token in ("__real_spell_web(sn,level,ch,vo)","13384253252868165408",
                      "count==4032","count==224","web_pairs();web_refusals();",
                      'interpret(actors,command)'):
            self.assertIn(token,test)
        training=(ROOT/"tests/test_hybrid_drow_training.c").read_text()
        for token in ("web_save_faults();","purchases==36",
                      'load_char_short(&reload,"Drowfixture")'):
            self.assertIn(token,training)


if __name__=="__main__":
    unittest.main()
