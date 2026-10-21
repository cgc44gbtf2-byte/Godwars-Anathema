"""Cross-check all 105 Werewolf help slots against native command/effect sources."""
from collections import Counter
from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"Genesis/src"
TREES=("Homid","Metis","Lupus","Ragabash","Theurge","Philodox","Galliard",
       "Ahroun","Black Furies","Bone Gnawers","Children of Gaia","Fianna",
       "Get of Fenris","Glass Walkers","Red Talons","Shadow Lords",
       "Silent Striders","Silver Fangs","Uktena","Wendigos","Stargazers")
UNVERIFIED={("Ahroun",1),("Galliard",2),("Bone Gnawers",5),
            ("Red Talons",1),("Red Talons",2),("Silent Striders",2),
            ("Silver Fangs",1),("Silver Fangs",2),("Uktena",3)}
UNIMPLEMENTED={("Metis",5),("Get of Fenris",4),("Silent Striders",4)}
UNAVAILABLE={("Silent Striders",5):"do_reach",("Silver Fangs",3):"do_wrath",
             ("Silver Fangs",5):"do_mastery",("Uktena",4):"do_invisibility"}
DISABLED={("Red Talons",4):"do_quicksand",("Red Talons",5):"do_lycaon"}
# Each passive has a production consumer, not merely an upkeep message.
PASSIVES={
 ("Lupus",3):("combat_defense.c","get_breed(ch,BREED_LUPUS) >= 3"),
 ("Ragabash",3):("act_move.c","get_auspice(ch,AUSPICE_RAGABASH) >= 3"),
 ("Ragabash",5):("combat_damage.c","get_auspice(victim, AUSPICE_RAGABASH) < 5"),
 ("Philodox",1):("act_info.c","get_auspice(ch, AUSPICE_PHILODOX) > 0"),
 ("Philodox",2):("act_info.c","get_auspice(ch, AUSPICE_PHILODOX) > 1"),
 ("Philodox",3):("combat_damage.c","get_auspice(ch, AUSPICE_PHILODOX) >= 3"),
 ("Ahroun",2):("combat_initiative.c","get_auspice(ch, AUSPICE_AHROUN) >= 2"),
 ("Ahroun",3):("combat_damage.c","get_auspice(ch, AUSPICE_AHROUN) >= 3"),
 ("Ahroun",4):("update.c","get_auspice(ch, AUSPICE_AHROUN) >= 4"),
 ("Black Furies",3):("combat_attack.c","get_tribe(ch, TRIBE_BLACK_FURIES) >= 3"),
 ("Black Furies",4):("fight.c","get_tribe(ch, TRIBE_BLACK_FURIES) >= 4"),
 ("Bone Gnawers",1):("magic.c","get_tribe(victim, TRIBE_BONE_GNAWERS) >= 1"),
 ("Bone Gnawers",2):("combat_defense.c","get_tribe(victim, TRIBE_BONE_GNAWERS) >= 2"),
 ("Children of Gaia",2):("combat_defense.c","get_tribe(ch,TRIBE_CHILDREN_OF_GAIA) >= 2"),
 ("Get of Fenris",1):("combat_damage.c","get_tribe(ch, TRIBE_GET_OF_FENRIS) >= 1"),
 ("Get of Fenris",2):("fight.c","get_tribe(victim, TRIBE_GET_OF_FENRIS) * 10"),
 ("Get of Fenris",3):("update.c","get_tribe(vch, TRIBE_GET_OF_FENRIS) >= 3"),
 ("Get of Fenris",5):("fight.c","get_tribe(victim, TRIBE_GET_OF_FENRIS) >= 5"),
 ("Red Talons",3):("update.c","get_tribe(ch, TRIBE_RED_TALONS) >= 3"),
 ("Shadow Lords",1):("combat_damage.c","get_tribe(ch, TRIBE_SHADOW_LORDS) >= 1"),
 ("Silent Striders",1):("combat_initiative.c","get_tribe(ch, TRIBE_SILENT_STRIDERS) >= 1"),
 ("Silent Striders",3):("combat_initiative.c","get_tribe(ch, TRIBE_SILENT_STRIDERS) >= 3"),
 ("Uktena",2):("act_move.c","get_tribe(ch, TRIBE_UKTENA) > 1"),
 ("Wendigos",4):("magic.c","get_tribe(victim, TRIBE_WENDIGOS) >= 4"),
 ("Stargazers",2):("act_info.c","get_tribe(ch, TRIBE_STARGAZERS) >= 2"),
 ("Stargazers",3):("handler.c","get_disc(victim, DISC_OBFUSCATE) <= get_tribe(ch, TRIBE_STARGAZERS)"),
 ("Stargazers",4):("combat_defense.c","get_tribe(ch,TRIBE_STARGAZERS) >= 4"),
 ("Stargazers",5):("combat_damage.c","get_tribe(victim, TRIBE_STARGAZERS) >= 5"),
}

def catalog():
    text=(ROOT/"Genesis/area/class.hlp").read_text()
    blocks={m.group(1).strip("'"):m.group(2) for m in re.finditer(
        r"^0 ([^\n~]+)~\n(.*?)^~",text,re.M|re.S)}
    rows={}
    for tree in TREES:
        for m in re.finditer(r"^([1-5]): (.*?) - (.*?) <([^>\n]+)>$",blocks[tree],re.M):
            key=(tree,int(m.group(1)))
            if key in rows:raise AssertionError("Repeated rank: "+str(key))
            rows[key]={"name":m.group(2),"description":m.group(3),"tag":m.group(4)}
    return rows

