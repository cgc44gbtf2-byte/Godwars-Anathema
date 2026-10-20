"""Atomic auxiliary writers retain old source pins and main-loop retry wiring."""
import hashlib
from pathlib import Path
import unittest
from aux_writers_source import without_aux_writers_stage, without_aux_writers_update
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src"
class AuxWritersCatalog(unittest.TestCase):
    def test_exact_prior_leaderboard_module(self):
        text=without_aux_writers_stage((SRC/"random.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(text)).encode()+b"\0"+text).hexdigest(),
                         "ef9e430bbe7a626c5d8c9e1e13cbd4bd866bf245")
    def test_no_in_place_board_or_leaderboard_writer(self):
        board=(SRC/"board.c").read_text()
        writer=board.split("static int append_note",1)[1].split("/* Find the number",1)[0]
        self.assertNotIn("fopen",writer)
        self.assertIn("atomic_save(NOTE_DIR",writer)
        leader=(SRC/"random.c").read_text()
        writer=leader.split("/* Atomic leaderboard writer scope. */",1)[1].split(
            "/* End atomic leaderboard writer scope. */",1)[0]
        self.assertNotIn("fopen",writer)
        self.assertIn("atomic_save_leaderboard",writer)
    def test_entire_prior_update_module_unchanged_except_retry_hook(self):
        text=without_aux_writers_update((SRC/"update.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(text)).encode()+b"\0"+text).hexdigest(),
                         "7ec9814a41d5f13dacf3a8c57ee3faa0b33b9270")
    def test_fixed_name_api_and_normal_name_validation(self):
        atomic=(SRC/"atomic_save.c").read_text()
        self.assertIn('atomic_save_named(directory,"leader.txt",writer,context,error,1)',atomic)
        self.assertIn("atomic_save_name_valid(name)",atomic)
    def test_real_pulse_wiring_and_default_suite(self):
        update=(SRC/"update.c").read_text().split("void update_handler( void )",1)[1]
        self.assertIn("save_notes();",update)
        self.assertIn("retry_leaderboard_save();",update)
        make=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_aux_writers",make)
        self.assertIn("python3 tests/test_aux_writers_catalog.py",make)
if __name__=="__main__":unittest.main()
