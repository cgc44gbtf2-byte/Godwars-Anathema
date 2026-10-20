"""Restore the scoped native safety/access changes, then pin the whole module."""
import hashlib
import json
import re
import unittest
from test_class_layout import ROOT, SRC, make_values
from test_hybrid_monk_buffs_catalog import without_monk_buff_stage

NAMES=("repeat","aurashot","blitz","cripple","darkblaze")
MINIMUM=(1,2,3,5,6)
NATIVE_GATE="if (!IS_MONK(ch) || ch->pcdata == NULL || ch->in_room == NULL)"
HYBRID_GATE="if (!hybrid_power_parent(ch, CLASS_MONK) || ch->pcdata == NULL || ch->in_room == NULL)"
NATIVE_RANK="    const long tech = get_monk(ch, MONK_TECH);\n"
HYBRID_RANK="    const long tech = hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_TECH);\n"

def replace_once(text,old,new):
    assert text.count(old)==1,repr(old)
    return text.replace(old,new,1)

def function_span(text,name):
    match=re.search(r"^void do_"+name+r"\(",text,re.M)
    assert match,name
    end=re.search(r"^void ",text[match.end():],re.M)
    assert end,name
    return match.start(),match.end()+end.start()

def without_monk_combat_stage(text):
    text=without_monk_buff_stage(text)
    for name,minimum in zip(NAMES,MINIMUM):
        a,b=function_span(text,name);body=text[a:b]
        gate=HYBRID_GATE if HYBRID_GATE in body else NATIVE_GATE
        body=replace_once(body,gate,"if (!IS_MONK(ch))")
        rank=HYBRID_RANK if HYBRID_RANK in body else NATIVE_RANK
        body=replace_once(body,rank,"")
        body=replace_once(body,"    if (ch == NULL) return;\n","")
        body=replace_once(body,f"if (tech < {minimum})",f"if (get_monk(ch, MONK_TECH) < {minimum})")
        if name=="repeat":
            body=replace_once(body,"    ROOM_INDEX_DATA *combat_room = ch->in_room;\n","")
            body=replace_once(body,"loop <= tech / 2","loop <= get_monk(ch, MONK_TECH) / 2")
            body=replace_once(body,"if (!combat_pair_in_room(combat_room, ch, victim)) break;",
                              "if (!combat_target_live(ch, victim)) break;")
            body=replace_once(body,"    if (combat_character_in_room(combat_room, ch)) WAIT_STATE(ch, 15);",
                              "    WAIT_STATE(ch, 15);")
        if name=="aurashot":
            body=replace_once(body," * tech;"," * get_monk(ch, MONK_TECH);")
            body=replace_once(body,
                "    const long heal = dam * 4L / 5;\n"
                "    ch->hit = ch->hit > LONG_MAX-heal ? ch->max_hit\n"
                "                                    : UMIN(ch->hit+heal,ch->max_hit);",
                "    ch->hit += dam * .8;\n    if (ch->hit > ch->max_hit) ch->hit = ch->max_hit;")
        if name=="blitz":
            body=replace_once(body,"    ROOM_INDEX_DATA *combat_room = ch->in_room;\n","")
            body=replace_once(body,"victim = combat_room->people;","victim = ch->in_room->people;")
            body=replace_once(body,
                "if (!combat_character_in_room(combat_room, ch)) return;\n"
                "/* A damage callback may remove the saved next room member. */\n"
                "if (!combat_character_in_room(combat_room, victim)) break;\n","")
            assert body.count(" * tech;")==2
            body=body.replace(" * tech;"," * get_monk(ch, MONK_TECH);")
            body=replace_once(body,
                "if (IS_NPC(victim)) damage = number_range(750, 1250) * get_monk(ch, MONK_TECH);",
                "\tif (IS_NPC(victim)) damage = number_range(750, 1250) * get_monk(ch, MONK_TECH);")
            body=replace_once(body,
                "else damage = number_range(500, 750) * get_monk(ch, MONK_TECH);",
                "\telse damage = number_range(500, 750) * get_monk(ch, MONK_TECH);")
            body=replace_once(body,"    if (combat_character_in_room(combat_room, ch)) WAIT_STATE(ch, 18);",
                              "    WAIT_STATE(ch, 18);")
        if name=="cripple":
            body=replace_once(body,
                "    victim->mana = victim->mana > dam * 3L ? victim->mana-dam * 3L : 0;",
                "    victim->mana -= dam * 3;\n    if (victim->mana < 0) victim->mana = 0;")
        text=text[:a]+body+text[b:]
    return text

