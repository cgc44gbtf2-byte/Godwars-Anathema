"""Prices, dispatch, honest command listings and exact native source scope."""
import hashlib
import json
import re
import unittest
from test_class_layout import ROOT,SRC,make_values
from werewolf_source import without_werewolf_stage

def blob(text):
    raw=text.encode()
    return hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()

class WerewolfCommandsCatalog(unittest.TestCase):
    def test_native_scope_restores_exact_prior_modules(self):
        for path,expected in (
            ("classes/werewolf/werewolf.c","7dc28aead30f5407cc3ec2f3e2c657ed0fe0c788"),
            ("classes/hybrid/training.c","4646c2494470f6427efbba6da69752b32590c2a2"),
            ("interp.c","ccb0125d4264a8a5447618c29a9240ec674d2eb4"),
            ("merc.h","b843f157662ab181daabe913d74685f1c8bc75e6")):
            restored=without_werewolf_stage((SRC/path).read_text())
            if path=="interp.c":
                from reader_fields_source import without_reader_fields_stage
                restored=without_reader_fields_stage(restored)
            if path=="merc.h":
                # The hybrid form-ledger stage adds its own independent
                # persistence marker; it is not part of Werewolf command scope.
                restored=restored.replace("    int                 hybrid_forms_present;\n","")
            self.assertEqual(blob(restored),expected,path)

    def test_only_shared_pricing_policy_no_quest_gate(self):
        price=(SRC/"classes/werewolf/advancement.h").read_text()
        for part in ("rank * 100 * multiplier","rank * 1000000 * multiplier",
                     "rank * 1000 * multiplier","hybrid ? 2 : 1","rank >= 4"):
            self.assertIn(part,price)
        native=(SRC/"classes/werewolf/werewolf.c").read_text().split(
            "void do_advance(",1)[1].split("\nvoid do_wwpowers(",1)[0]
        secondary=(SRC/"classes/hybrid/training.c").read_text().split(
            "case HYBRID_LESSON_WOLF_RANK:",1)[1].split(
                "case HYBRID_LESSON_DROW_SPEED:",1)[0]
        for body in (native,secondary):
            self.assertIn("werewolf_advancement_price",body)
            self.assertNotIn("SCORE_NUM_QUEST",body)
        self.assertIn("hybrid_player_save_valid(ch)",native)
        self.assertIn("ch->mkill < kills",native)
        self.assertIn("ch->mkill<kills",secondary)
        self.assertNotIn("ch->mkill -=",native)

    def test_shift_is_real_dispatch_without_extra_form_permissions(self):
        self.assertRegex((SRC/"interp.c").read_text(),
                         r'"shift",\s*do_shift,\s*POS_FIGHTING,\s*LEV_AWA')
        text=(SRC/"classes/werewolf/instincts.c").read_text().split(
            "void do_shift(",1)[1]
        self.assertIn('do_rage(ch,"")',text)
        self.assertIn('do_calm(ch,"")',text)
        self.assertIn("ch->position < POS_STANDING",text)
        self.assertNotIn("SET_BIT",text)
        self.assertNotIn("do_werewolf(",text)
        self.assertNotIn("HYBRID_",text)

    def test_all_werewolf_handlers_are_registered_or_explicitly_unavailable(self):
        table=(SRC/"interp.c").read_text()
        entries={fn for _,fn in re.findall(
            r'\{\s*"([^"]+)",\s*(do_\w+)',table)}
        missing=set()
        for path in (SRC/"classes/werewolf").glob("*.c"):
            missing.update(fn for fn in re.findall(
                r"^void\s+(do_\w+)\s*\(",path.read_text(),re.M)
                if fn not in entries)
        self.assertEqual(missing,{"do_quicksand","do_lycaon","do_reach",
                                 "do_wrath","do_mastery","do_invisibility"})
        menu=(SRC/"classes/werewolf/werewolf.c").read_text().split(
            "void do_wwpowers(",1)[1].split("void do_gifts(",1)[0]
        for label in ("Quicksand[disabled]","Lycaon[disabled]",
                      "Reach[unavailable]","Wrath[unavailable]",
                      "Mastery[unavailable]","Invisibility[unavailable]",
                      "Totemgift[unimplemented]"):
            self.assertIn(label,menu)
        for label in ("Spiritward","Reshapeobject","Spiritguardian","Halosun","Formillusion",
                      "Shroudnight","Clapthunder","Paralyzingstare"):
            self.assertIn(label,menu)
        commands=set(re.findall(r'\{\s*"([^"]+)"\s*,\s*do_\w+',table))
        labels=re.findall(r'(?:strcpy|strcat)\(buf,\s*"([^"\n]+)"',menu)
        for label in labels:
            if label==".\\n\\r" or "[" in label:continue
            words=label.split(":")[-1].strip().split()
            if words:self.assertIn(words[0].lower(),commands,label)
        self.assertIn("Scream[unimplemented]",menu)
        self.assertNotIn("get_auspice(ch, TRIBE_",menu)

    def test_engine_suite_and_saved_format_unchanged(self):
        self.assertIn("tests/test_werewolf_commands",make_values(ROOT,"TESTS"))
        test=(ROOT/"tests/test_werewolf_commands.c").read_text()
        for item in ("count==1539","mode<19","hybrid_training_prepare",
                     "SCORE_NUM_QUEST]=0","interpret(actors,command)",
                     "shift werewolf","shift human","LONG_MIN","LONG_MAX"):
            self.assertIn(item,test)
        self.assertEqual(json.loads((ROOT/"deploy/update_compat.json").read_text()),
                         {"schema":1,"save_epoch":4,"handoff":2})

    def test_documented_native_and_hybrid_prices(self):
        docs=(ROOT/"docs/WEREWOLF.md").read_text()
        for phrase in ("100 | 1,000,000 | 1,000","600 | 6,000,000 | 6,000",
                       "cumulative","Gaia 1","ordinary player-save",
                       "unavailable","advance info"):
            self.assertIn(phrase,docs)

    def test_help_placeholders_active_commands_and_duplicates(self):
        helpfile=(ROOT/"Genesis/area/class.hlp").read_text()
        def section(name):
            return helpfile.split("0 "+name+"~",1)[1].split("~",1)[0]
        shadow=section("'Shadow Lords'")
        fenris=section("'Get of Fenris'")
        self.assertIn("Clap of Thunder",shadow)
        self.assertRegex(shadow,r"2: Clap of Thunder - [^\n]+<clapthunder>")
        self.assertNotRegex(shadow,r"Clap of Thunder[^\n]*<auto>")
        self.assertIn("<paralyzingstare>",shadow)
        self.assertIn("Strength of the Dominator",shadow)
        self.assertIn("<dominator>",shadow)
        self.assertNotIn("Open Wounds",shadow)
        self.assertIn("Adds 100 to the Werewolf damage cap. <auto>",fenris)
        self.assertNotRegex(helpfile,r"(?i)\bblah\b")
        self.assertIn("1 -> 2     100     1,000,000",section("Ranks"))
        self.assertIn("double every number",section("Ranks"))
        self.assertIn("not implemented Shift options",section("Shift"))
        werewolf=helpfile.split("0 Homid~",1)[1].split("0 Vampires~",1)[0]
        names=re.findall(r"(?m)^\d+: ([^\n]+?) - ",werewolf)
        self.assertEqual(len(names),105)
        self.assertEqual(len(set(n.casefold() for n in names)),len(names))
        text=(SRC/"classes/werewolf/werewolf.c").read_text().split(
            "void do_wwpowers(",1)[1].split("void do_gifts(",1)[0]
        labels=re.findall(r'(?:strcpy|strcat)\(buf,\s*"([^"\n]+)"',text)
        active=[]
        for label in labels:
            if label==".\\n\\r" or "[" in label:continue
            words=label.split(":")[-1].strip().split()
            if words:active.append(words[0].casefold())
        self.assertEqual(len(active),58)
        self.assertEqual(len(set(active)),len(active))

if __name__=="__main__":
    unittest.main()
