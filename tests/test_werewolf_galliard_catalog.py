"""Galliard utility safety; no new power, targeting policy or world import."""
import hashlib
import json
from pathlib import Path
import re
import unittest
from werewolf_galliard_source import without_werewolf_galliard_stage

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src/classes/werewolf/wwauspice.c"

def body(text,name):
    return re.search(r"void do_"+name+r"\s*\(.*?(?=\nvoid do_|\Z)",text,re.S)[0]

class GalliardCatalog(unittest.TestCase):
    def test_only_two_bodies_change(self):
        raw=without_werewolf_galliard_stage(SRC.read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest(),
                         "7f2e072027a997df9ef080aea96bb37dbaadbc35")

    def test_future_edits_cannot_hide(self):
        text=SRC.read_text().replace("attempt < 4096","attempt < 4097")
        with self.assertRaises(AssertionError):without_werewolf_galliard_stage(text)

    def test_bounded_original_random_then_complete_scan(self):
        text=body(SRC.read_text(),"bridgewalk")
        for token in ("attempt < 4096","number_range( 100, 16000 )",
                      "to_room <= 16000","ROOM_PRIVATE","ROOM_SOLITARY",
                      "ROOM_NO_TELEPORT","No moon bridge","Galliard 5"):
            self.assertIn(token,text)
        self.assertNotIn("for ( ; ; )",text)
        self.assertLess(text.index("No moon bridge"),text.index("do_blissful"))

    def test_original_permissions_and_costs(self):
        text=SRC.read_text()
        self.assertIn("AUSPICE_GALLIARD) < 1",body(text,"wyld"))
        bridge=body(text,"bridgewalk")
        for token in ("!IS_WEREWOLF(ch)","AUSPICE_GALLIARD) < 5",
                      "EXTRA_SUMMON","victim->level > 1000","ROOM_SAFE",
                      "ROOM_NO_RECALL","WAIT_STATE(ch,8)"):
            self.assertIn(token,bridge)
        for name in ("wyld","bridgewalk"):
            code=body(text,name)
            for token in ("hybrid_","mana -=","practice -="):self.assertNotIn(token,code)

    def test_wounded_calls_and_bounded_messages(self):
        text=body(SRC.read_text(),"wyld")
        self.assertNotIn("sprintf(",text)
        self.assertEqual(text.count("snprintf(buf, sizeof buf,"),2)
        self.assertIn("victim == NULL && ch->hit > 0",text)
        self.assertLess(text.index("combat_character_in_room(ch->in_room, victim)"),
                        text.index("victim->short_descr"))

    def test_real_engine_native_pin_and_lookup_contract(self):
        make=(ROOT/"Makefile").read_text()
        self.assertIn("TESTS += tests/test_werewolf_galliard",make)
        self.assertIn("-Dget_room_index=werewolf_galliard_room",make)
        self.assertIn("tests/galliard_auspice.o",make.split("\nclean:",1)[1])
        test=(ROOT/"tests/test_werewolf_galliard.c").read_text()
        for token in ("count==800","819415021862684503","return get_room_index(vnum)",
                      "4096+15901","permissions_and_rank","POS_MORTAL"):
            self.assertIn(token,test)

    def test_compatibility_unchanged(self):
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

    def test_readme_is_brief_and_links_werewolf_guide_without_host_details(self):
        text=(ROOT/"README.md").read_text()
        self.assertLessEqual(len(text.splitlines()),120)
        self.assertIn("Werewolf advancement, shifting and gift-help corrections",text)
        self.assertIn("(docs/WEREWOLF.md)",text)
        self.assertTrue((ROOT/"docs/WEREWOLF.md").is_file())
        self.assertNotRegex(text,r"\b(?:\d{1,3}\.){3}\d{1,3}\b")
        self.assertNotRegex(text,r"(?i)\bport\s*[:=]?\s*\d+")

    def test_documented_sparse_world_limitations(self):
        text=(ROOT/"docs/WEREWOLF.md").read_text()
        for token in ("4,096","15,901","800","Call of the Wyld","Galliard 5",
                      "sparse","No new powers"):
            self.assertIn(token,text)

if __name__=="__main__":unittest.main()
