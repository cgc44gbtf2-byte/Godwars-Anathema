"""Two native gift handlers only; preserve all earlier whole-source pins."""
import hashlib
import json
from pathlib import Path
import re
import unittest
from werewolf_reaction_source import without_werewolf_reaction_stage

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src/classes/werewolf/wwtribe.c"

def blob(text):
    d=text.encode()
    return hashlib.sha1(b"blob "+str(len(d)).encode()+b"\0"+d).hexdigest()

def body(text,name):
    return re.search(r"void do_"+name+r"\(.*?(?=\nvoid do_|\Z)",text,re.S)[0]

class WerewolfReactionsCatalog(unittest.TestCase):
    def test_inverse_restores_exact_8658ffd_module(self):
        self.assertEqual(blob(without_werewolf_reaction_stage(SRC.read_text())),
                         "a6f22b3547b3889b5b0df650918aa4056cfb1051")

    def test_future_edits_cannot_hide_in_inverse(self):
        text=SRC.read_text().replace(
            "damage = number_range(500,1500);","damage = number_range(500,1501);",1)
        with self.assertRaises(AssertionError):without_werewolf_reaction_stage(text)

    def test_target_and_cached_actor_guards(self):
        text=SRC.read_text()
        self.assertIn("combat_pair_in_room(cast_room, ch, victim)",body(text,"bodywrack"))
        clap=body(text,"clap")
        self.assertLess(clap.index("!combat_character_in_room(cast_room, victim)"),
                        clap.index("victim_next = victim->next_in_room"))
        self.assertIn("if (victim->hit <= 0 || victim->position == POS_DEAD) continue;",clap)

    def test_native_damage_and_waits_retained(self):
        text=SRC.read_text()
        for token in ("damage = number_range(500,1500)","WAIT_STATE( ch, 12 )",
                      "get_tribe(ch,TRIBE_BLACK_FURIES) < 5"):
            self.assertIn(token,body(text,"bodywrack"))
        for token in ("get_tribe(ch, TRIBE_SHADOW_LORDS) * number_range(50,100)",
                      "number_range(1,20) <= get_tribe(ch, TRIBE_SHADOW_LORDS)",
                      "WAIT_STATE( ch, 24 )"):
            self.assertIn(token,body(text,"clap"))

    def test_stun_feedback_goes_to_actual_victim(self):
        clap=body(SRC.read_text(),"clap")
        self.assertIn('send_to_char("You fall to the ground, stunned!\\n\\r",victim);',clap)
        self.assertNotIn('send_to_char("You fall to the ground, stunned!\\n\\r",ch);',clap)

    def test_real_native_engine_and_explicit_callback_seams(self):
        make=(ROOT/"Makefile").read_text()
        for token in ("TESTS += tests/test_werewolf_reactions",
                      "-Dkillperson=werewolf_test_killperson",
                      "-Ddo_kill=werewolf_test_do_kill"):
            self.assertIn(token,make)
        test=(ROOT/"tests/test_werewolf_reactions.c").read_text()
        for token in ("7350283483851042777","2466425606829114923","count==1216",
                      "do_kill(ch,argument)","killperson(ch,victim)",
                      "callback_case(TRUE,TRUE,4)","stun_feedback()","entry_guards()"):
            self.assertIn(token,test)

    def test_save_and_handoff_unchanged(self):
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

    def test_scope_and_callback_limit_documented(self):
        text=(ROOT/"docs/WEREWOLF.md").read_text()
        for token in ("Body Wrack and Clap","1,216","210","test-only",
                      "protected-target","No new power"):
            self.assertIn(token,text)

if __name__=="__main__":unittest.main()
