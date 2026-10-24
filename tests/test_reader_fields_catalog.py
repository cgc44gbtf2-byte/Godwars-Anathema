"""Narrow reader/factory change: pin every earlier source module exactly."""
import hashlib
import re
import unittest
from test_class_layout import ROOT,SRC,make_values
from reader_fields_source import without_reader_fields_stage
from test_guardian_lifecycle_catalog import without_hybrid_forms_save

def blob(text):
    b=text.encode()
    return hashlib.sha1(b"blob "+str(len(b)).encode()+b"\0"+b).hexdigest()

class ReaderFieldsCatalog(unittest.TestCase):
    def test_readme_is_a_short_overview_without_game_connection_details(self):
        text=(ROOT/"README.md").read_text()
        self.assertLess(len(text.splitlines()),160)
        self.assertNotRegex(text,r"\b(?:\d{1,3}\.){3}\d{1,3}\b")
        self.assertNotRegex(text.lower(),r"\b(?:host|port|endpoint|hostname)\b")
        for heading in ("## Classes and player changes","## Hybrid status",
                        "## Install and start","## Server management","## Guides",
                        "## Building and testing","## Credits and licensing"):
            self.assertIn(heading,text)
        self.assertIn("Players cannot convert into hybrids yet.",text)
        self.assertIn("LaWaylon (Luto)",text)
        for target in re.findall(r"\]\(([^)]+)\)",text):
            if not target.startswith(("https://","#")):
                self.assertTrue((ROOT/target.split("#",1)[0]).is_file(),target)

    def test_previous_modules_byte_exact(self):
        for path,expected in {
            "db.c":"303fb8cb50284af12ea8b74737750fb7dbf26c0b",
            "save.c":"00cac7a7d5bfe4cf99e8b9e24d970f487ce359ff",
            "interp.c":"ccb0125d4264a8a5447618c29a9240ec674d2eb4",
            "clan.c":"89d083ddce8367e98e58cec721cf290d9da105b2",
            "rename.c":"109f8ae08d341caa14f8eb5dae5749a96d8975de",
        }.items():
            with self.subTest(path=path):
                text=without_reader_fields_stage((SRC/path).read_text())
                if path=="save.c":text=without_hybrid_forms_save(text)
                self.assertEqual(blob(text),expected)

    def test_factory_integer_limits_have_exact_rng_contract(self):
        numbers=(SRC/"object_numbers.h").read_text()
        factory=(SRC/"db.c").read_text()
        test=(ROOT/"tests/test_reader_fields.c").read_text()
        for text in (numbers,factory):
            self.assertNotIn("rand(",text)
        for part in ("object_fuzzy_input(level)","object_cost_bounded(",
                     "if(number>INT_MIN)","if(number<INT_MAX)"):
            self.assertIn(part,factory)
        self.assertLess(factory.index("long cost_base=number_fuzzy(10)"),
                        factory.index("long cost_first=number_fuzzy("))
        self.assertLess(factory.index("long cost_first=number_fuzzy("),
                        factory.index("long cost_second=number_fuzzy("))
        for part in ("__builtin_mul_overflow","LONG_MAX","INT_MIN","INT_MAX"):
            self.assertIn(part,numbers)
        for part in ("--native-dump","--boundary","3024 level cases",
                     "2744 cost oracles","reader_errors();","fd_count()==before",
                     "read_error=1","deny_close=1","EACCES","ENOTDIR"):
            self.assertIn(part,test)
        self.assertIn("tests/test_reader_fields",make_values(ROOT,"TESTS"))

    def test_fail_closed_readers_and_nonfatal_reserve_management(self):
        save=(SRC/"save.c").read_text()
        io=(SRC/"file_io.h").read_text()
        for part in ("FILE *owned=fpReserve;","fpReserve=NULL;",
                     "if(owned!=NULL) fclose(owned);","file_reserve_close();",
                     "fpReserve=fopen(NULL_FILE,\"r\");"):
            self.assertIn(part,io)
        for part in ("else if(errno!=ENOENT)","found=TRUE;ch->pcdata->player_load_failed=1",
                     "if(ferror(fp)) ch->pcdata->player_load_failed=1",
                     "if(fclose(fp)!=0) ch->pcdata->player_load_failed=1",
                     "if(ferror(fp)) numbers_ok=FALSE",
                     "if(fclose(fp)!=0) numbers_ok=FALSE",
                     "Their playerfile could not be opened.",
                     "Their playerfile could not be read safely."):
            self.assertIn(part,save)
        self.assertEqual(save.count("else if(errno!=ENOENT)"),2)
        self.assertEqual(save.count("if(ferror(fp)) ch->pcdata->player_load_failed=1"),2)
        self.assertEqual(save.count("if(fclose(fp)!=0) ch->pcdata->player_load_failed=1"),2)
        self.assertIn("errno==ENOENT && check_parse_name(name)",
                      (SRC/"rename.c").read_text())
        for path in ("save.c","db.c","clan.c","rename.c","interp.c"):
            self.assertNotIn("fclose(fpReserve)",(SRC/path).read_text())
        self.assertIn('python3 tests/smoke_numbers.py',
                      (ROOT/".github/workflows/ci.yml").read_text())

if __name__=="__main__":
    unittest.main()