class WerewolfHelpCatalog(unittest.TestCase):
    def test_all_105_slots_have_explicit_unique_classification(self):
        rows=catalog()
        self.assertEqual(set(rows),{(t,r) for t in TREES for r in range(1,6)})
        self.assertEqual(len({x["name"].casefold() for x in rows.values()}),105)
        kinds=Counter(x["tag"] if x["tag"] in {
            "auto","unverified","unimplemented","unavailable","disabled"} else
            x["tag"].split(":")[0] if ":" in x["tag"] else "command"
            for x in rows.values())
        self.assertEqual(kinds,{"command":57,"auto":28,"unverified":9,
            "unimplemented":3,"unavailable":4,"disabled":2,"stub":1,"upgrade":1})

    def test_57_commands_and_stub_match_menu_at_exact_tree_and_rank(self):
        text=(SRC/"classes/werewolf/werewolf.c").read_text().split(
            "void do_wwpowers(",1)[1].split("void do_gifts(",1)[0]
        menu={}
        for m in re.finditer(
            r"if \(get_(?:breed|auspice|tribe)\(ch, \w+\) >= 1\)\n    \{(.*?)\n    \}",
            text,re.S):
            body=m.group(1)
            label=re.search(r'strcpy\(buf, "([^"]+)"\)',body).group(1)
            tree,first=label.split(":",1)
            if first.strip():menu[(tree,1)]=first.strip().lower()
            for rank,cmd in re.findall(r'>= ([2-5])\) strcat\(buf, " ([^"]+)"\)',body):
                if "[" not in cmd:menu[(tree,int(rank))]=cmd.lower()
        rows=catalog()
        actual={k:v["tag"] for k,v in rows.items() if re.fullmatch(r"[a-z]+",v["tag"])
                and v["tag"] not in {"auto","unverified","unimplemented","unavailable","disabled"}}
        actual[("Homid",4)]=rows[("Homid",4)]["tag"].removeprefix("stub: ")
        self.assertEqual(actual,menu)
        self.assertEqual(len(set(actual.values())),58)
        bindings=dict(re.findall(r'\{\s*"([^"]+)"\s*,\s*(do_\w+)',
                                 (SRC/"interp.c").read_text()))
        definitions="\n".join(p.read_text() for p in SRC.rglob("*.c"))
        for command in actual.values():
            self.assertIn(command,bindings)
            self.assertRegex(definitions,r"void\s+"+bindings[command]+r"\s*\([^;{}]*\)\s*\{")

    def test_each_auto_entry_has_a_real_production_consumer(self):
        rows=catalog()
        self.assertEqual({k for k,v in rows.items() if v["tag"]=="auto"},set(PASSIVES))
        for key,(path,needle) in PASSIVES.items():
            with self.subTest(gift=key):
                self.assertIn(needle,(SRC/path).read_text())

    def test_gaps_are_not_sold_as_working_passives_or_commands(self):
        rows=catalog()
        for tag,keys in (("unverified",UNVERIFIED),("unimplemented",UNIMPLEMENTED),
                         ("unavailable",UNAVAILABLE),("disabled",DISABLED)):
            self.assertEqual({k for k,v in rows.items() if v["tag"]==tag},set(keys))
        table=(SRC/"interp.c").read_text()
        handlers=set(re.findall(r'\{\s*"[^"]+"\s*,\s*(do_\w+)',table))
        source=(SRC/"classes/werewolf/wwtribe.c").read_text()
        for fn in list(UNAVAILABLE.values())+list(DISABLED.values()):
            self.assertIn("void "+fn+"(",source)
            self.assertNotIn(fn,handlers)
        stub=(SRC/"classes/werewolf/wwbreed.c").read_text().split(
            "void do_disquiet(",1)[1].split("void do_spiritward(",1)[0]
        self.assertIn('stc("Gutted!',stub)
        for action in ("SET_BIT","REMOVE_BIT","damage(","affect_to_char(","ch->primal -="):
            self.assertNotIn(action,stub)

    def test_grandmother_is_an_upgrade_not_an_extra_auto_heal(self):
        row=catalog()[("Theurge",2)]
        self.assertEqual(row["tag"],"upgrade: mother")
        source=(SRC/"classes/werewolf/wwauspice.c").read_text().split(
            "void do_mother(",1)[1].split("void do_pulse(",1)[0]
        self.assertIn("ch == victim && get_auspice(ch, AUSPICE_THEURGE) < 2",source)
        self.assertIn("ch->pcdata->obeah = 15",source)

    def test_protection_descriptions_match_their_limited_consumers(self):
        rows=catalog()
        self.assertIn("standard poison spell",rows[("Bone Gnawers",1)]["description"])
        self.assertIn("standard frost-breath spell",rows[("Wendigos",4)]["description"])
        self.assertIn("underwater movement",rows[("Uktena",2)]["description"])
        self.assertNotIn("soak",rows[("Children of Gaia",5)]["description"])
        source=(SRC/"combat_defense.c").read_text()
        wolf=source.split("case CLASS_WEREWOLF:",1)[1].split("case CLASS_HIGHLANDER:",1)[0]
        self.assertNotIn("ITEMA_SUNSHIELD",wolf)
        elemental=source.split("case CLASS_ELEM:",1)[1].split("case CLASS_MONK:",1)[0]
        self.assertIn("ITEMA_SUNSHIELD",elemental)

if __name__=="__main__":
    unittest.main()
