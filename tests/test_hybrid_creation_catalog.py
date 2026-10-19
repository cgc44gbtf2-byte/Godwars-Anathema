"""Only the Sorcerer material recipe gate gains secondary-parent access."""
import hashlib
import re
import unittest
from test_class_layout import ROOT, SRC, make_values
from test_hybrid_writing_catalog import without_sorcerer_writing_adapter


def without_sorcerer_creation_adapter(text):
    """Reverse exactly the scoped entry gate, not arbitrary class predicates."""
    text=without_sorcerer_writing_adapter(text)
    include='#include "classes/hybrid/access.h"\n'
    assert text.count(include)==1
    start=text.index("void do_sorccreate(")
    end=text.index("\nvoid do_vision(",start)
    body=text[start:end]
    gate="if (!hybrid_power_parent(ch, CLASS_SORCERER))"
    assert body.count(gate)==1 and text.count("hybrid_power_parent(")==1
    body=body.replace(gate,"if (!IS_SORCERER(ch))",1)
    return (text[:start]+body+text[end:]).replace(include,"",1)


class HybridCreationCatalog(unittest.TestCase):
    def test_only_one_creation_entry_gate_changes(self):
        data=without_sorcerer_creation_adapter((SRC/"classes/sorcerer/sorcerer.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "91fd66c8ba3a020d09525ca3fdbd13c33065812d")

    def test_native_recipes_prices_owner_and_zero_resale_retained(self):
        body=(SRC/"classes/sorcerer/sorcerer.c").read_text().split(
            "void do_sorccreate(",1)[1].split("\nvoid do_vision(",1)[0]
        for part in ("get_obj_index(OBJ_VNUM_PROTOPLASM)","obj->cost = 0",
                     "obj->points = 0","obj->questmaker = str_dup(ch->name)",
                     "ch->primal -= 10","ITEM_BOOK","ITEM_PAGE","ITEM_PEN"):
            self.assertIn(part,body)
        self.assertLess(body.index("get_obj_index("),body.index("ch->primal -= 10"))
        self.assertNotRegex(body,r"ch->(?:class|pcdata->\w+)\s*=(?!=)")
        self.assertNotRegex(body,r"\b(?:hybrid_player_attach|hybrid_training_commit|get_color)\s*\(")

    def test_interpreter_tier_and_other_sorcerer_routes_not_broadened(self):
        table=(SRC/"interp.c").read_text()
        self.assertRegex(table,r'"sorccreate",\s*do_sorccreate,\s*POS_RESTING,\s*LEV_SOR')
        self.assertRegex(table,r'"chant",\s*do_chant,\s*POS_FIGHTING,\s*LEV_SOR')
        text=(SRC/"classes/sorcerer/sorcerer.c").read_text()
        for name in ("colors","vision","enchant"):
            self.assertIn("if (!IS_SORCERER(ch))",
                          text.split("void do_"+name+"(",1)[1].split("\nvoid ",1)[0])
        chant=text.split("void do_chant(",1)[1].split("\nvoid ",1)[0]
        self.assertNotIn("IS_SORCERER(",chant)  # Already shared before this stage.

    def test_real_engine_replay_not_permission_or_recipe_stubs(self):
        self.assertIn("tests/test_hybrid_creation",make_values(ROOT,"TESTS"))
        make=(ROOT/"Makefile").read_text()
        self.assertIn("tests/test_hybrid_creation: tests/test_hybrid_creation.c $(ENGINE_OBJECTS)",make)
        self.assertIn("tests/test_object_strings",make_values(ROOT,"TESTS"))
        factory=(SRC/"db.c").read_text().split("OBJ_DATA *create_object(",1)[1].split(
            "\nOBJ_INDEX_DATA",1)[0]
        for field in ("name","short_descr","description","chpoweron","chpoweroff",
                      "chpoweruse","victpoweron","victpoweroff","victpoweruse"):
            self.assertRegex(factory,r"obj->"+field+r"\s*=\s*str_dup\(")
        test=(ROOT/"tests/test_hybrid_creation.c").read_text()
        for part in ("interpret(ch,line)","0x54d9b5797dc7217f","comparisons==2304",
                     "LONG_MIN","LONG_MAX","load_char_short","load_char_obj",
                     "save_char_obj_checked(ch)","ch->primal==980",
                     "IS_SORCERER(ch)==!!order","get_color(ch,BLUE_MAGIC)==0"):
            self.assertIn(part,test)
        self.assertNotRegex(test,r"\b(?:void|int|bool|long)\s+(?:hybrid_power_parent|"
                            r"create_object|do_sorccreate|interpret)\s*\([^;]*\)\s*\{")

    def test_secondary_invalid_npc_and_unrelated_records_rejected(self):
        test=(ROOT/"tests/test_hybrid_creation.c").read_text()
        for part in ("hybrid_present=0","hybrid_present=2","player_load_failed=1",
                     "secondary_class=CLASS_NONE","ACT_IS_NPC","ch->level=LEV_AWA",
                     "colors[BLUE_MAGIC]=6","mode==10?CLASS_WEREWOLF"):
            self.assertIn(part,test)
        text=(SRC/"classes/hybrid/access.c").read_text()
        self.assertIn("hybrid_player_save_valid(ch)",text)
        self.assertIn("s && s->secondary_class==parent",text)

    def test_help_documents_materials_not_conversion_or_atomic_crafting(self):
        text=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("sorccreate book","sorccreate page","sorccreate pen",
                     "ordinary player-save timing","not an atomic inventory/currency transaction"):
            self.assertIn(part,text)


if __name__=="__main__":
    unittest.main()
