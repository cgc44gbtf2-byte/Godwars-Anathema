"""A ranked mana-based utility does not enable other Elementalist state."""
import hashlib
import json
import unittest
from test_class_layout import ROOT, SRC, make_values


def blob(text):
    data=text.encode()
    return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()


class HybridHealingCatalog(unittest.TestCase):
    def test_only_sunbath_permission_changes_after_arithmetic_fix(self):
        text=(SRC/"classes/elemental/elemental.c").read_text()
        self.assertEqual(text.count('#include "classes/hybrid/access.h"\n'),1)
        self.assertEqual(text.count("hybrid_power_parent("),1)
        self.assertEqual(text.count("hybrid_power_rank("),1)
        restored=text.replace('#include "classes/hybrid/access.h"\n',"").replace(
            "hybrid_power_parent(ch,CLASS_ELEM)","IS_ELEM(ch)").replace(
            "hybrid_power_rank(ch,CLASS_ELEM,HYBRID_TRAIN_ELEMENT,ELEM_SUN)",
            "ch->pcdata->powers[ELEM_SUN]")
        self.assertEqual(blob(restored),"f0ae81c799c5fa1488226dfb148a13f71b15db0a")

    def test_native_fix_contains_no_other_handler_changes(self):
        text=(SRC/"classes/elemental/elemental.c").read_text().replace(
            '#include "classes/hybrid/access.h"\n',"").replace(
            "hybrid_power_parent(ch,CLASS_ELEM)","IS_ELEM(ch)").replace(
            "hybrid_power_rank(ch,CLASS_ELEM,HYBRID_TRAIN_ELEMENT,ELEM_SUN)",
            "ch->pcdata->powers[ELEM_SUN]").replace('#include <limits.h>\n',"")
        fixed=('i = number_range(1001,1500);\n'
               '    /* Add before clamping only when the sum is representable. */\n'
               '    victim->hit = victim->hit > LONG_MAX-i ? victim->max_hit\n'
               '                                         : UMIN(victim->hit+i,victim->max_hit);\n'
               '    \tch->mana -= 800;\n')
        native=('i = victim->max_hit / 10;\n'
                '\tif ( ch->fighting != NULL ) i /= (11 - ch->pcdata->powers[ELEM_SUN]);\n'
                '\ti = number_range(1001,1500);\n'
                '   \tvictim->hit += i;\n'
                '    \tch->mana -= 800;\n'
                '    \tif (victim->hit > victim->max_hit)\n'
                '\t   victim->hit = victim->max_hit;\n')
        self.assertEqual(text.count(fixed),1)
        start=text.index("void do_sunbath")
        stop=text.index("void do_solarlight",start)
        sun=text[start:stop].replace(
            "    else if ( ( victim = get_char_room( ch, arg ) ) == NULL )",
            "    \tif ( ( victim = get_char_room( ch, arg ) ) == NULL )")
        text=text[:start]+sun+text[stop:]
        self.assertEqual(blob(text.replace(fixed,native)),
                         "d68a2624b0093da8ff888f62db6e4010c504e279")

    def test_native_cost_rng_lag_and_rank_threshold(self):
        text=(SRC/"classes/elemental/elemental.c").read_text()
        body=text.split("void do_sunbath",1)[1].split("void do_solarlight",1)[0]
        for token in ("ch->mana < 800","ch->mana -= 800","WAIT_STATE(ch, 18)",
                      "number_range(1001,1500)","ELEM_SUN) < 3"):
            self.assertIn(token,body)
        for token in ("hybrid_secondary_resource_","hybrid_player_commit",
                      "SET_BIT","REMOVE_BIT","ch->class =","ch->class="):
            self.assertNotIn(token,body)

    def test_real_engine_test_in_normal_and_sanitized_default(self):
        self.assertEqual(make_values(ROOT,"TESTS").count("tests/test_hybrid_healing"),1)
        make=(ROOT/"Makefile").read_text()
        self.assertIn("tests/test_hybrid_healing.c $(ENGINE_OBJECTS)",make)
        self.assertIn("python3 tests/test_hybrid_healing_catalog.py",make)
        self.assertIn("tests/test_hybrid_healing",(ROOT/".gitignore").read_text())
        test=(ROOT/"tests/test_hybrid_healing.c").read_text()
        self.assertIn("static const long baseline[8][7]",test)
        self.assertIn("overflow();malformed_rank();native_snapshots();secondary_cases();",test)

    def test_no_live_conversion_or_new_save_format(self):
        preview=(SRC/"classes/hybrid/selfbrid.c").read_text()
        self.assertNotIn("hybrid_player_attach(",preview)
        compat=json.loads((ROOT/"deploy/update_compat.json").read_text())
        self.assertEqual(compat["save_epoch"],3)
        self.assertEqual(compat["handoff"],2)

    def test_no_new_permission_mocks_in_legacy_tests(self):
        for name in ("test_powers.c","test_classes.c","test_hybrid_healing.c"):
            text=(ROOT/"tests"/name).read_text()
            self.assertNotRegex(text,r"\b(?:int|long)\s+hybrid_power_(?:parent|rank)\s*\(")

    def test_empty_parser_output_is_the_only_interpreter_change(self):
        text=(SRC/"interp.c").read_text()
        fixed="    if (*argument == '\\0')\n    {\n        arg[0]='\\0';\n        return 0;\n    }"
        native="    if (*argument == '\\0')\n\treturn 0;"
        self.assertEqual(text.count(fixed),1)
        self.assertEqual(blob(text.replace(fixed,native)),
                         "57a96947f99c2ccc2ba5121c5abe71853f42f62b")
        tests=(ROOT/"tests/test_hybrid_healing.c").read_text()
        self.assertIn('assert(number_argument("",empty)==0&&empty[0]',tests)

    def test_touch_permissions_only_restore_fixed_native_module(self):
        text=(SRC/"classes/monk/monk.c").read_text()
        self.assertEqual(text.count('#include "classes/hybrid/access.h"\n'),1)
        self.assertEqual(text.count("hybrid_power_parent("),1)
        self.assertEqual(text.count("hybrid_power_rank("),1)
        native=text.replace('#include "classes/hybrid/access.h"\n',"").replace(
            "hybrid_power_parent(ch,CLASS_MONK)","IS_MONK(ch)").replace(
            "    const long mantra=hybrid_power_rank(ch,CLASS_MONK,HYBRID_TRAIN_MONK,MONK_MANTRA);\n","").replace(
            "if (mantra < 4)","if (get_monk(ch, MONK_MANTRA) < 4)").replace(
            "number_range(400, 600) * mantra","number_range(400, 600) * get_monk(ch, MONK_MANTRA)")
        self.assertEqual(blob(native),"4d423dc19a5b06ca2897d980bc7ee07fe350ea1e")
        fixed=("    victim->hit = victim->hit > LONG_MAX-heal ? victim->max_hit\n"
               "                                            : UMIN(victim->hit+heal,victim->max_hit);")
        original=("    victim->hit += heal;\n"
                  "    if (victim->hit > victim->max_hit) victim->hit = victim->max_hit;")
        self.assertEqual(native.count(fixed),1)
        native=native.replace('#include <limits.h>\n',"").replace(fixed,original)
        self.assertEqual(blob(native),"b20b414f5550719b9662d61c8805eb018ee97520")

    def test_touch_native_cost_rng_order_targets_and_lag(self):
        text=(SRC/"classes/monk/monk.c").read_text()
        body=text.split("void do_touch",1)[1].split("void do_almighty",1)[0]
        ordered=["if (mantra < 4)","ch->mana < 5000","get_char_room(ch, arg)",
                 "number_range(400, 600) * mantra","cost = heal * 3 / 2",
                 "ch->mana < cost","victim->hit =","ch->mana -= cost",
                 "update_pos(victim)","WAIT_STATE(ch, 18)"]
        positions=[body.index(s) for s in ordered]
        self.assertEqual(positions,sorted(positions))
        for token in ("hybrid_secondary_resource_","hybrid_player_commit",
                      "SET_BIT","REMOVE_BIT","ch->class =","ch->class="):
            self.assertNotIn(token,body)
        self.assertIn("else if ((victim = get_char_room(ch, arg)) == NULL)",body)

    def test_touch_replays_are_in_default_real_engine_suite(self):
        text=(ROOT/"tests/test_hybrid_healing.c").read_text()
        self.assertIn("static const long baseline[2][11][7]",text)
        self.assertIn("touch_overflow();touch_native_snapshots();touch_secondary_cases();touch_cost_boundaries();",text)
        self.assertIn("for(long rank=4;rank<=8;++rank)",text)
        self.assertIn("assert(saw_odd_heal)",text)
        self.assertIn("pcs[0].monk[MONK_MANTRA]=LONG_MAX",text)
        self.assertIn("branch+=2",text)
        make=(ROOT/"Makefile").read_text()
        target=make.split("tests/test_powers: tests/test_powers.c",1)[1].split("\n\n",1)[0]
        self.assertIn("tests/powers_info.o $(HYBRID_ACCESS_SOURCES)",target)
        self.assertIn("tests/powers_info.o: Genesis/src/act_info.c",make)
        cleanup=make.split("clean:",1)[1]
        self.assertIn("tests/powers_info.o",cleanup)


if __name__=="__main__":
    unittest.main()
