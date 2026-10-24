"""Whole-module old-source pins plus bounded terminal checkpoint ownership."""
import hashlib
from pathlib import Path
import unittest
from aux_shutdown_source import without_aux_shutdown_stage
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src"

class AuxiliaryShutdownCatalog(unittest.TestCase):
    def test_only_exact_checkpoint_scopes_changed_in_old_modules(self):
        expected={
            "act_wiz.c":"b4c1337c212091ef407f036c76ac55b15a843c20",
            "board.c":"421db60f9f197872248548df0d49d804cc3c493f",
            "comm.c":"60ac79ba259797aac5eef47c27bd8d6840aefdb0",
            "random.c":"4f49a4d41b799b45e159a82e3b6018a68aeccc70",
            "restart.c":"df0269a1ce737d2353a6db9492322ec1b2c52e25",
            "auxiliary_save.h":"a4add7550e45ab2646fe525a0140369489256d44",
        }
        for path,sha in expected.items():
            body=without_aux_shutdown_stage((SRC/path).read_text()).encode()
            self.assertEqual(hashlib.sha1(b"blob "+str(len(body)).encode()+b"\0"+body).hexdigest(),sha,path)

    def test_both_destinations_are_attempted_without_loop_or_short_circuit(self):
        text=(SRC/"auxiliary_save.c").read_text()
        self.assertIn("bool notes=flush_notes();",text)
        self.assertIn("bool leaderboard=flush_leaderboard();",text)
        self.assertIn("return notes&&leaderboard;",text)
        self.assertNotIn("while",text)
        board=(SRC/"board.c").read_text().split("bool flush_notes(void)",1)[1].split("/* End forced",1)[0]
        self.assertIn("notes_retry_after=0;",board)
        self.assertIn("boards[i].changed && !save_board",board)
        leader=(SRC/"random.c").read_text().split("bool flush_leaderboard(void)",1)[1].split("/* End forced",1)[0]
        self.assertIn("if(!leaderboard_save_pending)return TRUE;",leader)
        self.assertIn("if(leaderboard_load_failed)return FALSE;",leader)

    def test_no_io_in_handler_and_no_manual_teardown_before_checkpoint(self):
        signals=(SRC/"signals.c").read_text()
        self.assertNotIn("auxiliary_save_checkpoint",signals)
        signal=(SRC/"comm.c").read_text().split("static void shutdown_from_signal",1)[1].split("void game_loop_unix",1)[0]
        self.assertLess(signal.index("auxiliary_save_checkpoint()"),signal.index("close_socket("))
        shutdown=(SRC/"act_wiz.c").read_text().split("void do_shutdown(",1)[1].split("void do_snoop",1)[0]
        self.assertLess(shutdown.index("auxiliary_save_checkpoint()"),shutdown.index("append_file("))
        restart=(SRC/"restart.c").read_text().split("void restart_copyover(",1)[1].split("static bool read_handoff",1)[0]
        self.assertLess(restart.index("auxiliary_save_checkpoint()"),restart.index("add_flags("))

    def test_suite_and_production_module_registered(self):
        make=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_aux_shutdown",make)
        self.assertIn("python3 tests/test_aux_shutdown_catalog.py",make)
        self.assertIn("auxiliary_save.o",(SRC/"Makefile").read_text())
        test=(ROOT/"tests/test_aux_shutdown.c").read_text()
        for marker in ("manual_shutdown","copyover_pending","game_loop_unix(-1)",
                       "SAVE_FAULT_DIRSYNC","load_failed=TRUE","load_leaderboard();"):
            self.assertIn(marker,test)

if __name__=="__main__":unittest.main()
