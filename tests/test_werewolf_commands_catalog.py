"""Prices, dispatch, honest command listings and exact native source scope."""
import hashlib
import json
import re
import unittest
from test_class_layout import ROOT,SRC,make_values
from werewolf_source import without_werewolf_stage
from quicksand_source import without_quicksand

def blob(text):
    raw=text.encode()
    return hashlib.sha1(b"blob "+str(len(raw)).encode()+b"\0"+raw).hexdigest()

class WerewolfCommandsCatalog(unittest.TestCase):
    def test_invisibility_inverse_and_staff_command_order(self):
        from invisibility_source import without_invisibility
        for path,expected in (
            ("handler.c","60868b8d703b9a25b19a3b34bbc801792bc28c47"),
            ("classes/werewolf/wwtribe.c","361745c66f6785c5e01dc78209a47fe9a214236b"),
            ("classes/werewolf/werewolf.c","e266548bb5687c05b30e8d5bcdafb53c5f0e36cd"),
            ("interp.c","f66cc5423740fcdf976f6afed60e31cf015acc47"),
            ("merc.h","e4c0ad5ff0134c020a4bc7fba5ab9fb57d9c7cc8"),
            ("../area/class.hlp","e7b10caec9a78ee7e028e0fb374db4703ed5c83e"),
        ):
            text=(SRC/path).read_text()
            restored=without_invisibility(text)
            self.assertEqual(blob(restored),expected)
            self.assertEqual(without_invisibility(restored),restored)
            self.assertNotEqual(blob(without_invisibility(text+"\n/* mutation */\n")),expected)
        source=(SRC/"handler.c").read_text()
        self.assertNotEqual(blob(without_invisibility(source.replace(
            "!IS_AFFECTED(ch, AFF_DETECT_INVIS)","IS_AFFECTED(ch, AFF_DETECT_INVIS)",1))),
            "60868b8d703b9a25b19a3b34bbc801792bc28c47")
        interp=(SRC/"interp.c").read_text()
        self.assertLess(interp.index('{ "invis",'),interp.index('{ "invisibility",'))
        self.assertIn("tests/test_invisibility",make_values(ROOT,"TESTS"))

    def test_wrath_inverse_preserves_all_other_runtime_and_help_bytes(self):
        from wrath_source import without_wrath
        for path,expected in (
            ("classes/werewolf/wwtribe.c","ed6fbdd8263e54294e4a6db31a999cf7a83551de"),
            ("classes/werewolf/werewolf.c","4b25ab7b4dd57c3ae7d280d8eb7339b175aa35cb"),
            ("interp.c","61bb8074c2d39b42275fa735e7be1510b7803e80"),
            ("merc.h","a47166c0a274e70beb5d3f178662c4fd9a087b3f"),
            ("../area/class.hlp","f049922512be296df2fc281728107779c6ecbe48"),
        ):
            with self.subTest(path=path):
                text=(SRC/path).read_text()
                restored=without_wrath(text)
                self.assertEqual(blob(restored),expected)
                self.assertEqual(without_wrath(restored),restored)
                self.assertNotEqual(blob(without_wrath(text+"\n/* mutation */\n")),expected)
        source=(SRC/"classes/werewolf/wwtribe.c").read_text()
        with self.assertRaises(AssertionError):
            without_wrath(source.replace("victim->hitroll < LONG_MIN + rank",
                                         "victim->hitroll < LONG_MIN",1))

    def test_reach_inverse_preserves_all_other_runtime_and_help_bytes(self):
        from reach_source import without_reach
        for path,expected in (
            ("classes/werewolf/wwtribe.c","aa686020c6f9d4e0d2a452b139c629e2d0be01ee"),
            ("classes/werewolf/werewolf.c","7665c4350bef29d48c8a83efc8f705ae3f48dbaa"),
            ("interp.c","71d7b8c829f316d2630ba88b1b609b76f0b9269d"),
            ("merc.h","ef2a67bcd4bb8b02409b81e2b41710d9156670a8"),
            ("../area/class.hlp","d4cbeb38484cb6a33a3acf7c4af110475b4c4eb6"),
        ):
            with self.subTest(path=path):
                text=(SRC/path).read_text()
                restored=without_reach(text)
                self.assertEqual(blob(restored),expected)
                self.assertEqual(without_reach(restored),restored)
                self.assertNotEqual(blob(without_reach(text+"\n/* mutation */\n")),expected)
        source=(SRC/"classes/werewolf/wwtribe.c").read_text()
        with self.assertRaises(AssertionError):
            without_reach(source.replace("safe = is_safe(ch, victim);",
                                         "safe = FALSE;",1))

    def test_quicksand_inverse_preserves_all_other_runtime_bytes(self):
        for path,expected in (
            ("act_move.c","83cb8b9819d7205bee50c7a27323390adfb59b6d"),
            ("classes/werewolf/werewolf.c","b8c06b3ab7df6129f0cc1aafa76e14e080eaaf42"),
            ("classes/werewolf/wwtribe.c","9e3d18f7a70157010da5a80d411ced498a63d422"),
            ("combat_status.c","103048b2fcf8d20745b2602d6fe5c56d34aa132d"),
            ("handler.c","fe6fa8189fea2cb77c3f64c0b6d2c493ddd94bad"),
            ("interp.c","55919c3bb51ff6b282e16d9fa99b189665767b05"),
            ("merc.h","bae3d92e26d1d41b41d76764e07b5538722388b7"),
        ):
            with self.subTest(path=path):
                text=(SRC/path).read_text()
                old=without_quicksand(text)
                self.assertEqual(blob(old),expected)
                self.assertEqual(without_quicksand(old),old)
                self.assertNotEqual(blob(without_quicksand(text+"\n/* mutation */\n")),expected)
        source=(SRC/"classes/werewolf/wwtribe.c").read_text()
        with self.assertRaises(AssertionError):
            without_quicksand(source.replace("victim->quicksand_timer = 20;",
                                             "victim->quicksand_timer = 200;",1))

    def test_quicksand_uses_loaded_character_pulses_and_no_saved_flags(self):
        code=(SRC/"combat_status.c").read_text()
        self.assertIn("for (ch = char_list; ch != NULL; ch = ch->next)",code)
        self.assertIn("--ch->quicksand_timer == 0",code)
        comm=(SRC/"comm.c").read_text()
        self.assertEqual(comm.count("        combat_status_update();"),2)
        for path in ("save.c","update.c"):
            self.assertNotIn("quicksand_timer",(SRC/path).read_text())
        helpfile=(ROOT/"Genesis/area/class.hlp").read_text()
        self.assertIn("20 pulses; costs 1 mana. <quicksand>",helpfile)

    def test_native_scope_restores_exact_prior_modules(self):
        for path,expected in (
            ("classes/werewolf/werewolf.c","7dc28aead30f5407cc3ec2f3e2c657ed0fe0c788"),
            ("classes/hybrid/training.c","4646c2494470f6427efbba6da69752b32590c2a2"),
            ("interp.c","cd18b4a21c44e1c1dc9db1caf42caffd35235c67"),
            ("merc.h","3b098a58b44542ce99b83b66d9ea780932642300")):
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
        self.assertEqual(text.count("do_werewolf("),1)
        self.assertIn('do_werewolf(ch,"shift")',text)
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
        self.assertEqual(missing,{"do_mastery"})
        menu=(SRC/"classes/werewolf/werewolf.c").read_text().split(
            "void do_wwpowers(",1)[1].split("void do_gifts(",1)[0]
        for label in ("Quicksand","Lycaon","Screamgaia",
                      "Reach","Wrath",
                      "Mastery[unavailable]","Invisibility",
                      "Totemgift"):
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
        self.assertNotIn("Scream[unimplemented]",menu)
        self.assertNotIn("Totemgift[unimplemented]",menu)
        self.assertNotIn("get_auspice(ch, TRIBE_",menu)

    def test_engine_suite_and_saved_format_unchanged(self):
        self.assertIn("tests/test_werewolf_commands",make_values(ROOT,"TESTS"))
        self.assertIn("tests/test_werewolf_totem",make_values(ROOT,"TESTS"))
        self.assertIn("tests/test_werewolf_wrath",make_values(ROOT,"TESTS"))
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
        self.assertEqual(len(active),65)
        self.assertEqual(len(set(active)),len(active))

if __name__=="__main__":
    unittest.main()
