"""Audited Drow adapters are not permission to enable other Drow state."""
import hashlib
import json
import re
import unittest
from test_class_layout import ROOT, SRC, make_values


def between(text,name,next_name):
    return text.split("void "+name,1)[1].split("void "+next_name,1)[0]


class HybridDrowCatalog(unittest.TestCase):
    def test_native_bodies_preserved_except_owned_permission_checks(self):
        text=(SRC/"classes/drow/drow.c").read_text()
        from drow_globe_source import without_drow_globe_stage
        text=without_drow_globe_stage(text)
        from drow_poison_source import without_drow_poison_stage
        text=without_drow_poison_stage(text)
        self.assertEqual(text.count('#include "classes/hybrid/access.h"\n'),1)
        self.assertEqual(text.count("if (!hybrid_power_parent(ch,CLASS_DROW))"),3)
        self.assertEqual(text.count("if (!hybrid_power_drow(ch,"),3)
        safety_head = (
            "    ROOM_INDEX_DATA *combat_room;\n\n"
            "    if (ch == NULL || IS_NPC(ch) || ch->pcdata == NULL || ch->in_room == NULL)\n"
            "        return;\n")
        self.assertEqual(text.count(safety_head),1)
        self.assertEqual(text.count("    combat_room = ch->in_room;\n"),1)
        self.assertEqual(text.count(
            "    if (combat_character_in_room(combat_room, ch)) WAIT_STATE(ch, 8);"),1)
        text=text.replace(safety_head,"").replace("    combat_room = ch->in_room;\n","").replace(
            "    if (combat_character_in_room(combat_room, ch)) WAIT_STATE(ch, 8);",
            "    WAIT_STATE(ch, 8);")
        # Remove only the extra blank line introduced before the entry guard.
        text=text.replace("    CHAR_DATA *victim;\n\n\n    one_argument(argument, arg);",
                          "    CHAR_DATA *victim;\n\n    one_argument(argument, arg);")
        restored=text.replace('#include "classes/hybrid/access.h"\n',"").replace(
            "if (!hybrid_power_parent(ch,CLASS_DROW))","if (!IS_DROW(ch))")
        for flag in ("DROW_ATTUNE","DROW_FIRE","DROW_ACID"):
            restored=restored.replace("if (!hybrid_power_drow(ch,"+flag+"))",
                                      "if (!get_drow(ch, "+flag+"))")
        data=restored.encode()
        sha=hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()
        self.assertEqual(sha,"dc179a1b0181b21977d16f516bd5f1825ce4c2cd")

    def test_other_drow_handlers_remain_native(self):
        text=(SRC/"classes/drow/drow.c").read_text()
        from drow_globe_source import without_drow_globe_stage
        text=without_drow_globe_stage(text)
        for name,next_name in (("do_drowarmor","do_door"),
                               ("do_door","do_prayer"),
                               ("do_globedark","do_shoot")):
            body=between(text,name,next_name)
            self.assertNotIn("hybrid_power_",body)
            self.assertIn("IS_DROW(ch)",body)

    def test_no_caster_form_resource_or_class_swap(self):
        text=(SRC/"classes/drow/drow.c").read_text()
        for body in (between(text,"do_attune","do_acidbolt"),
                     between(text,"do_faeriefire","do_summonspider")):
            self.assertNotRegex(body,r"ch->(?:class|mana|primal|exp)\s*[-+*/]?=(?!=)")
            self.assertNotRegex(body,r"(?:SET_BIT|REMOVE_BIT)\s*\(ch->")
            self.assertNotIn("hybrid_secondary_resource_",body)
            self.assertNotIn("hybrid_player_commit",body)

    def test_native_ownership_protection_and_duplicate_guards(self):
        text=(SRC/"classes/drow/drow.c").read_text()
        attune=between(text,"do_attune","do_acidbolt")
        faerie=between(text,"do_faeriefire","do_summonspider")
        for flag in ("ITEM_QUEST","ITEM_AMMO","ITEM_EGG","ITEM_VOODOO",
                     "ITEM_MONEY","ITEM_TREASURE","ITEM_PAGE","QUEST_ARTIFACT"):
            self.assertIn(flag,attune)
        self.assertIn("str_cmp(ch->name,obj->questowner)",attune)
        self.assertIn("ITEM_LOYAL",attune)
        self.assertIn("is_safe(ch, victim)",faerie)
        self.assertIn("AFF_FAERIE_FIRE",faerie)
        self.assertEqual(faerie.count("affect_to_char(victim, &af)"),3)

    def test_real_engine_interpreter_coverage_in_default_suite(self):
        make=(ROOT/"Makefile").read_text()
        self.assertIn("tests/test_hybrid_access.c $(ENGINE_OBJECTS)",make)
        self.assertIn("python3 tests/test_hybrid_drow_catalog.py",make)
        self.assertEqual(make_values(ROOT,"TESTS").count("tests/test_hybrid_access"),1)
        self.assertEqual(make_values(ROOT,"ENGINE_SOURCES").count(
            "Genesis/src/classes/drow/drow.c"),1)
        tests=(ROOT/"tests/test_hybrid_access.c").read_text()
        self.assertIn('interpret(ch,"faeriefire beta")',tests)
        self.assertIn('interpret(ch,"attunement token")',tests)
        self.assertIn("assert(cases==10)",tests)
        self.assertIn("drow_utilities();",tests)

    def test_no_conversion_save_epoch_or_handoff_change(self):
        compat=json.loads((ROOT/"deploy/update_compat.json").read_text())
        self.assertEqual(compat["save_epoch"],4)
        self.assertEqual(compat["handoff"],2)
        preview=(SRC/"classes/hybrid/selfbrid.c").read_text()
        self.assertNotIn("hybrid_player_attach(",preview)
        self.assertNotIn("hybrid_training_commit(",preview)


if __name__=="__main__":
    unittest.main()
