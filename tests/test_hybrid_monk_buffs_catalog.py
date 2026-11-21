"""Pin the native Monk module and the two shared cleanup call sites."""
import hashlib
import json
import unittest
from test_class_layout import ROOT, SRC, make_values
from test_favour_expiry_catalog import without_favour_metadata_monk,without_favour_metadata_merc
from gargoyle_death_source import without_gargoyle_death

BASE_MONK="6b39e0bfa7c961f53a82ce8061044152c2d26809"
BASE_DEATH="c4dd62ddf5117d3d7f01e788f526b497ee58cf08"
BASE_UPDATE="0a2ef9fcafcdf72f1d314c11236ee3ffb9a7c39e"
BASE_MERC="3e2d1a6c71a00bb7159af64272dddf42e846133b"
NATIVE_GATE="if (!IS_MONK(ch) || ch->pcdata == NULL || ch->in_room == NULL)"
HYBRID_GATE="if (!hybrid_power_parent(ch, CLASS_MONK) || ch->pcdata == NULL || ch->in_room == NULL)"
NATIVE_DAWN="""            for ( d = descriptor_list; d != NULL; d = d->next )
            {
               if ( d->connected == CON_PLAYING
                  && ( ch = d->character) != NULL
                  &&  !IS_NPC(ch)
                  &&  !IS_EXTRA(ch, EXTRA_OSWITCH)
                  &&  !IS_MORE(ch, MORE_HEAD)
\t\t  &&  IS_MORE(ch, MORE_FAVOUR))
\t       {
\t\t  REMOVE_BIT(ch->more, MORE_FAVOUR);
\t\t  stc("You feel weary as the blessing of the almighty fades from you.\\n\\r", ch);
\t       }
            }"""

def blob(text):
    from werewolf_source import without_werewolf_stage
    text=without_werewolf_stage(text)
    b=text.encode()
    return hashlib.sha1(b"blob "+str(len(b)).encode()+b"\0"+b).hexdigest()

def replace_once(text,old,new):
    assert text.count(old)==1,repr(old)
    return text.replace(old,new,1)

def without_monk_buff_stage(text):
    text=without_favour_metadata_monk(text)
    start=text.index("void do_almighty(")
    helper=text.index("void monk_favour_sunrise(void)",start)
    end=text.index("void do_celestial(",helper)
    body=text[start:helper]
    body=replace_once(body,"    if (ch == NULL) return;\n","")
    gate=HYBRID_GATE if HYBRID_GATE in body else NATIVE_GATE
    body=replace_once(body,gate,"if (!IS_MONK(ch))")
    body=replace_once(body,
        "if (arg[0] != '\\0' && (victim = get_char_room(ch, arg)) == NULL)",
        "if ((victim = get_char_room(ch, arg)) == NULL)")
    if "hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_MANTRA) < 5" in body:
        body=replace_once(body,
            "hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_MANTRA) < 5",
            "get_monk(ch, MONK_MANTRA) < 5")
    text=text[:start]+body+text[end:]
    start=text.index("void do_lifecloak(")
    end=text.index("void do_mclan(",start)
    body=text[start:end]
    body=replace_once(body,"    if (ch == NULL) return;\n","")
    gate=HYBRID_GATE if HYBRID_GATE in body else NATIVE_GATE
    body=replace_once(body,gate,"if (!IS_MONK(ch))")
    if "hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_MANTRA) < 8" in body:
        body=replace_once(body,
            "hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_MANTRA) < 8",
            "get_monk(ch, MONK_MANTRA) < 8")
    body=replace_once(body,
        "if (ch->max_hit <= 0 || ch->max_mana <= 0 ||\n"
        "        ch->hit != ch->max_hit || ch->mana != ch->max_mana)",
        "if (ch->hit < ch-> max_hit || ch->mana < ch->max_mana)")
    body=replace_once(body,
        "ch->hit = (ch->hit / 5) * 4 + (ch->hit % 5) * 4 / 5;",
        "ch->hit = ch->hit * .8;")
    return text[:start]+body+text[end:]

def without_cloak_death_stage(text):
    text=without_gargoyle_death(text)
    text=replace_once(text,
        "if (victim->hit <= 0 && monk_cloak_can_save(victim))",
        "if (victim->hit <= 0 && IS_MONK(victim) && IS_MORE(victim, MORE_CLOAK))")
    return replace_once(text,"victim->hit = UMAX(1, victim->max_hit / 5);",
                        "\tvictim->hit = victim->max_hit * .2;")

def without_favour_dawn_stage(text):
    from aux_writers_source import without_aux_writers_update
    text=without_aux_writers_update(text)
    return replace_once(text,"            monk_favour_sunrise();",NATIVE_DAWN)

