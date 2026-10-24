"""Magical page authoring gains two scoped checks; native grammar stays frozen."""
import hashlib
import re
import unittest
from test_class_layout import ROOT, SRC, make_values
from test_hybrid_enchant_catalog import without_sorcerer_enchant_adapter


def without_sorcerer_writing_adapter(text):
    text=without_sorcerer_enchant_adapter(text)
    start=text.index("void do_write(")
    end=text.index("\n\n/* Shared across mixed page kinds.",start)
    body=text[start:end]
    gate="hybrid_power_parent(ch, CLASS_SORCERER)"
    assert body.count(gate)==2
    body=body.replace(gate,"IS_SORCERER(ch)")
    return text[:start]+body+text[end:]


class HybridWritingCatalog(unittest.TestCase):
    def test_exactly_two_checks_no_native_grammar_changes(self):
        normalized=without_sorcerer_writing_adapter((SRC/"classes/sorcerer/sorcerer.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(normalized)).encode()+b"\0"+normalized).hexdigest(),
                         "ddb15b3d04b0c1cbbde244c98f41c3a79997c355")

    def test_ordinary_text_staff_override_and_native_tier_retained(self):
        text=(SRC/"classes/sorcerer/sorcerer.c").read_text()
        body=text.split("void do_write(",1)[1].split("\n\n/* Shared across mixed page kinds.",1)[0]
        self.assertEqual(body.count("!IS_IMM(ch)"),2)
        self.assertIn("&& !IS_SET(obj->spectype, ADV_FINISHED)",body)
        self.assertLess(body.index('!str_cmp(arg2, "title")'),body.index("hybrid_power_parent("))
        self.assertIn("if (IS_NPC(ch)) return;",body)
        self.assertRegex((SRC/"interp.c").read_text(),r'"write",\s*do_write,\s*POS_RESTING,\s*LEV_SLE')
        self.assertNotRegex(body,r"(?:get_color|hybrid_book_color_rank|hybrid_training_commit|hybrid_player_attach)\(")
        self.assertNotRegex(body,r"ch->(?:class|primal|mana|exp|pcdata->\w+)\s*[-+]?=(?!=)")

    def test_native_flags_colors_and_length_guard(self):
        text=without_sorcerer_enchant_adapter((SRC/"classes/sorcerer/sorcerer.c").read_text())
        body=text.split("void do_write(",1)[1].split("\n\n/* Shared across mixed page kinds.",1)[0]
        for part in ("ADV_FAILED","ADV_STARTED","ADV_FINISHED","ADV_DAMAGE","ADV_AFFECT","ADV_ACTION",
                     "ADV_NEXT_PAGE","ADV_REVERSED","MAX_STRING_LENGTH - 4"):
            self.assertIn(part,body)
        for color in ("blue","red","green","yellow","purple","black","random"):
            self.assertIn('"end.'+color+'"',body)
        for name in ("vision","enchant"):
            self.assertIn("if (!IS_SORCERER(ch))",text.split("void do_"+name+"(",1)[1].split("\nvoid ",1)[0])

    def test_replays_link_the_real_engine(self):
        self.assertIn("tests/test_hybrid_writing",make_values(ROOT,"TESTS"))
        self.assertIn("tests/test_hybrid_writing: tests/test_hybrid_writing.c $(ENGINE_OBJECTS)",
                      (ROOT/"Makefile").read_text())
        test=(ROOT/"tests/test_hybrid_writing.c").read_text()
        for part in ("interpret(ch,command)","0x5eb0dc614b3ede8c","p<9","order<2","c<32",
                     "save_char_obj_checked(ch)","load_char_short","load_char_obj",
                     "IS_SORCERER(ch)==!!order","get_color(ch,BLUE_MAGIC)==0"):
            self.assertIn(part,test)
        self.assertNotRegex(test,r"\b(?:void|int|bool|long)\s+(?:do_write|hybrid_power_parent|"
                            r"interpret|save_char_obj_checked)\s*\([^;]*\)\s*\{")

    def test_record_pen_npc_and_ordinary_writer_cases(self):
        test=(ROOT/"tests/test_hybrid_writing.c").read_text()
        for part in ("hybrid_present=0","hybrid_present=2","player_load_failed=1",
                     "secondary_class=CLASS_NONE","colors[BLUE_MAGIC]=6","ACT_IS_NPC",
                     "LEV_IMP","LEV_SLE","unequip_char","CLASS_VAMPIRE",
                     "Ordinary annotation","Ordinary title","MAX_STRING_LENGTH-5"):
            self.assertIn(part,test)

    def test_docs_distinguish_authoring_from_cast_ranks(self):
        text=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("write page line start.damage.spell","write page line end.red",
                     "Authoring does not grant color ranks","ordinary player-save timing",
                     "Vision ownership"):
            self.assertIn(part,text)

if __name__=="__main__":
    unittest.main()
