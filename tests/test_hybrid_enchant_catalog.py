"""One Sorcerer access gate plus the explicitly requested claimed-owner policy."""
import hashlib
import re
import unittest
from test_class_layout import ROOT, SRC, make_values
from test_hybrid_vision_catalog import without_sorcerer_vision_adapter

OWNERSHIP_GUARD="""    /* The creator and current carrier are not proof of ownership. Use the
     * existing claim/grant owner, with exact case-insensitive player matching.
     * No automatic claim, metadata rewrite or staff bypass. */
    if (obj->questowner == NULL || str_cmp(obj->questowner, ch->name))
    {
        stc("You can only enchant gear claimed in your own name.\\n\\r", ch);
        stc("Claim unowned gear first with: claim <item>.\\n\\r", ch);
        return;
    }

"""

def without_sorcerer_enchant_adapter(text):
    text=without_sorcerer_vision_adapter(text)
    start=text.index("void do_enchant(")
    end=text.index("\nvoid do_write(",start)
    body=text[start:end];gate="hybrid_power_parent(ch, CLASS_SORCERER)"
    assert body.count(gate)==1 and body.count(OWNERSHIP_GUARD)==1
    body=body.replace(gate,"IS_SORCERER(ch)",1).replace(OWNERSHIP_GUARD,"",1)
    return text[:start]+body+text[end:]

class HybridEnchantCatalog(unittest.TestCase):
    def test_only_access_and_explicit_owner_policy_change(self):
        data=without_sorcerer_enchant_adapter((SRC/"classes/sorcerer/sorcerer.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "0bccd3b62dfec56be720257da9cf5ff00ed7df1f")

    def test_exact_owner_before_affects_no_creator_or_staff_bypass(self):
        body=(SRC/"classes/sorcerer/sorcerer.c").read_text().split(
            "void do_enchant(",1)[1].split("\nvoid do_write(",1)[0]
        self.assertIn(OWNERSHIP_GUARD,body)
        self.assertNotIn("questmaker",body)
        self.assertNotRegex(body,r"\b(?:is_name|IS_IMM|do_claim)\(")
        self.assertLess(body.index("obj->questowner"),body.index('act("You weave'))
        self.assertLess(body.index("obj->questowner"),re.search(r"\bpaf\s*=\s*alloc_perm",body).start())
        self.assertNotRegex(body,r"obj->questowner\s*=(?!=)")
        guard="""    /* The shared cast route must not bypass the enchant command's owner rule. */
    if (obj->questowner == NULL || str_cmp(obj->questowner, ch->name))
    {
        stc("You can only enchant gear claimed in your own name.\\n\\r", ch);
        stc("Claim unowned gear first with: claim <item>.\\n\\r", ch);
        return;
    }

"""
        magic=(SRC/"magic.c").read_text()
        self.assertEqual(magic.count(guard),1)
        data=magic.replace(guard,"",1).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "953e67d4bab0db915e21a75ccc48fa140d9e7de5")

    def test_native_costs_bonus_flags_tier_and_equipment_guard(self):
        body=(SRC/"classes/sorcerer/sorcerer.c").read_text().split(
            "void do_enchant(",1)[1].split("\nvoid do_write(",1)[0]
        for part in ("get_obj_carry(ch, arg)","ITEM_WEAPON","ITEM_ARMOR","QUEST_SORCENCHANT",
                     "QUEST_ARTIFACT","APPLY_HITROLL","APPLY_DAMROLL"):
            self.assertIn(part,body)
        self.assertEqual(len(re.findall(r"paf->modifier\s*=\s*5;",body)),2)
        self.assertEqual(len(re.findall(r"paf->duration\s*=\s*-1;",body)),2)
        self.assertNotRegex(body,r"ch->(?:class|primal|mana|gold|exp|pcdata->\w+)\s*[-+]?=(?!=)")
        self.assertRegex((SRC/"interp.c").read_text(),r'"enchant",\s*do_enchant,\s*POS_RESTING,\s*LEV_SOR')

    def test_real_engine_owned_golden_and_repeated_native_pairs(self):
        self.assertIn("tests/test_hybrid_enchant",make_values(ROOT,"TESTS"))
        self.assertIn("tests/test_hybrid_enchant: tests/test_hybrid_enchant.c $(ENGINE_OBJECTS)",
                      (ROOT/"Makefile").read_text())
        test=(ROOT/"tests/test_hybrid_enchant.c").read_text()
        for part in ("interpret(ch,text)","0x9c4e5177d17c534f","cases==144","comparisons==5184",
                     "save_char_obj_checked(ch)","load_char_short","load_char_obj",
                     "ch->hitroll==22&&ch->damroll==24","get_color(ch,BLUE_MAGIC)==0"):
            self.assertIn(part,test)
        self.assertNotRegex(test,r"\b(?:void|int|bool|long)\s+(?:do_enchant|hybrid_power_parent|"
                            r"interpret|save_char_obj_checked)\s*\([^;]*\)\s*\{")

    def test_claim_owner_creator_staff_and_invalid_record_cases(self):
        test=(ROOT/"tests/test_hybrid_enchant.c").read_text()
        for part in ("cases==152","eNCHfixture","Enchfixture Other","Enchfixturex",
                     'char claim[]="claim gear"',"Otherfixture","LEV_IMP","ACT_IS_NPC",
                     "player_load_failed=1","hybrid_present=2","colors[BLUE_MAGIC]=6",
                     "hybrid_player_save_valid(ch)"):
            self.assertIn(part,test)
        self.assertIn("cases==114",test)
        self.assertIn('spell_enchant_weapon(skill_lookup("enchant weapon"),30,ch,obj)',test)
        claim=(SRC/"act_wiz.c").read_text().split("void do_claim(",1)[1].split("\nvoid ",1)[0]
        self.assertIn("obj->questowner = str_dup(ch->name)",claim)

    def test_help_describes_ownership_and_native_costs(self):
        text=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("claim <item>","enchant <item>","questowner","questmaker",
                     "no staff bypass","+5 hitroll","+5 damroll","ordinary player-save timing",
                     "Existing enchantments are not removed","native object-affect"):
            self.assertIn(part,text)

if __name__=="__main__":
    unittest.main()
