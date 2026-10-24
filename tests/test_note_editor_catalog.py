"""Keep draft lifetime fixes isolated from all previous engine behavior."""
import hashlib
from pathlib import Path
import unittest
from note_editor_source import without_note_editor_stage
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src"
class NoteEditorCatalog(unittest.TestCase):
    def test_previous_whole_modules_restored(self):
        for name,sha in (
            ("board.c","31048c14f9e4cc5dce765fcafd4f40b3de793a3d"),
            ("db.c","46ce49b905c599dc11ee841fbae735407e325405"),
        ):
            data=without_note_editor_stage((SRC/name).read_text()).encode()
            self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),sha)
    def test_capacity_checked_before_owned_text_is_freed(self):
        text=(SRC/"board.c").read_text().split("void handle_con_note_text",1)[1]
        self.assertLess(text.index("MAX_NOTE_TEXT - used"),text.index("free_string"))
        self.assertIn("added + 2",text)
        self.assertNotIn("strlen2(",text)
    def test_post_revalidates_access_before_transfer(self):
        text=(SRC/"board.c").read_text().split("case 'p': /* post note */",1)[1]
        self.assertLess(text.index("write_level"),text.index("finish_note"))
        self.assertIn("Your draft has been kept",text)
    def test_cleanup_clears_owner_before_recycling(self):
        text=(SRC/"db.c").read_text().split("void free_char",1)[1]
        self.assertLess(text.index("in_progress = NULL"),text.index("pcdata_free"))
    def test_real_engine_suite_registered(self):
        make=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_note_editor",make)
        self.assertIn("python3 tests/test_note_editor_catalog.py",make)
        test=(ROOT/"tests/test_note_editor.c").read_text()
        for token in ("close_socket2","close_socket(d)","SAVE_FAULT_RENAME",
                      "CON_NOTE_TO","CON_NOTE_FINISH","extract_char","native_flow"):
            self.assertIn(token,test)
if __name__=="__main__":unittest.main()
