import hashlib
import unittest
from test_class_layout import ROOT, SRC, make_values
from native_drow_source import without_native_drow_stage

BASE = "5a19a2e21fbae6df7814ab263e5fb717e3705352"

def blob(text):
    b=text.encode()
    return hashlib.sha1(b"blob "+str(len(b)).encode()+b"\0"+b).hexdigest()

class NativeDrowCatalog(unittest.TestCase):
    def test_previous_complete_source_is_preserved(self):
        self.assertEqual(blob(without_native_drow_stage(
            (SRC/"classes/drow/drow.c").read_text())), BASE)

    def test_mutations_remain_visible(self):
        text=(SRC/"classes/drow/drow.c").read_text()
        for old, new in (
            ("victim->hit = 30000;", "victim->hit = 99999;"),
            ("ch->pcdata->quest -= 400;", "ch->pcdata->quest -= 1;"),
            ("pObjIndex->weight < 0", "pObjIndex->weight < -1"),
            ("ch->pcdata->followers != 0", "ch->pcdata->followers > 10"),
        ):
            changed=text.replace(old,new)
            self.assertNotEqual(changed,text)
            try:
                restored=without_native_drow_stage(changed)
            except AssertionError:
                continue
            self.assertNotEqual(blob(restored),BASE)

    def test_real_world_suite_linked(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ("boot_db();", '"drowarmor %s"', '"prayer NativeDrow %s"',
                      "load_char_obj", "RT_TELEPORT|RT_RETURN",
                      "MOB_VNUM_GUARDIAN%MAX_KEY_HASH", "LONG_MAX"):
            self.assertIn(token,fixture)
        self.assertIn("tests/test_native_drow", make_values(ROOT,"TESTS"))
        self.assertIn("$(ENGINE_OBJECTS)", (ROOT/"Makefile").read_text())

if __name__ == "__main__":
    unittest.main()
