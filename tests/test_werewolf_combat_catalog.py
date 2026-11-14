"""Narrow native gift callback repair; no foreign powers or schema expansion."""
import hashlib
import json
from pathlib import Path
import re
import unittest
from werewolf_combat_source import without_werewolf_combat_stage
from werewolf_reaction_source import without_werewolf_reaction_stage

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src/classes/werewolf"

def blob(text):
    raw=text.encode()
    return hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()

def body(text,name):
    match=re.search(r"void do_"+name+r"\(.*?(?=\nvoid do_|\Z)",text,re.S)
    if not match:raise AssertionError(name)
    return match[0]

class WerewolfCombatCatalog(unittest.TestCase):
    def test_inverse_edits_preserve_full_historical_source_pins(self):
        for file,expected in (
            ("wwbreed.c","9ba911553359dbdc556e33d1382a96eaf306f6a3"),
            ("wwtribe.c","f7528ddc81ca95188d6c621ccccff62b97d457e1"),
        ):
            text=without_werewolf_combat_stage((SRC/file).read_text())
            self.assertEqual(blob(text),expected)
        changed=(SRC/"wwtribe.c").read_text().replace(
            "strike < 3","strike < 30",1)
        with self.assertRaises(AssertionError):
            without_werewolf_combat_stage(changed)

    def test_only_four_reviewed_command_bodies_change(self):
        for file,names,expected in (
            ("wwbreed.c",("elemental",),"746c435b66b8d6ae243061ad2b1c18a86886766e"),
            ("wwtribe.c",("surge","cutting","invoke"),"e41266af1dd59d7525cde5e89b4e98221e8e6a42"),
        ):
            text=without_werewolf_reaction_stage((SRC/file).read_text())
            for name in names:text=text.replace(body(text,name),"",1)
            self.assertEqual(blob(text),expected,file)

    def test_restored_disquiet_is_byte_identical(self):
        self.assertEqual(blob(body((SRC/"wwbreed.c").read_text(),"disquiet")),
                         "a50e84aefd89a7a2a06c42a475cf52525bd82079")

    def test_weather_modes_are_byte_identical(self):
        text=(SRC/"wwtribe.c").read_text()
        text=text[text.index('    if (!str_cmp(arg1, "fog"))'):
                  text.index('    else if (!str_cmp(arg1, "lightning"))')]
        self.assertEqual(blob(text),"b298b0bc29938755c863a3cbbaba174a7fff4052")

    def test_original_access_and_damage_ranges_remain(self):
        breed=body((SRC/"wwbreed.c").read_text(),"elemental")
        tribe=(SRC/"wwtribe.c").read_text()
        for token in ("IS_WEREWOLF(ch)","BREED_LUPUS) < 5",
                      "number_range(1000, 1500)","number_range(250, 500)",
                      "number_range(2000, 3000)","strike < 4"):
            self.assertIn(token,breed)
        for name,gate in (("surge","TRIBE_GLASS_WALKERS) < 3"),
                          ("cutting","TRIBE_WENDIGOS) < 2"),
                          ("invoke","TRIBE_WENDIGOS) < 5")):
            text=body(tribe,name)
            self.assertIn(gate,text);self.assertIn("IS_WEREWOLF(ch)",text)
            self.assertNotIn("hybrid_power_parent",text)
        lightning=body(tribe,"invoke")
        self.assertIn("number_range(1000, 2000) + (100 * ch->pcdata->rank)",lightning)
        self.assertIn("strike < 3",lightning)
        self.assertIn("ch->pcdata->weather = 10",lightning)

    def test_callbacks_use_shared_membership_guards(self):
        for file,names in (("wwbreed.c",("elemental",)),
                           ("wwtribe.c",("surge","cutting","invoke"))):
            for name in names:
                text=body((SRC/file).read_text(),name)
                self.assertIn("cast_room = ch->in_room",text)
                self.assertIn("combat_character_in_room(cast_room, ch)",text)
                self.assertIn("ch->position < POS_FIGHTING",text)
                self.assertNotIn("COMBAT_CALLBACK_TESTING",text)
        self.assertIn("combat_pair_in_room(cast_room, ch, victim)",
                      body((SRC/"wwtribe.c").read_text(),"cutting"))

    def test_native_and_contract_cases_are_in_both_phases(self):
        make=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_werewolf_combat",make)
        self.assertIn("tests/test_werewolf_combat: tests/test_werewolf_combat.c",make)
        test=(ROOT/"tests/test_werewolf_combat.c").read_text()
        for token in ("15599035253278121217","cases==1064",
                      "remove_mobile();","callback_boundaries(0)",
                      "target_selection(0)","entry_guards()"):
            self.assertIn(token,test)

    def test_save_and_handoff_are_unchanged(self):
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

    def test_honest_runtime_and_balance_scope_documented(self):
        text=(ROOT/"docs/WEREWOLF.md").read_text()
        for token in ("Combat callback safety","cached next","explicit target",
                      "No new gifts","live PvP"):
            self.assertIn(token,text)

if __name__=="__main__":
    unittest.main()