class MonkCombatCatalog(unittest.TestCase):
    def test_full_native_module_restores_before_this_stage(self):
        native=without_monk_combat_stage((SRC/"classes/monk/monk.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(native)).encode()+b"\0"+native).hexdigest(),
                         "fe768fdaedd145cb0e9a3fede25aa1fc8bc95212")

    def test_rank_and_parent_changes_are_scoped_and_consistent(self):
        text=(SRC/"classes/monk/monk.c").read_text()
        phases=[]
        for name,minimum in zip(NAMES,MINIMUM):
            a,b=function_span(text,name);body=text[a:b]
            phases.append(HYBRID_GATE in body)
            self.assertIn(f"if (tech < {minimum})",body)
            self.assertEqual(body.count(NATIVE_RANK)+body.count(HYBRID_RANK),1)
            self.assertNotRegex(body,r"(?:ch->class\s*=(?!=)|hybrid_secondary_\w+\s*\()")
        self.assertTrue(all(phases))

    def test_native_resource_costs_and_attack_count(self):
        text=(SRC/"classes/monk/monk.c").read_text()
        for name,cost,lag in (("repeat",600,15),("aurashot",2000,12),
                              ("blitz",5000,18),("cripple",2000,9),("darkblaze",2000,18)):
            a,b=function_span(text,name);body=text[a:b]
            self.assertIn(f"ch->mana -= {cost};",body)
            self.assertIn(f"WAIT_STATE(ch, {lag})",body)
        self.assertIn("loop <= tech / 2",text)
        self.assertIn("ch->mana < 1200",text)
        self.assertIn("highlander_counter_parry(ch, victim, NULL, TYPE_UNARMED)",text)

    def test_callback_membership_precedes_saved_next_dereference(self):
        text=(SRC/"classes/monk/monk.c").read_text()
        a,b=function_span(text,"blitz");body=text[a:b]
        self.assertLess(body.index("combat_character_in_room(combat_room, victim)"),
                        body.index("victim_next = victim->next_in_room"))
        self.assertIn("if (combat_character_in_room(combat_room, ch)) WAIT_STATE",body)
        a,b=function_span(text,"repeat");body=text[a:b]
        self.assertIn("combat_pair_in_room(combat_room, ch, victim)",body)
        self.assertIn("if (combat_character_in_room(combat_room, ch)) WAIT_STATE",body)

    def test_arithmetic_and_null_boundaries_are_explicit(self):
        text=(SRC/"classes/monk/monk.c").read_text()
        self.assertIn("ch->hit > LONG_MAX-heal",text)
        self.assertIn("dam * 4L / 5",text)
        self.assertIn("victim->mana > dam * 3L ? victim->mana-dam * 3L : 0",text)
        for name in NAMES:
            a,b=function_span(text,name);body=text[a:b]
            self.assertIn("if (ch == NULL) return;",body)
            self.assertIn("ch->pcdata == NULL || ch->in_room == NULL",body)

    def test_real_engine_calls_are_not_permission_or_damage_stubs(self):
        test=(ROOT/"tests/test_hybrid_monk_combat.c").read_text()
        self.assertIn("tests/test_hybrid_monk_combat",make_values(ROOT,"TESTS"))
        self.assertIn("__real_one_hit(ch,victim,dt,message)",test)
        self.assertIn("__real_hurt_person(ch,victim,damage)",test)
        self.assertIn("free(removed)",test)
        self.assertIn("0xd7e43eace6e725b7",test)
        self.assertNotRegex(test,r"\b(?:bool|int|long)\s+(?:is_safe|hybrid_power_\w+|get_monk)\s*\([^;]*\)\s*\{")

    def test_native_getter_still_clamps_without_changing_class_identity(self):
        getter=(SRC/"act_info.c").read_text().split("long get_monk(",1)[1].split("bool get_drow",1)[0]
        self.assertIn("long max = 8;",getter)
        self.assertIn("if (ch->pcdata->monk[power] > max) return max;",getter)
        self.assertIn("if (!IS_MONK(ch)) return 0;",getter)

    def test_persistence_protocol_and_standalone_forms_not_changed(self):
        compat=json.loads((ROOT/"deploy/update_compat.json").read_text())
        self.assertEqual((compat["save_epoch"],compat["handoff"]),(3,2))
        text=(SRC/"classes/monk/monk.c").read_text()
        for name in NAMES:
            a,b=function_span(text,name);body=text[a:b]
            self.assertNotIn("AFF_POLYMORPH",body)
            self.assertNotIn("hybrid_forms_",body)
            self.assertNotIn("save_char",body)

    def test_branch_caps_and_live_original_banks_in_engine_matrix(self):
        test=(ROOT/"tests/test_hybrid_monk_combat.c").read_text()
        for fragment in ("focus==MONK_TECH?rank:UMIN(rank,4)","rank<=8",
                         "focus<3","mode<16","skill<5","pairs==25920",
                         "pairs==32400","fixture(CLASS_MONK,parents[p],rank,-1)",
                         "secondary_progression.monk[MONK_TECH]=9",
                         "actors[0].pcdata=NULL","__real_one_hit","__real_hurt_person"):
            self.assertIn(fragment,test)

    def test_command_costs_guide_and_nonplayable_status_are_documented(self):
        doc=(ROOT/"docs/HYBRIDS.md").read_text()
        for fragment in ("Secondary Monk direct attacks","1,200 mana",
                         "600 per","2,000 mana","5,000 mana","32,400 paired",
                         "No playable `selfbrid` conversion","Chimera, Almighty Favor"):
            self.assertIn(fragment,doc)
        help=(SRC/"classes/hybrid/help.c").read_text()
        for fragment in ("repeat (1)","aurashot (2)","blitz (3)",
                         "cripple (5)","darkblaze (6)","Forms and buffs remain native-only"):
            self.assertIn(fragment,help)

if __name__=="__main__":
    unittest.main()
