"""Reader boundaries, exact old-module pins and startup fail-closed wiring."""
import hashlib
from pathlib import Path
import unittest
from admin_config_source import without_admin_config_stage
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src"
class AdminConfigCatalog(unittest.TestCase):
    def test_previous_modules_exactly_restored(self):
        for name,expected in (("interp","775d6ed9c481fe2d343bec1032fa1cd3a575053e"),
                              ("act_wiz","74910b6319b72e474180f973c0e03ea3abd0b9bd"),
                              ("db","32c8947c50c6d150bf16da28a354ca35d7518a46")):
            with self.subTest(name=name):
                data=without_admin_config_stage((SRC/(name+".c")).read_text()).encode()
                self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),expected)
    def test_bounded_nonfatal_readers(self):
        text=(SRC/"admin_config.c").read_text()
        for required in ("fread_word_checked","fread_number_checked","ADMIN_RECORD_LIMIT",
                         "ADMIN_FILE_LIMIT","O_NONBLOCK","O_NOFOLLOW","S_ISREG"):
            self.assertIn(required,text)
        for old in ("fread_word(","fread_number(","fread_to_eol(","exit("):
            self.assertNotIn(old,text)
    def test_reload_and_write_guards(self):
        text=(SRC/"interp.c").read_text()
        writer=(SRC/"admin_config.c").read_text()
        self.assertIn("admin_save_disabled()",text.split("void save_disabled()",1)[1])
        self.assertIn("admin_save_bans()",text.split("void save_bans(void)",1)[1])
        self.assertIn("if(!disabled_ready || !serializable_disabled())",writer)
        self.assertIn("if(!bans_ready || !serializable_bans())",writer)
        self.assertEqual((SRC/"act_wiz.c").read_text().count("if(!admin_bans_ready()){"),2)
    def test_startup_checks_both_configs(self):
        text=(SRC/"db.c").read_text().split("load_disabled( );",1)[1]
        self.assertLess(text.index("load_bans( );"),text.index("admin_configuration_healthy()"))
        self.assertIn("exit(1)",text.split("admin_configuration_healthy()",1)[1].split("load_leaderboard",1)[0])
    def test_production_and_default_tests(self):
        self.assertIn("admin_config.o",(SRC/"Makefile").read_text())
        text=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_admin_config",text)
        self.assertIn("python3 tests/test_admin_config_catalog.py",text)
    def test_commands_cannot_outgrow_reader_limits(self):
        self.assertIn("admin_ban_can_add(arg)",(SRC/"act_wiz.c").read_text())
        self.assertIn("admin_disabled_can_add(&cmd_table[i],ch->name,get_trust(ch))",
                      (SRC/"interp.c").read_text())
        text=(SRC/"admin_config.c").read_text()
        self.assertIn("charge(&bytes,strlen(p->name)+1)",text)
        self.assertIn("++records>ADMIN_RECORD_LIMIT",text)
if __name__=="__main__":unittest.main()
