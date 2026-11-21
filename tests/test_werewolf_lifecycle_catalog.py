"""Native gates and entire shared-module source pin remain independently checked."""
import hashlib
import json
from pathlib import Path
import unittest
from werewolf_lifecycle_source import without_werewolf_lifecycle_stage

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/"Genesis/src/clan.c"
class LifecycleCatalog(unittest.TestCase):
    def test_previous_shared_module_exact(self):
        before=without_werewolf_lifecycle_stage(SOURCE.read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(before)).encode()+b"\0"+before).hexdigest(),
                         "b85b32221fa0a08a49c850b7605c9e49c14434c4")

    def test_unreviewed_edit_rejected(self):
        changed=SOURCE.read_text().replace("strike < 3","strike < 4")
        with self.assertRaises(AssertionError):without_werewolf_lifecycle_stage(changed)

    def test_triple_attack_contract_and_schema(self):
        text=SOURCE.read_text().split("void do_werewolf(",1)[1].split("\nvoid do_unwerewolf(",1)[0]
        for part in ("strike < 3","multi_hit(ch, vch)","combat_pair_in_room(room, ch, vch)",
                     "combat_character_in_room(room, ch)","combat_character_in_room(room, vch)",
                     "IS_COMBAT_PAUSED(ch)","PLR_WOLFMAN","LONG_MAX - 25",
                     "for (member = char_list; member != NULL && member != vch_next"):
            self.assertIn(part,text)
        self.assertNotIn("hybrid_",text)
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

if __name__=="__main__":unittest.main()
