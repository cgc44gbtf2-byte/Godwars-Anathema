"""Celestial's two access predicates, bounded native fixes, no global grants."""
import hashlib
import json
import re
import unittest
from test_class_layout import ROOT,SRC,make_values
from test_guardian_lifecycle_catalog import without_guardian_lifecycle_save,without_hybrid_forms_save
from item_info_source import without_item_info_stage
from progress_metrics_source import without_progress_metrics_stage

def once(text,old,new):
    assert text.count(old)==1,repr(old)
    return text.replace(old,new,1)

def blob(text):
    text=without_progress_metrics_stage(text)
    if "void obj_score(" in text:text=without_item_info_stage(text)
    data=text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()

def without_celestial_stage(text):
    start=text.index("void do_celestial(")
    end=text.index("\nvoid do_lifecloak(",start)
    body=text[start:end]
    body=once(body,
        "if (!hybrid_power_parent(ch, CLASS_MONK) || ch->pcdata == NULL || ch->in_room == NULL)",
        "if (!IS_MONK(ch))")
    body=once(body,
        "hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_MANTRA)",
        "get_monk(ch, MONK_MANTRA)")
    body=once(body,'    if (ch == NULL) return;\n'
                   '    argument = one_argument(argument ? argument : "", arg);',
                   '    argument = one_argument(argument, arg);')
    body=once(body,'    if (ch->mana < 1500)\n    {\n'
                   '        stc("You need 1500 mana to open a Celestial path.\\n\\r", ch);\n'
                   '        return;\n    }\n\n',"")
    return text[:start]+body+text[end:]

def without_celestial_visibility_stage(text):
    start=text.index("bool can_see( CHAR_DATA *ch, CHAR_DATA *victim )")
    end=text.index("\n}\n",start)+3
    body=text[start:end]
    body=once(body,"    if (ch == NULL || victim == NULL) return FALSE;\n","")
    body=once(body,"    /* Preserve self/staff/item-vision shortcuts above.  Ordinary room sight\n"
                   "     * cannot inspect an unplaced observer or target during world lookup. */\n"
                   "    if (ch->in_room == NULL || victim->in_room == NULL) return FALSE;\n","")
    return text[:start]+body+text[end:]

class CelestialCatalog(unittest.TestCase):
    def test_both_entire_native_modules_restore_exact_base(self):
        self.assertEqual(blob(without_celestial_stage((SRC/"classes/monk/monk.c").read_text())),
                         "c573750215f15c21be34346aa1afeff2a3918aec")
        self.assertEqual(blob(without_celestial_visibility_stage((SRC/"handler.c").read_text())),
                         "cd73ac2703176871b2e07bbbecc5c2914a53c4eb")

    def test_no_macro_getter_interpreter_or_save_changes(self):
        for path,sha in (("merc.h","b843f157662ab181daabe913d74685f1c8bc75e6"),
                         ("interp.c","ccb0125d4264a8a5447618c29a9240ec674d2eb4"),
                         ("act_info.c","d4b24a952210f67fbfadabb480e9908422c72280"),
                         ("save.c","1f3317263a45d1a886db411d4c0a52a5bba92909")):
            text=(SRC/path).read_text()
            if path=="save.c":
                text=without_hybrid_forms_save(without_guardian_lifecycle_save(text))
            if path=="merc.h":
                text=text.replace("    int                 hybrid_forms_present;\n","")
            self.assertEqual(blob(text),sha)
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

    def test_rank_cost_and_native_destination_policy(self):
        body=(SRC/"classes/monk/monk.c").read_text().split("void do_celestial(",1)[1].split(
            "\nvoid do_lifecloak(",1)[0]
        for part in ("hybrid_power_parent(ch, CLASS_MONK)","HYBRID_TRAIN_MONK, MONK_MANTRA) < 7",
                     "ROOM_PRIVATE","ROOM_SOLITARY","ROOM_NO_TELEPORT","EXTRA_SUMMON",
                     "victim->level > 800","victim->in_room->vnum == ch->in_room->vnum",
                     "ch->mana < 1500","ch->mana -= 1500","guardian_message(victim)"):
            self.assertIn(part,body)
        self.assertLess(body.index("ch->mana < 1500"),body.index("char_from_room(ch)"))
        self.assertNotRegex(body,r"\b(?:WAIT_STATE|number_range|hybrid_player_attach|save_char_obj)\s*\(")
        self.assertRegex((SRC/"interp.c").read_text(),
                         r'"celestial",\s*do_celestial,\s*POS_STANDING,\s*LEV_AWA')

    def test_visibility_retains_privileged_shortcuts(self):
        body=(SRC/"handler.c").read_text().split("bool can_see( CHAR_DATA",1)[1]
        null_room=body.index("if (ch->in_room == NULL || victim->in_room == NULL)")
        for shortcut in ("if (ch == victim)","if (IS_BUI(ch))","if (IS_ITEMAFF(ch, ITEMA_VISION))"):
            self.assertLess(body.index(shortcut),null_room)
        self.assertLess(null_room,body.index("victim->in_room->people"))

    def test_real_engine_and_native_goldens(self):
        text=(ROOT/"tests/test_hybrid_celestial.c").read_text()
        for part in ("0x4e015ab4b8be236a","count==8100","count==90",
                     "load_char_obj","load_char_short","hybrid_secondary_specialize",
                     "LONG_MIN","actor","--visibility"):
            self.assertIn(part,text)
        self.assertNotRegex(text,r"\b(?:interpret|char_to_room|can_see|hybrid_power_rank)\s*\([^;]*\)\s*\{")
        self.assertIn("tests/test_hybrid_celestial",make_values(ROOT,"TESTS"))
        self.assertIn("tests/test_hybrid_celestial: tests/test_hybrid_celestial.c $(ENGINE_OBJECTS)",
                      (ROOT/"Makefile").read_text())

    def test_documented_limits(self):
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("Secondary Monk Celestial Path","1,500 mana","180 native",
                     "8,100 paired","90 full/short","destination","no new recovery",
                     "No conversion","Chimera"):
            self.assertIn(part,docs)
        self.assertIn("celestial (7; 1500 mana)",(SRC/"classes/hybrid/help.c").read_text())

if __name__=="__main__":
    unittest.main()