class MonkBuffCatalog(unittest.TestCase):
    def test_only_two_monk_handlers_and_their_owned_helpers(self):
        text=(SRC/"classes/monk/monk.c").read_text()
        self.assertEqual(blob(without_monk_buff_stage(text)),BASE_MONK)

    def test_death_dawn_and_declarations_restore_exact_originals(self):
        self.assertEqual(blob(without_cloak_death_stage((SRC/"combat_death.c").read_text())),BASE_DEATH)
        self.assertEqual(blob(without_favour_dawn_stage((SRC/"update.c").read_text())),BASE_UPDATE)
        text=without_favour_metadata_merc((SRC/"merc.h").read_text())
        self.assertEqual(blob(replace_once(text,
            "void monk_favour_sunrise(void);\nbool monk_cloak_can_save(CHAR_DATA *victim);\n","")),
            BASE_MERC)

    def test_real_engine_and_native_digest_are_required(self):
        test=(ROOT/"tests/test_hybrid_monk_buffs.c").read_text()
        self.assertIn("tests/test_hybrid_monk_buffs",make_values(ROOT,"TESTS"))
        self.assertIn("0x440019fda0994e7f",test)
        self.assertIn("weather_update();",test)
        self.assertIn("update_pos(actors)",test)
        self.assertNotRegex(test,r"\b(?:int|bool|void|long)\s+(?:get_monk|"
                            r"hybrid_power_\w+|do_almighty|do_lifecloak|"
                            r"weather_update|update_pos)\s*\([^;]*\)\s*\{")

    def test_resource_flags_and_accounting_are_unchanged(self):
        text=without_favour_metadata_monk((SRC/"classes/monk/monk.c").read_text())
        for v in ("ch->mana -= 20000;","ch->mana -= 5000;",
                  "WAIT_STATE(ch, 24);","WAIT_STATE(ch, 12);",
                  "SET_BIT(vch->more, MORE_FAVOUR);","SET_BIT(victim->more, MORE_FAVOUR);",
                  "SET_BIT(ch->more, MORE_CLOAK);","ch->mana = 0;"):
            self.assertIn(v,text)
        self.assertIn("if (IS_NPC(vch)) continue;",text)
        self.assertIn("REMOVE_BIT(vch->more, MORE_FAVOUR);",text)

    def test_saved_format_and_forms_unchanged(self):
        compat=json.loads((ROOT/"deploy/update_compat.json").read_text())
        self.assertEqual((compat["save_epoch"],compat["handoff"]),(4,2))
        source=(SRC/"classes/monk/monk.c").read_text()
        self.assertNotIn("hybrid_forms_",source)

    def test_only_effective_mantra_can_activate_or_rescue(self):
        text=(SRC/"classes/monk/monk.c").read_text()
        for name,end,rank in (("almighty","monk_favour_sunrise",5),
                              ("lifecloak","do_mclan",8)):
            body=text.split("void do_"+name+"(",1)[1].split("void "+end+"(",1)[0]
            self.assertIn(HYBRID_GATE,body)
            self.assertIn("hybrid_power_rank(ch, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_MANTRA) < "+str(rank),body)
            self.assertNotIn("get_monk(",body)
            self.assertNotIn("ch->class =",body)
        helper=text.split("bool monk_cloak_can_save(",1)[1].split("void do_celestial(",1)[0]
        self.assertIn("hybrid_power_rank(victim, CLASS_MONK, HYBRID_TRAIN_MONK, MONK_MANTRA) >= 8",helper)
        self.assertIn("REMOVE_BIT(victim->more, MORE_CLOAK);",helper)

    def test_real_matrix_and_switched_cleanup_are_not_mocked(self):
        test=(ROOT/"tests/test_hybrid_monk_buffs.c").read_text()
        for fragment in ("secondary==11664&&original==2916",
                         "focus==MONK_MANTRA?rank:UMIN(rank,4)",
                         "bad<12","descs[0].original=actors",
                         "descs[1].connected=CON_NOTE_TEXT","MORE_HEAD",
                         "get_soak(actors)==soak+100",
                         "get_damcap(actors)==cap+100","actors[0].hit==0"):
            self.assertIn(fragment,test)
        load=(ROOT/"tests/test_hybrid_player.c").read_text()
        self.assertIn("ch->more=MORE_FAVOUR|MORE_CLOAK|MORE_BRISTLES;",load)
        self.assertIn("ch->more==(MORE_FAVOUR|MORE_CLOAK|MORE_BRISTLES)",load)

    def test_only_owned_flags_are_cleared(self):
        text=without_favour_metadata_monk((SRC/"classes/monk/monk.c").read_text())
        sweep=text.split("void monk_favour_sunrise(void)",1)[1].split("bool monk_cloak_can_save(",1)[0]
        self.assertIn("vch=char_list",sweep)
        self.assertEqual(sweep.count("REMOVE_BIT("),1)
        self.assertIn("REMOVE_BIT(vch->more, MORE_FAVOUR);",sweep)
        self.assertNotIn("descriptor_list",sweep)

    def test_usage_and_offline_rollback_limits_are_honest(self):
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        for fragment in ("Secondary Monk persistent buffs","14,580 paired",
                         "180 full/short loads","Offline Favor expiry",
                         "An older binary","Chimera and live form"):
            self.assertIn(fragment,docs)
        help=(SRC/"classes/hybrid/help.c").read_text()
        self.assertIn("Secondary Monk Mantra: almighty (5;",help)
        self.assertIn("lifecloak (8;",help)
        self.assertIn("Favor deadlines are also checked on reload",help)

if __name__=="__main__":
    unittest.main()
