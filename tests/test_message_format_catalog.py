"""Scope and build checks supplement the real-engine formatter suite."""
import hashlib
import re
import unittest
from test_class_layout import ROOT,SRC,make_values
from identify_source import without_identify_stage

class MessageFormatCatalog(unittest.TestCase):
    def test_shared_limit_and_completion_used_by_all_dialects(self):
        source=(SRC/"core.c").read_text()
        self.assertEqual(source.count("const char *limit = buf + sizeof(buf) - 3;"),3)
        self.assertEqual(source.count("message_finish(buf, &point, limit);"),3)
        self.assertEqual(source.count("message_append(&point, limit, i);"),3)
        self.assertNotRegex(source,r"while\s*\(\s*\(\s*\*point\s*=")
        self.assertIn("while (*point < limit && *text",source)
        self.assertIn('message_append(&point, limit, "$")',source)

    def test_audiences_visibility_and_object_isolation_retained(self):
        source=(SRC/"core.c").read_text()
        for part in ("TO_CHAR && to != ch","TO_VICT && (to != vch || to == ch)",
                     "TO_ROOM && to == ch","TO_NOTVICT","ROOM_VNUM_IN_OBJECT",
                     "can_see_obj(observer, obj)","PERS(person, observer)",
                     "obj->chobj == observer","CON_PLAYING"):
            self.assertIn(part,source)
        self.assertEqual(source.count("while (p > buf && p[-1] == '#')"),1)

    def test_null_context_and_scoped_familiar_visibility(self):
        source=(SRC/"core.c").read_text()
        self.assertEqual(source.count("(!IS_NPC(ch) && ch->pcdata == NULL)) return;"),3)
        self.assertEqual(source.count("if (!IS_NPC(to) && to->pcdata == NULL) continue;"),3)
        self.assertIn("wizard->pcdata != NULL",source)
        self.assertIn("had_fam_vision = IS_EXTRA(to, EXTRA_CAN_SEE) != 0;",source)
        self.assertEqual(source.count("if (!had_fam_vision) REMOVE_BIT"),4)
        self.assertNotIn('sprintf(kav,"%s',source)

    def test_native_apis_and_other_formatting_callers_unchanged(self):
        expected={"handler.c":"5661917ea1caf18177e2cb5c1e4dd6c300475500",
                  "comm.c":"e880af206f10a4cf2512e6011ed15095e8d8a0f7",
                  "magic.c":"ff6670721b45989e1e24b8c337f4e8ad766a8b18",
                  "clan.c":"89d083ddce8367e98e58cec721cf290d9da105b2",
                  "merc.h":"b843f157662ab181daabe913d74685f1c8bc75e6",
                  "interp.c":"ccb0125d4264a8a5447618c29a9240ec674d2eb4"}
        for path,sha in expected.items():
            b=(SRC/path).read_bytes()
            if path=="magic.c": b=without_identify_stage(b.decode()).encode()
            self.assertEqual(hashlib.sha1(b"blob "+str(len(b)).encode()+b"\0"+b).hexdigest(),sha)
        source=(SRC/"core.c").read_text()
        self.assertEqual(re.findall(r"^void (\w+)\(",source,re.M),["act","act2","kavitem"])

    def test_real_engine_native_golden_and_boundary_tests_wired(self):
        self.assertIn("tests/test_message_format",make_values(ROOT,"TESTS"))
        text=(ROOT/"tests/test_message_format.c").read_text()
        for part in ("rows==1215","17448779909195589922","f<3","n<8",
                     "people[3].extra==before","ROOM_VNUM_IN_OBJECT","PLR_ANSI",
                     "people[1].pcdata=NULL","TO_VICT's arg2 is a character"):
            self.assertIn(part,text)
        self.assertNotRegex(text,r"\b(?:act|act2|kavitem|can_see)\s*\([^;]*\)\s*\{")

    def test_documented_limits_and_no_global_safety_claim(self):
        docs=(ROOT/"docs/MESSAGES.md").read_text()
        for part in ("1,215","MSL - 3","1215","spell_identify",
                     "pointer-type contract","No combat","familiar","trailing `$`"):
            self.assertIn(part,docs)

if __name__=="__main__":unittest.main()
