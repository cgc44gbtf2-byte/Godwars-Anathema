"""Narrow load/save/pulse integration; no broad class or command permission changes."""
import hashlib
import json
import unittest
from test_class_layout import ROOT,SRC,make_values

def once(text,old,new):
    assert text.count(old)==1,repr(old)
    return text.replace(old,new,1)

def without_guardian_lifecycle_save(text):
    text=once(text,'#include "guardian.h"\n',"")
    text=once(text,'    fprintf( fp, "More         %li\\n",\n'
                   '        guardian_saved_more(ch, monk_favour_saved_more(ch, &time_info)));',
                   '    fprintf( fp, "More         %li\\n",monk_favour_saved_more(ch, &time_info));')
    return once(text,"                guardian_reconcile(ch);\n","")

def blob(text):
    data=text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()

class GuardianLifecycleCatalog(unittest.TestCase):
    def test_only_narrow_save_and_pulse_changes(self):
        self.assertEqual(blob(without_guardian_lifecycle_save((SRC/"save.c").read_text())),
                         "1f3317263a45d1a886db411d4c0a52a5bba92909")
        text=(SRC/"combat_status.c").read_text()
        text=once(text,'#include "guardian.h"\n',"")
        text=once(text,"        guardian_reconcile(ch);\n","")
        self.assertEqual(blob(text),"2c7f2f2e6583abe95ddbbdfca2373e9e63e03112")
        for path,sha in (("more.c","c44eb80fd4a319117ee12f749fe889f66f4329c5"),
                         ("comm.c","e880af206f10a4cf2512e6011ed15095e8d8a0f7"),
                         ("merc.h","b843f157662ab181daabe913d74685f1c8bc75e6")):
            self.assertEqual(blob((SRC/path).read_text()),sha)

    def test_current_grants_only_no_rank_addition_or_auto_activation(self):
        text=(SRC/"guardian.c").read_text()
        for part in ("ch->pcdata == NULL","get_disc(ch, DISC_NECROMANCY) >= 4",
                     "get_tribe(ch, TRIBE_CHILDREN_OF_GAIA) >= 4",
                     "hybrid_power_parent(ch, CLASS_MONK)",
                     "HYBRID_TRAIN_MONK, MONK_MANTRA) >= 6"):
            self.assertIn(part,text)
        self.assertNotIn("SET_BIT(",text)
        self.assertNotIn("pcdata->monk[",text)
        self.assertNotRegex(text,r"\b(?:get_disc|get_tribe|hybrid_power_rank)\([^;]+\)\s*\+")

    def test_only_guardian_bit_pure_save_projection_no_cost_rng_or_output(self):
        text=(SRC/"guardian.c").read_text()
        for forbidden in ("->mana","->hit","->level",
                          "send_to_char","stc(","number_","WAIT_STATE","MORE_FAVOUR",
                          "MORE_BRISTLES","create_mobile","hybrid_player_commit"):
            self.assertNotIn(forbidden,text)
        self.assertNotRegex(text,r"->(?:class|pcdata)\s*=(?!=)")
        self.assertIn("return more & ~MORE_GUARDIAN;",text)
        self.assertEqual(text.count("ch->more ="),1)
        self.assertIn("IS_NPC(ch) || !IS_MORE(ch, MORE_GUARDIAN)",text)

    def test_reconcile_after_validated_load_and_before_input_even_linkdead(self):
        text=(SRC/"save.c").read_text()
        end=text.split('if ( !str_cmp( word, "End" ) )',1)[1].split('KEY( "Exp"',1)[0]
        self.assertLess(end.index("hybrid_player_load_finish"),end.index("guardian_reconcile"))
        pulse=(SRC/"combat_status.c").read_text().split("void combat_status_update",1)[1]
        self.assertLess(pulse.index("guardian_reconcile(ch)"),pulse.index("combat_pause"))
        self.assertNotIn("desc",pulse)
        comm=(SRC/"comm.c").read_text()
        self.assertLess(comm.index("combat_status_update();"),comm.index("read_from_descriptor( d )"))
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":3,"handoff":2})

    def test_real_engine_matrix_persistence_and_sanitizer_linkage(self):
        text=(ROOT/"tests/test_guardian_lifecycle.c").read_text()
        for part in ("eligibility_cases==2970","pulse_cases==11880",
                     "projection_cases==20790","load_cases==630","bad<8",
                     "load_char_short","load_char_obj","save_char_obj_checked",
                     "combat_status_update","LONG_MIN","CON_NOTE_TEXT",
                     "descs[0].original=ch","legacy_bit()"):
            self.assertIn(part,text)
        self.assertIn("tests/test_guardian_lifecycle",make_values(ROOT,"TESTS"))
        mk=(ROOT/"Makefile").read_text()
        self.assertIn("rm -f tests/save_info.o",mk)
        self.assertIn("Genesis/src/guardian.c tests/save_info.o",mk)
        self.assertIn("guardian.o",(SRC/"Makefile").read_text())

    def test_documented_legacy_and_explicit_reactivation_limits(self):
        text=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("automatic revocation","next main-loop pulse","pure save",
                     "does not automatically reactivate","630 full/short",
                     "not a provenance migration","No conversion"):
            self.assertIn(part,text)

if __name__=="__main__":
    unittest.main()
