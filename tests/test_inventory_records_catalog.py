"""Structural inventory checks must preserve the complete earlier reader."""
import hashlib
from pathlib import Path
import unittest
from inventory_records_source import without_inventory_records_stage
from test_guardian_lifecycle_catalog import without_hybrid_forms_save
ROOT=Path(__file__).resolve().parents[1]
class InventoryRecordsCatalog(unittest.TestCase):
    def test_previous_reader_restored_byte_for_byte(self):
        current=(ROOT/"Genesis/src/save.c").read_text()
        restored=without_hybrid_forms_save(without_inventory_records_stage(current)).encode()
        baseline="128c3a902f4b9e02ec9fe7b632ddf4b8d085d541"
        self.assertEqual(hashlib.sha1(b"blob "+str(len(restored)).encode()+b"\0"+restored).hexdigest(),baseline)
    def test_checks_precede_publication_and_clear_descendants(self):
        source=(ROOT/"Genesis/src/save.c").read_text()
        full=source[source.index("void fread_obj("):source.index("void fread_tiny_obj(")]
        tiny=source[source.index("void fread_tiny_obj("):source.index("void do_finger(")]
        for reader in (full,tiny):
            self.assertIn("iNest>0 && rgObjNest[iNest-1]==NULL",reader)
            self.assertIn("n=(unsigned)iNest+1;n<MAX_NEST;n++",reader)
        self.assertIn("if ( fNest || iNest < 0",full)
        self.assertIn("if (fVnum) { numbers_ok=FALSE; break; }",full)
        self.assertNotIn("rgObjNest[iNest] = obj;",full)
        self.assertLess(full.index("!fNest || !fVnum"),full.index("rgObjNest[iNest]=obj;"))
        self.assertLess(tiny.index("if (!numbers_ok)"),tiny.index("rgObjNest[iNest]=obj;"))
    def test_real_engine_regressions_are_in_default_suite(self):
        make=(ROOT/"Makefile").read_text()
        for token in ("TESTS += tests/test_inventory_records",
                      "python3 tests/test_inventory_records_catalog.py"):
            self.assertIn(token,make)
        suite=(ROOT/"tests/test_inventory_records.c").read_text()
        for token in ("--native-dump","--boundary","save_char_obj_checked",
                      "inventory_rows==276","alarm(3)","ExtraDescr first~"):
            self.assertIn(token,suite)
        self.assertEqual((ROOT/".github/workflows/ci.yml").read_text().count(
            "python3 tests/smoke_inventory.py"),2)
if __name__=="__main__":unittest.main()
