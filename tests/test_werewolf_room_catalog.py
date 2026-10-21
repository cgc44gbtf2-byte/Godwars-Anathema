"""Native room effects only; exact source scope plus real-engine contracts."""
import hashlib
import json
from pathlib import Path
import re
import unittest
from werewolf_room_source import without_werewolf_room_stage

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src/classes/werewolf/wwtribe.c"

def body(text,name):
    return re.search(r"void do_"+name+r"\(.*?(?=\nvoid do_|\Z)",text,re.S)[0]

class WerewolfRoomCatalog(unittest.TestCase):
    def test_inverse_restores_whole_parent_module(self):
        raw=without_werewolf_room_stage(SRC.read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest(),
                         "a63829e9f289737f0541149d9e661699ea677fcd")

    def test_future_body_edits_cannot_hide(self):
        text=SRC.read_text().replace("number_range(200,400)","number_range(201,400)",1)
        with self.assertRaises(AssertionError):without_werewolf_room_stage(text)

    def test_no_new_access_or_costs(self):
        text=SRC.read_text()
        for name,gate in (("breathwyld","TRIBE_BLACK_FURIES) < 1"),
                          ("banshee","TRIBE_FIANNA) < 2"),
                          ("obedience","TRIBE_SHADOW_LORDS) < 5")):
            source=body(text,name)
            self.assertIn(gate,source)
            self.assertIn("IS_WEREWOLF(ch)",source)
            self.assertNotIn("hybrid_",source)
            self.assertNotIn("mana -=",source)
            self.assertNotIn("practice -=",source)

    def test_cached_membership_before_next_read(self):
        text=SRC.read_text()
        for name,pointer,next_read in (
            ("banshee","ich","fch = ich->next_in_room"),
            ("obedience","victim","vch_next = victim->next_in_room")):
            source=body(text,name)
            self.assertLess(source.index("!combat_character_in_room(cast_room, "+pointer+")"),
                            source.index(next_read))
        self.assertIn("if (!combat_character_in_room(cast_room, ich)) continue;",
                      body(text,"banshee"))

    def test_global_cleanup_surviving_moved_characters(self):
        text=SRC.read_text()
        self.assertIn("for (live = char_list;",body(text,"banshee"))
        obey=body(text,"obedience")
        self.assertEqual(obey.count("for (live = char_list;"),2)
        self.assertEqual(obey.count("if (IS_NPC(live)) REMOVE_BIT(live->act, ACT_COMMANDED);"),2)
        self.assertIn("strlen(argument) >= sizeof action",obey)
        self.assertEqual(obey.count("snprintf( buf, sizeof buf,"),4)
        self.assertNotIn("sprintf(",obey)

    def test_healing_is_bounded_without_changing_native_formula(self):
        source=body(SRC.read_text(),"breathwyld")
        for token in ("number_range(200,400)","500 * ch->pcdata->rank",
                      "LONG_MAX - heal","(LONG_MAX - 2000) / 500",
                      "vch->hit = vch->max_hit","WAIT_STATE(ch,12)"):
            self.assertIn(token,source)

    def test_real_engine_and_fixed_native_hash(self):
        make=(ROOT/"Makefile").read_text()
        for token in ("TESTS += tests/test_werewolf_room",
                      "-Ddo_flee=werewolf_room_flee","-Dinterpret=werewolf_room_interpret"):
            self.assertIn(token,make)
        self.assertIn("tests/room_tribe.o",make.split("\nclean:",1)[1])
        test=(ROOT/"tests/test_werewolf_room.c").read_text()
        for token in ("count==912","16022392963781972845",
                      "do_flee(ch,argument)","interpret(ch,argument)",
                      "long_command(TRUE)","callback_case(TRUE,action,TRUE)"):
            self.assertIn(token,test)

    def test_compatibility_and_limitations(self):
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})
        text=(ROOT/"docs/WEREWOLF.md").read_text()
        for token in ("912","Obedience","Breath of the Wyld",
                      "test-only","Wrath of Gaia","Call of the Wyrm"):
            self.assertIn(token,text)

if __name__=="__main__":unittest.main()
