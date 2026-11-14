"""Identify safety checks supplement the real-engine native/output suite."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from identify_source import without_identify_stage

class IdentifyCatalog(unittest.TestCase):
    def test_only_identify_scope_changes_in_magic(self):
        text=without_identify_stage((SRC/"magic.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(text)).encode()+b"\0"+text).hexdigest(),
                         "ff6670721b45989e1e24b8c337f4e8ad766a8b18")

    def test_bounded_variable_text_and_missing_contexts(self):
        text=(SRC/"magic.c").read_text().split("/* Identification helpers are deliberately local;",1)[1].split("void spell_infravision(",1)[0]
        self.assertIn("vsnprintf(buf, size, format, args)",text)
        self.assertIn("size - 3",text)
        self.assertIn("hashes % 2 != 0",text)
        self.assertIn("ch == NULL || obj == NULL",text)
        self.assertIn("obj->pIndexData != NULL ?",text)
        self.assertNotIn("capitalize(obj->short_descr)",text)
        self.assertNotIn("sprintf(buf,\"%s",text)
        self.assertIn("itemtype < MAX_SKILL && skill_table[itemtype].name != NULL",text)

    def test_average_no_unchecked_sum(self):
        text=(SRC/"magic.c").read_text()
        self.assertIn("if ((low < 0) != (high < 0))",text)
        self.assertIn("low / 2 + high / 2 + (low % 2 + high % 2) / 2",text)
        self.assertNotIn("( obj->value[1] + obj->value[2] ) / 2",text)

    def test_engine_goldens_boundaries_and_build_wired(self):
        self.assertIn("tests/test_identify",make_values(ROOT,"TESTS"))
        text=(ROOT/"tests/test_identify.c").read_text()
        for token in ("rows==1248","1470866982027351726","LONG_MIN","LONG_MAX",
                      "__int128","--native-dump","PLR_ANSI","obj.pIndexData=NULL"):
            self.assertIn(token,text)
        self.assertIn("tests/test_identify_catalog.py",(ROOT/"Makefile").read_text())
        self.assertNotIn("void spell_identify",text)

    def test_scope_documented(self):
        text=(ROOT/"docs/MESSAGES.md").read_text()
        for token in ("1,248","169","cyclic","cost","identification"):
            self.assertIn(token,text)

if __name__=="__main__": unittest.main()
