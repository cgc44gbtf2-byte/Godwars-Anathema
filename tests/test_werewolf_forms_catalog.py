"""Legacy ranks and transformation policies are not expanded by caller guards."""
import hashlib
import json
from pathlib import Path
import re
import unittest
from werewolf_forms_source import without_werewolf_forms_stage

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/"Genesis/src/classes/werewolf/wwauspice.c"

class FormsCatalog(unittest.TestCase):
    def test_exact_prior_source_restored(self):
        raw=without_werewolf_forms_stage(SOURCE.read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest(),
                         "542d17a963ab2a3c53ae8e342d85575a73d75332")

    def test_unreviewed_edit_detected(self):
        text=SOURCE.read_text().replace("LONG_MAX - increase","LONG_MAX / increase")
        with self.assertRaises(AssertionError):without_werewolf_forms_stage(text)

    def test_source_scope_and_policy(self):
        source=SOURCE.read_text()
        from drow_globe_source import without_drow_globe_stage
        source=without_drow_globe_stage(source)
        def body(name):
            return re.search(r"void do_"+name+r"\s*\(.*?(?=\nvoid do_|\Z)",source,re.S)[0]
        wyrm=body("wyrmcall");song=body("songrage")
        self.assertIn("AUSPICE_GALLIARD) < 3",wyrm)
        self.assertIn("AUSPICE_GALLIARD) < 4",song)
        self.assertIn("combat_pair_in_room(room, ch, ich)",wyrm)
        self.assertIn("combat_pair_in_room(room, ch, victim)",song)
        for token in ("do_majesty","do_shadowbody","do_change"):
            self.assertIn(token,wyrm)
        for token in ("do_werewolf","do_beastlike","do_berserk"):
            self.assertIn(token,song)
        self.assertIn("LONG_MAX - increase",song)
        self.assertNotIn("hybrid_",wyrm+song)
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

if __name__=="__main__":unittest.main()
