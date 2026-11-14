"""Disquiet identities, scope, deployment compatibility and real-engine coverage."""
import hashlib
import json
from pathlib import Path
import re
import unittest
from werewolf_combat_source import without_werewolf_combat_stage

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src"

def blob(text):
    data=text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()

OLD_DISQUIET=(
    'void do_disquiet( CHAR_DATA *ch, char *argument )\n{\n'
    '    if (!IS_WEREWOLF(ch))\n    {\n'
    '\tstc("Huh?\\n\\r", ch);\n\treturn;\n    }\n\n'
    '    if (get_breed(ch, BREED_HOMID) < 4)\n    {\n'
    '\tstc("You require the level 4 Homid power to use Disquiet.\\n\\r", ch);\n'
    '\treturn;\n    }\n\n'
    '    stc("Gutted!\\n\\r", ch);\n    return;\n}\n\n'
)

class DisquietCatalog(unittest.TestCase):
    def test_all_other_breed_code_byte_identical_to_36b0d014(self):
        text=without_werewolf_combat_stage(
            (SRC/"classes/werewolf/wwbreed.c").read_text())
        for line in ('#include <limits.h>\n','#include "guardian.h"\n',
                     '#include "../hybrid/player.h"\n'):
            self.assertEqual(text.count(line),1)
            text=text.replace(line,"",1)
        text,count=re.subn(r"void do_disquiet\(.*?(?=void do_spiritward\()",
                           lambda _:OLD_DISQUIET,text,flags=re.S)
        self.assertEqual(count,1)
        self.assertEqual(blob(text),"76db250da39a1bbf6516b7620385e389f1edfc23")

    def test_skill_table_only_appends_affect_identity(self):
        text=(SRC/"const.c").read_text()
        start=text.index("    /* Append only:")
        end=text.index("\n};",start)
        added=text[start:end]
        self.assertIn('"disquiet"',added)
        self.assertIn("spell_null",added)
        self.assertIn("SLOT(0)",added)
        self.assertEqual(blob(text[:start]+text[end+1:]),
                         "c95f1f6fbde4fc2eb36585d62f94628621b3ec47")
        self.assertRegex((SRC/"merc.h").read_text(),r"#define\s+MAX_SKILL\s+132\b")

    def test_no_calm_flag_or_secondary_permission_port(self):
        body=(SRC/"classes/werewolf/wwbreed.c").read_text().split(
            "void do_disquiet(",1)[1].split("void do_spiritward(",1)[0]
        for text in ("MORE_CALM","MORE_ANGER","do_order(","hybrid_power_parent("):
            self.assertNotIn(text,body)
        for text in ("IS_WEREWOLF(ch)","BREED_HOMID","rank < 4","rank * 5",
                     "is_affected(victim, sn)","LONG_MIN + penalty",
                     "WILL_PRESENCE","RES_PRESENCE","guardian_eligible"):
            self.assertIn(text,body)

    def test_standard_affect_identity_and_ownership(self):
        body=(SRC/"classes/werewolf/wwbreed.c").read_text().split(
            "void do_disquiet(",1)[1].split("void do_spiritward(",1)[0]
        for text in ("AFFECT_DATA effect = {0}","effect.type = sn",
                     "effect.duration = rank","effect.modifier = -penalty",
                     "APPLY_HITROLL","APPLY_DAMROLL","WAIT_STATE(ch, 12)"):
            self.assertIn(text,body)
        self.assertEqual(body.count("affect_to_char(victim, &effect)"),2)

    def test_saved_effect_requires_epoch_four_handoff_unchanged(self):
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})
        doc=(ROOT/"deploy/UPDATING.md").read_text()
        self.assertIn("Named Disquiet",doc)
        self.assertIn("binary-only downgrade from4 is refused",doc)

    def test_real_engine_suite_is_part_of_both_test_phases(self):
        make=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_disquiet",make)
        self.assertIn("tests/test_disquiet: tests/test_disquiet.c",make)
        fixture=(ROOT/"tests/test_disquiet.c").read_text()
        for text in ('#include "test_hybrid_aura.c"',"interpret(actors,command)",
                     "char_update();","load_char_obj","load_char_short",
                     "save_char_obj_checked","LONG_MIN","LONG_MAX",
                     "original_and_secondary_access"):
            self.assertIn(text,fixture)

    def test_help_does_not_advertise_stub_or_calm_control(self):
        from test_werewolf_help_catalog import catalog
        row=catalog()[("Homid",4)]
        self.assertEqual(row["tag"],"disquiet")
        self.assertIn("hitroll and damroll",row["description"])
        self.assertNotIn("stub",row["description"].lower())

if __name__=="__main__":
    unittest.main()
