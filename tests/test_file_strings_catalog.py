"""Checked runtime strings must not change the native boot reader or old pins."""
import hashlib
from pathlib import Path
import unittest
from string_readers_source import without_string_readers_stage
from test_guardian_lifecycle_catalog import without_hybrid_forms_save
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src"
def blob(text):
    data=text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()
class StringReadersCatalog(unittest.TestCase):
    def test_previous_modules_are_restored_byte_for_byte(self):
        for path,expected in (
            ("db.c","e5240dc752bdac278be07a9fc5e929e19fb31579"),
            ("save.c","616bdd8a655425e1ea104f04181e9cada9162fba"),
        ):
            text=without_string_readers_stage((SRC/path).read_text())
            if path=="save.c":text=without_hybrid_forms_save(text)
            self.assertEqual(blob(text),expected)
    def test_checked_parser_and_scoped_session_wiring(self):
        parser=(SRC/"file_string.c").read_text()
        save=(SRC/"save.c").read_text()
        for token in ("FILE_STRING_MAX_READS","isspace((unsigned char)c)",
                      "FILE_STRING_FORMAT","FILE_STRING_LIMIT","FILE_STRING_IO",
                      "if(ferror(fp))return FILE_STRING_IO;"):
            self.assertIn(token,parser)
        self.assertNotIn("exit(",parser)
        self.assertNotIn("malloc(",parser)
        self.assertIn("#define fread_string(fp) saved_string((fp),&numbers_ok)",save)
        self.assertIn("#undef fread_string",save)
        self.assertIn("if(victim==NULL)",save)
        self.assertIn("file_string.o",(SRC/"Makefile").read_text())
    def test_real_engine_and_tcp_checks_run_in_ci(self):
        make=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_file_strings",make)
        self.assertIn("python3 tests/test_file_strings_catalog.py",make)
        suite=(ROOT/"tests/test_file_strings.c").read_text()
        for token in ("--native-dump","--boundary","fread_string_checked",
                      "FILE_STRING_MAX_READS","string_bad(kind,shorter)",
                      "save_char_obj_checked","fopencookie","fBootDb=TRUE"):
            self.assertIn(token,suite)
        self.assertEqual((ROOT/".github/workflows/ci.yml").read_text().count(
            "python3 tests/smoke_strings.py"),2)
if __name__=="__main__":unittest.main()
