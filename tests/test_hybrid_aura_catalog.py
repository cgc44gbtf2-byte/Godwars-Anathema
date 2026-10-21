"""A scoped Read Aura adapter; no global class identity/getter changes."""
import hashlib
import json
import unittest
from test_class_layout import ROOT,SRC,make_values
from aura_source import without_aura_stage
from item_info_source import without_item_info_stage

def blob(text):
    from file_numbers_source import without_file_number_save
    text=without_file_number_save(text)
    if "void obj_score(" in text:text=without_item_info_stage(text)
    b=text.encode()
    return hashlib.sha1(b"blob "+str(len(b)).encode()+b"\0"+b).hexdigest()

class AuraCatalog(unittest.TestCase):
    def test_entire_shared_module_restores_exact_previous_source(self):
        from scry_source import without_scry_stage
        text=without_scry_stage((SRC/"clan.c").read_text())
        self.assertEqual(blob(without_aura_stage(text)),"a5570740eb895b02ff7aa058ed631e153ecbeef3")
        self.assertEqual(text.count("hybrid_power_rank("),1)

    def test_independent_learned_source_and_native_fallbacks(self):
        body=(SRC/"clan.c").read_text().split("void do_readaura(",1)[1].split("\nvoid do_shield(",1)[0]
        for part in ("HYBRID_TRAIN_MONK, MONK_MANTRA) >= 2",
                     "if (IS_MAGE(ch) && !monk_ready)","ch->mana -= 500",
                     "|| (IS_WEREWOLF(ch) && !monk_ready)",
                     "get_breed(ch, BREED_HOMID) >= 5 && !monk_ready",
                     'spell_identify( skill_lookup( "identify" )',
                     "EXTRA_FAKE_CON","MORE_GUARDIAN"):
            self.assertIn(part,body)
        for cls in ("NINJA","VAMPIRE","WEREWOLF","DROW","DRAGON","MONK","MAGE"):
            self.assertIn("!monk_ready && IS_"+cls+"(ch)",body)
        self.assertNotRegex(body,r"\b(?:SET_BIT|REMOVE_BIT|hybrid_player_attach|save_char_obj)\s*\(")

    def test_safety_guards_and_bounded_npc_rendering(self):
        text=(SRC/"clan.c").read_text().split("void do_readaura(",1)[1].split("\nvoid do_shield(",1)[0]
        self.assertIn("ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || ch->in_room == NULL",text)
        self.assertLess(text.index("ch == NULL"),text.index("one_argument("))
        for part in ('argument != NULL ? argument : ""',"npc_label == NULL",
                     'act("$n examines someone intently."',"snprintf(buf, sizeof(buf)",
                     'sizeof(" is an NPC.\\n\\r")','npc_label != NULL ? npc_label : "Someone"'):
            self.assertIn(part,text)
        self.assertNotIn('sprintf(buf, "%s is an NPC.',text)

    def test_native_macros_getters_interpreter_codecs_and_training_unchanged(self):
        for path,sha in (("merc.h","b843f157662ab181daabe913d74685f1c8bc75e6"),
                         ("interp.c","ccb0125d4264a8a5447618c29a9240ec674d2eb4"),
                         ("act_info.c","d4b24a952210f67fbfadabb480e9908422c72280"),
                         ("save.c","3655e4f99b37b0d5165e58dc73d78c00b47564fc"),
                         ("classes/monk/monk.c","051fe16da87e9b7647c819512e833893c22762b0"),
                         ("classes/hybrid/training.c","4646c2494470f6427efbba6da69752b32590c2a2")):
            self.assertEqual(blob((SRC/path).read_text()),sha)
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

    def test_real_dispatch_independent_projection_native_matrix_and_loaders(self):
        self.assertIn("tests/test_hybrid_aura",make_values(ROOT,"TESTS"))
        test=(ROOT/"tests/test_hybrid_aura.c").read_text()
        for part in ("interpret(ch,command)","count==960","pairs==15552","count==72",
                     "bad<5","hybrid_id=0","LONG_MAX","LONG_MIN","load_char_short",
                     "save_char_obj_checked","memcmp(&before,ch->pcdata",
                     "order||focus==MONK_MANTRA?8:4","native==3"):
            self.assertIn(part,test)
        self.assertNotRegex(test,r"\b(?:hybrid_power_rank|do_readaura|char_hitroll)\s*\([^;]*\)\s*\{")

    def test_usage_limits_and_partial_scope_are_documented(self):
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("Secondary Monk Read Aura","readaura <target>","Mantra2",
                     "960 native","15,552 paired","72 full/short","free Monk source",
                     "Shield","Scry","other shared message-formatting"):
            self.assertIn(part,docs)
        self.assertIn("readaura (Mantra2; free)",(SRC/"classes/hybrid/help.c").read_text())

if __name__=="__main__":
    unittest.main()
