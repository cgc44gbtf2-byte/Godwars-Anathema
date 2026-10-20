"""Only the two Stun Fist access/rank gates may differ from the native body."""
import hashlib
import json
import re
import unittest
from test_class_layout import ROOT, SRC, make_values

NEW_PARENT="if (!hybrid_power_parent(ch, CLASS_MONK) || ch->pcdata == NULL)"
OLD_PARENT="if (!IS_MONK(ch) || ch->pcdata == NULL)"
NEW_RANK="if (hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_TECH) < 7)"
OLD_RANK="if (get_monk(ch, MONK_TECH) < 7)"

def without_hybrid_stun_adapter(text):
    before,body=text.split("void do_stunfist( CHAR_DATA *ch, char *argument )",1)
    body,after=body.split("void do_chimera(",1)
    assert body.count(NEW_PARENT)==body.count(NEW_RANK)==1
    body=body.replace(NEW_PARENT,OLD_PARENT,1).replace(NEW_RANK,OLD_RANK,1)
    return before+"void do_stunfist( CHAR_DATA *ch, char *argument )"+body+"void do_chimera("+after

class HybridStunCatalog(unittest.TestCase):
    def test_only_two_guard_expressions_change_the_whole_monk_module(self):
        native=without_hybrid_stun_adapter((SRC/"classes/monk/monk.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(native)).encode()+b"\0"+native).hexdigest(),
                         "727bc0fe5352b60e932ae2d99c483bb0c2fbae14")

    def test_cost_timers_and_native_class_predicate_remain(self):
        header=(SRC/"merc.h").read_text()
        for name,value in (("COST","5000"),("DURATION","PULSE_PER_SECOND"),
                           ("RECOVERY","(2 * PULSE_PER_SECOND)"),("IMMUNITY","(6 * PULSE_PER_SECOND)")):
            actual=re.search(r"^#define STUN_FIST_"+name+r"\s+(.+)$",header,re.M).group(1)
            self.assertEqual(actual.strip(),value)
        self.assertRegex(header,r"#define IS_MONK\(ch\)\s+\(\(ch\)->class == CLASS_MONK && !IS_NPC\(ch\)\)")
        compat=json.loads((ROOT/"deploy/update_compat.json").read_text())
        self.assertEqual((compat["save_epoch"],compat["handoff"]),(3,2))

    def test_targets_immunity_counter_and_charge_order_stay_native(self):
        body=(SRC/"classes/monk/monk.c").read_text().split("void do_stunfist(",1)[1].split("void do_chimera(",1)[0]
        order=["ch == NULL","ch->pcdata == NULL","IS_COMBAT_PAUSED(ch)",
               "HYBRID_TRAIN_MONK, MONK_TECH) < 7","ch->in_room == NULL",
               "get_char_room(ch, arg)","ch == victim","!IS_AWAKE(victim)",
               "ch->master == victim","victim->stun_immunity > 0",
               "ch->mana < STUN_FIST_COST","is_safe(ch, victim)","ch->mana -= STUN_FIST_COST",
               "ch->combat_pause = STUN_FIST_RECOVERY","highlander_counter_parry",
               "victim->combat_pause = STUN_FIST_DURATION","victim->stun_immunity = STUN_FIST_IMMUNITY"]
        positions=[body.index(part) for part in order]
        self.assertEqual(positions,sorted(positions))
        self.assertNotRegex(body,r"(?:ch->class\s*=|hybrid_(?:secondary|player)_\w+\s*\(|number_(?:range|percent)\s*\()")

    def test_engine_matrix_and_native_digest_are_not_permission_stubs(self):
        self.assertIn("tests/test_hybrid_stun",make_values(ROOT,"TESTS"))
        self.assertIn("tests/test_hybrid_stun: tests/test_hybrid_stun.c $(ENGINE_OBJECTS)",
                      (ROOT/"Makefile").read_text())
        test=(ROOT/"tests/test_hybrid_stun.c").read_text()
        for part in ("pairs==7776","pairs==9720","focus<3","rank<=8","mode<24",
                     "rank=-1;rank<=9","0xb967ef86a4987bad","combat_status_update()",
                     "interpret(actors,text)","--secondary-repro","--native-dump"):
            self.assertIn(part,test)
        self.assertNotRegex(test,r"\b(?:long|int|bool|void)\s+(?:hybrid_power_\w+|"
                            r"do_stunfist|combat_status_update|hurt_fighting|is_safe)\s*\([^;]*\)\s*\{")

    def test_raw_half_caps_primary_retention_and_corrupt_guards(self):
        test=(ROOT/"tests/test_hybrid_stun.c").read_text()
        for part in ("focus==MONK_TECH?rank:UMIN(rank,4)","Primary-bank decoy",
                     "hybrid.secondary_progression.monk[MONK_TECH]=9","player_load_failed=1",
                     "hybrid_present=2","CLASS_NONE","actors[0].pcdata=NULL",
                     "fixture(CLASS_MONK,parents[p],rank,-1)","actors[2].mana==10000",
                     "actors[0].mana==5000&&!actors[0].combat_pause"):
            self.assertIn(part,test)

    def test_docs_and_generated_help_explain_existing_records_only(self):
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("Secondary Monk Stun Fist","full Tech path","5,000 mana",
                     "other secondary Monk paths cap Tech at4","264 native snapshots",
                     "9720 paired replays","no new conversion","no new trainer"):
            self.assertIn(part,docs)
        help=(SRC/"classes/hybrid/help.c").read_text()
        self.assertIn("Tech 7 on the full Tech path",help)
        self.assertIn("5000 mana",help)

if __name__=="__main__":
    unittest.main()
