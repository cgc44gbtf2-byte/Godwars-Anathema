"""One shared native guardian, independent eligibility, no rank addition."""
import hashlib
import json
import unittest
from test_class_layout import ROOT,SRC,make_values
from test_guardian_lifecycle_catalog import without_guardian_lifecycle_save
from aura_source import without_aura_stage
from item_info_source import without_item_info_stage

def once(text,old,new):
    assert text.count(old)==1,repr(old)
    return text.replace(old,new,1)

DISMISS=('    if (IS_MORE(ch, MORE_GUARDIAN))\n    {\n'
         '\tstc("You dismiss your Spirit Guardian.\\n\\r", ch);\n'
         '\tguardian_message(ch);\n'
         '\tstc("I shall return when you call me, Master.\\n\\r", ch);\n'
         '\tREMOVE_BIT(ch->more, MORE_GUARDIAN);\n\treturn;\n    }\n')

def without_guardian_stage(text):
    text=once(text,'#include "classes/hybrid/access.h"\n',"")
    start=text.index("void do_spiritguardian(")
    end=text.index("\nvoid do_succulence(",start)
    body=text[start:end]
    body=once(body,
        '    const bool monk_parent = hybrid_power_parent(ch, CLASS_MONK);\n'
        '    const long mantra = monk_parent\n'
        '        ? hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_MANTRA) : 0;\n'
        '    const bool monk_ready = mantra >= 6;\n'
        '    if (!IS_VAMPIRE(ch) && !IS_WEREWOLF(ch) && !monk_parent)',
        '    if (!IS_VAMPIRE(ch) && !IS_WEREWOLF(ch) && !IS_MONK(ch))')
    body=once(body,'get_disc(ch,DISC_NECROMANCY) < 4 && !monk_ready',
                   'get_disc(ch,DISC_NECROMANCY) < 4')
    body=once(body,'get_tribe(ch,TRIBE_CHILDREN_OF_GAIA) < 4 && !monk_ready',
                   'get_tribe(ch,TRIBE_CHILDREN_OF_GAIA) < 4')
    body=once(body,'!IS_VAMPIRE(ch) && !IS_WEREWOLF(ch) && !monk_ready',
                   'IS_MONK(ch) && get_monk(ch, MONK_MANTRA) < 6')
    body=once(body,'    if (ch == NULL) return;\n'
        '    if (IS_NPC(ch) || ch->pcdata == NULL)\n    {\n'
        '        stc("Huh?\\n\\r",ch);\n        return;\n    }\n'
        '    /* Removing an existing toggle never grants an ability.  Do not strand\n'
        '     * it when class/training is removed or a secondary record is rejected. */\n',"")
    body=once(body,DISMISS+"\n","")
    body=once(body,'    stc("You call your Spirit Guardian.\\n\\r", ch);',
                   DISMISS+'    stc("You call your Spirit Guardian.\\n\\r", ch);')
    return text[:start]+body+text[end:]

def blob(text):
    if "void obj_score(" in text:text=without_item_info_stage(text)
    b=text.encode()
    return hashlib.sha1(b"blob "+str(len(b)).encode()+b"\0"+b).hexdigest()

class GuardianCatalog(unittest.TestCase):
    def test_whole_module_restores_exact_base(self):
        self.assertEqual(blob(without_guardian_stage((SRC/"more.c").read_text())),
                         "30fd54d167213e22ebff09f7246bea1c8c2889d0")

    def test_macro_getters_consumers_serializers_and_interpreter_unchanged(self):
        for path,sha in (("merc.h","b843f157662ab181daabe913d74685f1c8bc75e6"),
                         ("act_info.c","bcd53fad2731f9b30016c35eb50d16230280a489"),
                         ("clan.c","a5570740eb895b02ff7aa058ed631e153ecbeef3"),
                         ("interp.c","ccb0125d4264a8a5447618c29a9240ec674d2eb4"),
                         ("save.c","1f3317263a45d1a886db411d4c0a52a5bba92909")):
            text=(SRC/path).read_text()
            if path=="save.c":text=without_guardian_lifecycle_save(text)
            if path=="clan.c":text=without_aura_stage(text)
            self.assertEqual(blob(text),sha)
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":3,"handoff":2})

    def test_single_native_toggle_zero_cost_no_new_actors_or_forms(self):
        body=(SRC/"more.c").read_text().split("void do_spiritguardian(",1)[1].split(
            "\nvoid do_succulence(",1)[0]
        self.assertEqual(body.count("SET_BIT(ch->more, MORE_GUARDIAN)"),1)
        self.assertEqual(body.count("REMOVE_BIT(ch->more, MORE_GUARDIAN)"),1)
        self.assertLess(body.index("if (IS_MORE(ch, MORE_GUARDIAN))"),
                        body.index("const bool monk_parent"))
        for text in ("mantra >= 6","DISC_NECROMANCY) < 4 && !monk_ready",
                     "TRIBE_CHILDREN_OF_GAIA) < 4 && !monk_ready"):
            self.assertIn(text,body)
        self.assertNotRegex(body,r"\b(?:create_mobile|char_to_room|WAIT_STATE|number_range)\s*\(")
        self.assertNotRegex(body,r"ch->(?:mana|primal|quest|class)\s*[-+*/]?=(?!=)")
        self.assertNotIn("AFF_POLYMORPH",body)

    def test_real_engine_snapshots_and_overlap_matrix(self):
        text=(ROOT/"tests/test_hybrid_guardian.c").read_text()
        for part in ("0x4caf6723345865ab","rows==240","count==4860","count==5184",
                     "count==90","load_char_obj","load_char_short","do_readaura",
                     "secondary_progression.full_branch=LONG_MAX","LONG_MIN"):
            self.assertIn(part,text)
        self.assertNotRegex(text,r"\b(?:interpret|guardian_message|hybrid_power_rank)\s*\([^;]*\)\s*\{")
        self.assertIn("tests/test_hybrid_guardian",make_values(ROOT,"TESTS"))
        self.assertIn("tests/test_hybrid_guardian: tests/test_hybrid_guardian.c $(ENGINE_OBJECTS)",
                      (ROOT/"Makefile").read_text())

    def test_guardian_uses_effective_rank_not_native_decoy_or_added_banks(self):
        body=(SRC/"more.c").read_text().split("void do_spiritguardian(",1)[1].split(
            "\nvoid do_succulence(",1)[0]
        self.assertIn("hybrid_power_parent(ch, CLASS_MONK)",body)
        self.assertIn("hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_MANTRA)",body)
        self.assertNotIn("pcdata->monk[",body)
        self.assertNotRegex(body,r"(?:mantra|monk_ready)\s*\+")

    def test_documented_native_limits_and_partial_status(self):
        doc=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("Secondary Monk Spirit Guardian","warning toggle","zero mana",
                     "4,860 paired","5,184 overlap","240 valid native","90 full/short",
                     "automatic revocation","No conversion"):
            self.assertIn(part,doc)
        self.assertIn("spiritguardian (6; no mana cost)",
                      (SRC/"classes/hybrid/help.c").read_text())

if __name__=="__main__":
    unittest.main()
