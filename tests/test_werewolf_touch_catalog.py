"""Mother's Touch: bounded arithmetic, unchanged native permissions and source."""
import hashlib
import json
from pathlib import Path
import unittest
from werewolf_touch_source import without_werewolf_touch_stage

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/"Genesis/src/classes/werewolf/wwauspice.c"

class TouchCatalog(unittest.TestCase):
    def test_only_mother_and_limits_include_changed(self):
        raw=without_werewolf_touch_stage(SOURCE.read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest(),
                         "542d17a963ab2a3c53ae8e342d85575a73d75332")

    def test_inverse_rejects_unreviewed_arithmetic(self):
        text=SOURCE.read_text().replace("scaled * .03","scaled * .30")
        with self.assertRaises(AssertionError):without_werewolf_touch_stage(text)

    def test_native_policy_and_rounding_preserved(self):
        body=SOURCE.read_text().split("void do_mother(",1)[1].split("void do_pulse",1)[0]
        for token in ("!IS_WEREWOLF(ch)","rank < 1","AUSPICE_THEURGE) < 2",
                      "ch->pcdata->obeah = 15","WAIT_STATE(ch, 12)",
                      "LONG_MAX / rank","scaled * .03","(long)healed"):
            self.assertIn(token,body)
        self.assertNotIn("hybrid_",body)
        self.assertNotIn("mana -=",body)

    def test_compatibility_unchanged(self):
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

if __name__=="__main__":unittest.main()
