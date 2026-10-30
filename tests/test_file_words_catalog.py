"""Keep the shared parser and scoped session wiring covered in all CI jobs."""
import hashlib
from pathlib import Path
import unittest
from word_readers_source import without_word_readers_stage
from test_guardian_lifecycle_catalog import without_hybrid_forms_save
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src"
def blob(text):
    data=text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()
class WordReadersCatalog(unittest.TestCase):
    def test_previous_modules_are_restored_byte_for_byte(self):
        for path,expected in (
            ("db.c","a910708ed3c89e52190fdf0ccb85e8a05f708bfa"),
            ("save.c","120404c717d1641d19aaf4c8b4b207f683f44ae6"),
        ):
            text=without_word_readers_stage((SRC/path).read_text())
            if path=="save.c":text=without_hybrid_forms_save(text)
            self.assertEqual(blob(text),expected)
    def test_real_parser_and_explicit_session_error_policy(self):
        parser=(SRC/"file_word.c").read_text()
        save=(SRC/"save.c").read_text()
        for token in ("FILE_WORD_MAX_READS","isspace((unsigned char)c)",
                      "FILE_WORD_FORMAT","FILE_WORD_IO","ungetc(c,fp)",
                      "word[0]='\\0';"):
            self.assertIn(token,parser)
        self.assertNotIn("exit(",parser)
        self.assertNotIn("malloc(",parser)
        self.assertIn("if (ferror(fp)) return FILE_WORD_IO;",parser)
        self.assertIn("saved_word(fp,&section_ok)",save)
        self.assertIn("#define fread_word(fp) saved_word((fp),&numbers_ok)",save)
        self.assertEqual(save.count("if (!numbers_ok) { continue; }"),3)
        self.assertNotIn('feof( fp ) ? "End" : fread_word( fp )',save)
        self.assertIn("file_word.o",(SRC/"Makefile").read_text())
    def test_regression_cases_are_in_ci(self):
        make=(ROOT/"Makefile").read_text()
        suite=(ROOT/"tests/test_file_words.c").read_text()
        self.assertIn("TESTS += tests/test_file_words",make)
        self.assertIn("python3 tests/test_file_words_catalog.py",make)
        for token in ("--boundary","--native-dump","word_boundaries();",
                      "FILE_WORD_MAX_READS","FILE_WORD_IO","live_words();",
                      "save_char_obj_checked","fopencookie"):
            self.assertIn(token,suite)
        self.assertEqual((ROOT/".github/workflows/ci.yml").read_text().count(
            "python3 tests/smoke_words.py"),2)
if __name__=="__main__":unittest.main()
