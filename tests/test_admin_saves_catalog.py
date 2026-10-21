"""Fixed-name admin persistence and exact preservation of older source pins."""
import hashlib
from pathlib import Path
import unittest
from admin_saves_source import without_admin_saves_stage
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src"
class AdminSavesCatalog(unittest.TestCase):
    def test_previous_sources_exactly_restored(self):
        pins={
            "interp.c":"3daaa41f1805244dc110fa9eaebafb967f4f7677",
            "act_wiz.c":"bfd0c899650fadb6aa6f73f28473b34f61c4099a",
            "admin_config.c":"5e1534a37158928531cf1d6d41fdfe48bf4d5263",
            "admin_config.h":"1e5061a5c00acd9c8d401c5fbd6ec586b06cf7e5",
            "atomic_save.c":"61db3d70cc8b0418a046ec22cd41e651ee78577f",
            "atomic_save.h":"1d588b2028a823074ae69f920929f79cbb767fb1",
        }
        for name,expected in pins.items():
            with self.subTest(name=name):
                data=without_admin_saves_stage((SRC/name).read_text()).encode()
                self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),expected)
    def test_fixed_names_and_checked_deletion(self):
        code=(SRC/"atomic_save.c").read_text().split("/* Administrative policy fixed-name atomic APIs. */",1)[1]
        for name in ("ban.txt","disabled.txt"):
            self.assertIn(f'atomic_save_named(directory,"{name}",writer,context,error,1)',code)
            self.assertIn(f'atomic_remove_named(directory,"{name}",error)',code)
        for check in ("O_NOFOLLOW","O_DIRECTORY","AT_SYMLINK_NOFOLLOW",
                      "S_ISREG","geteuid()","unlinkat(dirfd,name,0)","fsync(dirfd)"):
            self.assertIn(check,code)
    def test_staff_actions_handle_failure_before_success(self):
        interp=(SRC/"interp.c").read_text()
        wiz=(SRC/"act_wiz.c").read_text()
        for text,kind in ((interp,"disabled-command"),(wiz,"ban")):
            for action in ("addition","removal"):
                part=text.split(f"/* Atomic {kind} {action}. */",1)[1].split(f"/* End atomic {kind} {action}. */",1)[0]
                self.assertIn("ATOMIC_SAVE_FAILED",part)
                self.assertIn("ATOMIC_SAVE_UNCERTAIN",part)
                self.assertIn("previous restrictions remain",part)
                self.assertIn("paused until restart",part)
    def test_legacy_entry_points_no_longer_truncate(self):
        text=(SRC/"interp.c").read_text().split("/* Save disabled commands */",1)[1]
        self.assertNotIn("fopen",text)
        self.assertNotIn("unlink",text)
        self.assertIn("admin_save_bans()",text)
        self.assertIn("admin_save_disabled()",text)
    def test_uncertain_commits_quarantine_and_restore_reserve(self):
        text=(SRC/"admin_config.c").read_text().split("/* Atomic policy save implementations.",1)[1]
        for name in ("bans","disabled"):
            part=text.split(f"enum atomic_save_result admin_save_{name}(void)",1)[1]
            self.assertIn(f"if(result==ATOMIC_SAVE_UNCERTAIN){name}_ready=FALSE",part)
            self.assertIn("file_reserve_close();",part)
            self.assertIn("file_reserve_open();",part)
    def test_real_engine_suite_and_unreadable_fixture_is_not_resaved(self):
        make=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_admin_saves",make)
        self.assertIn("python3 tests/test_admin_saves_catalog.py",make)
        unit=(ROOT/"tests/test_admin_saves.c").read_text()
        for case in ("RLIMIT_FSIZE","SAVE_FAULT_REMOVE","SAVE_FAULT_DIRSYNC",
                     "S_ISLNK","0770","fds()==count"):
            self.assertIn(case,unit)
        reader=(ROOT/"tests/test_reader_fields.c").read_text()
        self.assertIn('ban.name="private.example"',reader)
if __name__=="__main__":unittest.main()
