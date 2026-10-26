import hashlib
import unittest
from test_class_layout import ROOT, SRC, make_values
from native_drow_source import without_native_drow_stage

BASE = "5a19a2e21fbae6df7814ab263e5fb717e3705352"

def blob(text):
    b=text.encode()
    return hashlib.sha1(b"blob "+str(len(b)).encode()+b"\0"+b).hexdigest()

class NativeDrowCatalog(unittest.TestCase):
    def test_previous_complete_source_is_preserved(self):
        self.assertEqual(blob(without_native_drow_stage(
            (SRC/"classes/drow/drow.c").read_text())), BASE)

    def test_mutations_remain_visible(self):
        text=(SRC/"classes/drow/drow.c").read_text()
        for old, new in (
            ("victim->hit = 30000;", "victim->hit = 99999;"),
            ("ch->pcdata->quest -= 400;", "ch->pcdata->quest -= 1;"),
            ("pObjIndex->weight < 0", "pObjIndex->weight < -1"),
            ("ch->pcdata->followers != 0", "ch->pcdata->followers > 10"),
            ("cost = 10000;", "cost = 999;"),
            ("char buf[128];", "char buf[16];"),
        ):
            changed=text.replace(old,new)
            self.assertNotEqual(changed,text)
            try:
                restored=without_native_drow_stage(changed)
            except AssertionError:
                continue
            self.assertNotEqual(blob(restored),BASE)

    def test_real_world_suite_linked(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ("boot_db();", '"drowarmor %s"', '"prayer NativeDrow %s"',
                      "load_char_obj", "RT_TELEPORT|RT_RETURN",
                      "MOB_VNUM_GUARDIAN%MAX_KEY_HASH", "LONG_MAX"):
            self.assertIn(token,fixture)
        self.assertIn("tests/test_native_drow", make_values(ROOT,"TESTS"))
        self.assertIn("$(ENGINE_OBJECTS)", (ROOT/"Makefile").read_text())

    def test_reports_and_training_have_native_boundary_cases(self):
        fixture=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ("--report-only", "--report-extreme", "--training-bounds",
                      "LONG_MIN,5,LONG_MAX", "house_training();",
                      'command("prayer DrowStudent speed")',
                      'load_char_obj(&reload,"DrowStudent")'):
            self.assertIn(token,fixture)
        self.assertIn('assert(!memcmp(&before,actor->pcdata,sizeof before))',fixture)

    def test_only_reviewed_native_speed_runtime_edits(self):
        from native_drow_source import without_native_drow_speed
        baseline={
            "act_info.c":"1f75e52e5570adb1ec1f485da4c6448b94c0b08b",
            "combat_initiative.c":"fdb6313d65d875db0b035079a27705c65a7b6639",
            "combat_defense.c":"85870c3e786e30f58a484a6fbf6f8d4b53b2cd78",
            "combat_damage.c":"8ef28ea0e6107f77b8512f9ca40a50305ab709f0",
            "merc.h":"c7a99891b0f580cad130dc7e479672c62957cf25",
        }
        calls=0
        for path,expected in baseline.items():
            text=(SRC/path).read_text()
            self.assertEqual(blob(without_native_drow_speed(text)),expected,path)
            calls+=text.count("get_drow_speed(ch)")
            for old,new in (("* 50;","* 49;"),("* 100;","* 99;"),
                            ("rank <= 4 ? rank : 0","rank <= 5 ? rank : 0")):
                if old in text:
                    changed=text.replace(old,new,1)
                    self.assertNotEqual(blob(without_native_drow_speed(changed)),expected)
        self.assertEqual(calls,5)

    def test_native_speed_profiles_and_corrupt_data_checks(self):
        test=(ROOT/"tests/test_combat_calculations.c").read_text()
        for token in ("rows==120","2722730096019922979ULL","refusals==96",
                      "const long bad[]={-1,LONG_MIN,5,LONG_MAX}",
                      "assert(!memcmp(&before,&pa,sizeof pa))",
                      "assert(get_drow_speed(NULL)==0)"):
            self.assertIn(token,test)

    def test_prayer_menu_explains_recipient_and_self_alias(self):
        test=(ROOT/"tests/test_native_drow.c").read_text()
        for token in ('"prayer self nosuchgift"','"prayer acidbolt"',
                      'command("prayer self acidbolt")',
                      'assert(strstr(output,"prayer <name> <power>"))',
                      'load_char_obj(&reload,"NativeDrow")',"prayer_menu();"):
            self.assertIn(token,test)
        from native_drow_source import without_native_drow_stage
        text=(SRC/"classes/drow/drow.c").read_text()
        for old,new in (("cost = 5000;","cost = 50;"),
                        ("grant = DROW_ACID","grant = DROW_WEB")):
            self.assertNotEqual(blob(without_native_drow_stage(text.replace(old,new,1))),BASE)

if __name__ == "__main__":
    unittest.main()
