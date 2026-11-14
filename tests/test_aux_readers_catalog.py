"""Record boundaries, write quarantine and exact prior leaderboard source."""
import hashlib
from pathlib import Path
import unittest
from aux_readers_source import without_aux_readers_stage
ROOT=Path(__file__).resolve().parents[1]
class AuxReadersCatalog(unittest.TestCase):
    def test_previous_leaderboard_module_restored(self):
        data=without_aux_readers_stage((ROOT/"Genesis/src/random.c").read_text()).encode()
        sha=hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()
        self.assertEqual(sha,"6baab743cdeb0b04c90ae7a72afa77912d940863")
    def test_stage_then_commit_and_quarantine(self):
        board=(ROOT/"Genesis/src/board.c").read_text()
        reader=board.split("static bool load_board (",1)[1].split("/* Initialize",1)[0]
        self.assertNotIn("alloc_perm(",reader)
        for primitive in ("fread_word_checked","fread_number_checked","board_string"):
            self.assertIn(primitive,reader)
        self.assertLess(reader.index("if(fclose(fp)!=0)"),reader.index("board->note_first=first"))
        self.assertIn("boards[i].load_failed = !load_board",board)
        self.assertIn("Your draft has been kept",board)
        self.assertIn("char buf[5*MAX_STRING_LENGTH]",board)
        leader=(ROOT/"Genesis/src/random.c").read_text()
        reader=leader.split("void load_leaderboard()",1)[1].split("void save_leaderboard()",1)[0]
        self.assertLess(reader.index("if(fclose(fp)!=0)"),reader.index("leader_board=staged"))
        self.assertIn("fread_word_checked(fp,extra",reader)
        self.assertIn("free_string(*old_names[i])",reader)
    def test_default_real_engine_suite(self):
        make=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_aux_readers",make)
        self.assertIn("python3 tests/test_aux_readers_catalog.py",make)
        self.assertEqual((ROOT/".github/workflows/ci.yml").read_text().count(
            "python3 tests/smoke_aux_readers.py"),2)
        suite=(ROOT/"tests/test_aux_readers.c").read_text()
        for token in ("aux_rows==168","--native-dump","--boundary","aux_file_hash",
                      "load_failed","save_leaderboard"):
            self.assertIn(token,suite)
if __name__=="__main__":unittest.main()
