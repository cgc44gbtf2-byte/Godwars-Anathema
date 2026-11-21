"""Numeric-reader scope, real-engine coverage and prior native pins."""
import hashlib
import unittest
from test_class_layout import ROOT,SRC,make_values
from file_numbers_source import without_file_number_db,without_file_number_save
from test_guardian_lifecycle_catalog import without_hybrid_forms_save


def blob(text):
    data=text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()


class FileNumbersCatalog(unittest.TestCase):
    def test_all_other_database_and_save_source_remains_pinned(self):
        self.assertEqual(blob(without_file_number_db((SRC/"db.c").read_text())),
                         "062398a86ec90e64c0d94e49ce34a4bae9da046d")
        self.assertEqual(blob(without_hybrid_forms_save(
            without_file_number_save((SRC/"save.c").read_text()))),
                         "3655e4f99b37b0d5165e58dc73d78c00b47564fc")

    def test_checked_pure_bounded_nonrecursive_arithmetic(self):
        text=(SRC/"file_number.c").read_text()
        for part in ("int c, negative","unsigned char",
                     "__builtin_mul_overflow","__builtin_sub_overflow",
                     "__builtin_add_overflow","terms[--count]",
                     "if(ferror(fp))","ungetc(c,fp)==EOF","*result=value"):
            self.assertIn(part,text)
        self.assertEqual(text.count("fread_number_checked("),1)
        self.assertNotRegex(text,r"\b(?:malloc|calloc|realloc|exit|bug|send_to_char)\s*\(")
        self.assertNotRegex(text,r"\bnumber_(?:bits|range|percent|fuzzy)\s*\(")
        header=(SRC/"file_number.h").read_text()
        for part in ("FILE_NUMBER_MAX_TERMS 1024","FILE_NUMBER_MAX_BYTES 65536",
                     "FILE_NUMBER_FORMAT","FILE_NUMBER_RANGE","FILE_NUMBER_IO",
                     "FILE_NUMBER_LIMIT"):
            self.assertIn(part,header)

    def test_local_player_failure_not_fatal_boot_policy(self):
        text=(SRC/"save.c").read_text()
        for part in ("static long saved_number(FILE *fp, bool *ok)",
                     "if (*ok && fread_number_checked(fp,&number)!=FILE_NUMBER_OK)",
                     "#define fread_number(fp) saved_number((fp),&numbers_ok)",
                     "#undef fread_number","discard_saved_object(obj)",
                     "num<0 || num>1024","prototype=get_obj_index(vnum)",
                     'Their playerfile could not be read safely.'):
            self.assertIn(part,text)
        self.assertEqual(text.count("bool numbers_ok=TRUE;"),4)
        self.assertEqual(text.count("if (!numbers_ok) continue;"),2)
        body=(SRC/"db.c").read_text().split("long fread_number(",1)[1].split("char *fread_string(",1)[0]
        self.assertIn("fread_number_checked(fp,&number)",body)
        self.assertIn("exit(1)",body)
        self.assertNotIn("number = number * 10",body)

    def test_shared_production_and_engine_linkage(self):
        self.assertIn("file_number.o",make_values(SRC,"O_FILES"))
        self.assertIn("tests/test_file_numbers",make_values(ROOT,"TESTS"))
        self.assertIn("$(ENGINE_OBJECTS)",(ROOT/"Makefile").read_text().split(
            "tests/test_file_numbers:",1)[1].split("\n\n",1)[0])
        workflow=(ROOT/".github/workflows/ci.yml").read_text()
        self.assertEqual(workflow.count("python3 tests/smoke_numbers.py"),2)

    def test_real_engine_oracles_snapshots_and_rejection_cases(self):
        text=(ROOT/"tests/test_file_numbers.c").read_text()
        for part in ("rows==3042","612136340295879720","__int128 tail=",
                     "fopencookie","for(int c=0;c<256;c++)",
                     "fread_number_checked(fp,&value)","load_leaderboard()",
                     "load_char_short","load_char_obj","do_finger(&actor,name)",
                     '"Room -1"','"Vnum -1"',"LONG_MAX","LONG_MIN"):
            self.assertIn(part,text)
        self.assertNotRegex(text,r"\b(?:long|int)\s+fread_number\s*\(")

    def test_real_tcp_failures_preserve_files_and_another_player(self):
        text=(ROOT/"tests/smoke_numbers.py").read_text()
        for part in ("stage_world(REPO, runtime)","tempfile.TemporaryDirectory",
                     "subprocess.Popen","bad_short","bad_full",
                     "player.read_bytes() == bad_short",
                     "player.read_bytes() == bad_full",
                     'until(control, b"Generation:")',
                     'player.write_bytes(original)',"signal.SIGTERM",
                     "process.wait(timeout=15) == 0","secrets.token_urlsafe"):
            self.assertIn(part,text)
        self.assertNotIn("137.184.145.164",text)


if __name__=="__main__":
    unittest.main()
