"""Native handler preservation and learned-Drow lesson coverage."""
import hashlib
import unittest
from test_class_layout import ROOT, SRC
from drow_night_source import without_drow_night_stage

BASE_CLAN = "ecd02d2802c2c10c26720c88493135342e6f73eb"

def blob(text):
    data = text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()

class DarkvisionCatalog(unittest.TestCase):
    def test_complete_native_source_is_preserved(self):
        self.assertEqual(blob(without_drow_night_stage(
            (SRC/"clan.c").read_text())), BASE_CLAN)

    def test_unreviewed_changes_still_fail(self):
        text = (SRC/"clan.c").read_text()
        for old, new in (("get_breed(ch, BREED_METIS) < 3",
                          "get_breed(ch, BREED_METIS) < 1"),
                         ("ch->pcdata->wolf > 0", "ch->pcdata->wolf > 999"),
                         ("SET_BIT(ch->vampaff, VAM_NIGHTSIGHT)",
                          "SET_BIT(ch->vampaff, VAM_CLAWS)")):
            changed = text.replace(old, new)
            self.assertNotEqual(changed, text)
            try:
                restored = without_drow_night_stage(changed)
            except AssertionError:
                continue
            self.assertNotEqual(blob(restored), BASE_CLAN)

    def test_native_price_and_real_runtime_tests(self):
        native = (SRC/"classes/drow/drow.c").read_text()
        self.assertIn('arg2, "darkvision")) {grant = DROW_NIGHT; cost = 5000;}', native)
        ui = (SRC/"classes/hybrid/drow_training.c").read_text()
        self.assertIn('{"darkvision",DROW_NIGHT,5000}', ui)
        fixture = (ROOT/"tests/test_hybrid_drow_training.c").read_text()
        for token in ("darkvision_lessons();", 'interpret(ch,"darkvision")',
                      'interpret(ch,"nightsight")', "load_char_short",
                      'utility_save_faults("drow darkvision learn",DROW_NIGHT,5000)'):
            self.assertIn(token, fixture)

if __name__ == "__main__":
    unittest.main()
