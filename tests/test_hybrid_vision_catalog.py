"""Scoped sight-source changes; all other native calculations remain pinned."""
import hashlib
import re
import unittest
from test_class_layout import ROOT, SRC, make_values
from test_hybrid_monk_buffs_catalog import without_cloak_death_stage
from test_hybrid_stun_catalog import without_hybrid_stun_adapter
from test_hybrid_celestial_catalog import without_celestial_visibility_stage
from item_info_source import without_item_info_stage
from progress_metrics_source import without_progress_metrics_stage

NEW_VISION="""void do_vision( CHAR_DATA *ch, char *argument )
{
    sorcerer_vision_toggle(ch);
}

"""
OLD_VISION="""void do_vision( CHAR_DATA *ch, char *argument )
{
    if (!IS_SORCERER(ch))
    {
\tstc("Huh?\\n\\r", ch);
\treturn;
    }

    if (!IS_SET(ch->act, PLR_HOLYLIGHT))
    {
\tSET_BIT(ch->act, PLR_HOLYLIGHT);
\tSET_BIT(ch->affected_by, AFF_SHADOWSIGHT);
\tSET_BIT(ch->vampaff, VAM_NIGHTSIGHT);
\tstc("You manipulate magical fields about yourself to enhance your vision.\\n\\r", ch);
    }
    else
    {
\tREMOVE_BIT(ch->act, PLR_HOLYLIGHT);
\tREMOVE_BIT(ch->affected_by, AFF_SHADOWSIGHT);
\tREMOVE_BIT(ch->vampaff, VAM_NIGHTSIGHT);
\tstc("You allow your magically enhanced vision to fade.\\n\\r", ch);
    }

    return;
}

"""
def without_sorcerer_vision_adapter(text):
    assert text.count(NEW_VISION)==1
    return text.replace(NEW_VISION,OLD_VISION,1)

def without_hostile_sight_adapter(text):
    new="if (effective_holylight(victim)) strip_holylight(victim);"
    assert text.count(new)==1
    return text.replace(new,"if (IS_SET(victim->act, PLR_HOLYLIGHT)) REMOVE_BIT(victim->act, PLR_HOLYLIGHT);",1)

def without_vision_info_adapter(text):
    text=without_item_info_stage(text)
    for new,old in (("holylight","IS_SET({}->act, PLR_HOLYLIGHT)"),
                    ("shadowsight","IS_AFFECTED({},AFF_SHADOWSIGHT)"),
                    ("nightsight","IS_VAMPAFF({}, VAM_NIGHTSIGHT)")):
        text=re.sub(r"\beffective_"+new+r"\((\w+)\)",lambda m:old.format(m[1]),text)
    return text.replace("if ( IS_AFFECTED(ch, AFF_BLIND) && !IS_AFFECTED(ch,AFF_SHADOWSIGHT))",
                        "if ( IS_AFFECTED(ch, AFF_BLIND) && !IS_AFFECTED(ch, AFF_SHADOWSIGHT))")

NATIVE_READERS={
    "handler.c":"31630afce94a32cd6d092d3e88b307e65edae58978103096d02794acbb86cb71",
    "act_info.c":"9329784bc5c9ee2aaa28a45a586120cceac85a4e7a5953479a29885f597cbfa4",
    "kav_info.c":"7033361ae7f06dee67bb381072d513cc252f5f3125b770a86d74a94e885920bc",
    "random.c":"fd82898394fbd0e08dc6df57c8769f215f488b1feb94c1f26c307412865e64b6",
    "clan.c":"9a416f47b1ef3de879819cd9ba1007a87f053fa7fc5641855e384bfe143d5803",
    "classes/werewolf/wwtribe.c":"4f33bcb209a2b9806b131914cdea03d7cdd9d15c250bae891ac0deb3b47ad42b",
    "classes/highlander/highland.c":"487dfa0e94c7c69dd5ad4c627c3ab8587aafd9c8e29ce9c57763ac785416cad6",
    "classes/dragon/dragon.c":"e9e38fd31b24cf9db0da213a82626f6865a2857735e215f1839ccbe4b8e11ef6",
    "classes/monk/monk.c":"101110633ada4ead4205c5c4574255a21aae9183c1e721ae915b21abd304fd82",
    "classes/elemental/elemental.c":"da8474d2063b020b76afbf96269b889388bacf4c359ddc40c470c58f20232583",
}
def native_reader_tokens(text):
    text=without_progress_metrics_stage(text)
    if "void obj_score(" in text:text=without_item_info_stage(text)
    if "void do_readaura(" in text:
        from aura_source import without_aura_stage
        text=without_aura_stage(text)
    if "bool can_see( CHAR_DATA *ch, CHAR_DATA *victim )" in text:
        text=without_celestial_visibility_stage(text)
    text=text.replace("if (effective_holylight(victim)) strip_holylight(victim);",
                      "if (IS_SET(victim->act, PLR_HOLYLIGHT)) REMOVE_BIT(victim->act, PLR_HOLYLIGHT);")
    for new,old in (("holylight","IS_SET({}->act, PLR_HOLYLIGHT)"),
                    ("shadowsight","IS_AFFECTED({}, AFF_SHADOWSIGHT)"),
                    ("nightsight","IS_VAMPAFF({}, VAM_NIGHTSIGHT)")):
        text=re.sub(r"\beffective_"+new+r"\((\w+)\)",lambda m:old.format(m[1]),text)
    # Ignore layout only, never whitespace inside strings/character literals.
    return re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\s+',
                  lambda m:m[0] if m[0][0] in "\"'" else "",text)

class HybridVisionCatalog(unittest.TestCase):
    def test_one_delegation_and_other_sorcerer_code_byte_identical(self):
        data=without_sorcerer_vision_adapter((SRC/"classes/sorcerer/sorcerer.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "cabd4ae68983589b045df1387153cc4470058189")

    def test_consumers_only_readers_and_hostile_holy_removal_changed(self):
        data=without_vision_info_adapter((SRC/"act_info.c").read_text()).encode()
        self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),
                         "b24546978b3f3cf3a2191c8ec0803383bb89dea1")
        for path,digest in NATIVE_READERS.items():
            with self.subTest(path=path):
                text=(SRC/path).read_text()
                if path=="classes/monk/monk.c":text=without_hybrid_stun_adapter(text)
                self.assertEqual(hashlib.sha256(native_reader_tokens(text).encode()).hexdigest(),digest)

    def test_source_bits_are_independent_and_persist_in_existing_extra(self):
        header=(SRC/"merc.h").read_text()
        for name,value in (("HOLY","268435456"),("SHADOW","536870912"),("NIGHT","1073741824")):
            self.assertRegex(header,r"#define EXTRA_SORC_"+name+r"\s+"+value+r"L")
        code=(SRC/"classes/sorcerer/vision.c").read_text()
        toggle=code.split("void sorcerer_vision_toggle(",1)[1].split("void strip_holylight(",1)[0]
        self.assertIn("hybrid_power_parent(ch, CLASS_SORCERER)",toggle)
        self.assertNotRegex(toggle,r"(?:SET_BIT|REMOVE_BIT)\(ch->(?:act|affected_by|vampaff)")
        self.assertNotRegex(toggle,r"ch->(?:class|mana|primal|gold|exp|pcdata->\w+)\s*[-+]?=(?!=)")
        self.assertIn("ch->extra & EXTRA_SORC_VISION",code)
        self.assertIn('ch->extra & ~EXTRA_SWITCH',(SRC/"save.c").read_text())
        self.assertRegex((SRC/"clearclass.c").read_text(),r"victim->extra\s*=\s*131072")

    def test_legacy_provider_controls_still_use_raw_flags(self):
        clan=(SRC/"clan.c").read_text()
        nightsight=clan.split("void do_nightsight(",1)[1].split("\nvoid ",1)[0]
        self.assertIn("IS_VAMPAFF(ch, VAM_NIGHTSIGHT)",nightsight)
        self.assertNotIn("effective_nightsight",nightsight)
        for path in ("more.c","classes/vampire/disc.c","classes/werewolf/instincts.c","classes/mage/mage.c"):
            self.assertNotIn("effective_",(SRC/path).read_text())

    def test_real_engine_matrix_roundtrips_and_no_permission_stubs(self):
        self.assertIn("tests/test_hybrid_vision",make_values(ROOT,"TESTS"))
        self.assertIn("tests/test_hybrid_vision: tests/test_hybrid_vision.c $(ENGINE_OBJECTS)",
                      (ROOT/"Makefile").read_text())
        test=(ROOT/"tests/test_hybrid_vision.c").read_text()
        for part in ("cases==1216","pairs==76","save_char_obj_checked(ch)","load_char_short",
                     "can_see(ch,&target)","can_see_obj(ch,obj)","check_blind(ch)",
                     "clearclass Visfixture","player_load_failed=1","BLINDFOLDED","EXTRA_SORC_HOLY"):
            self.assertIn(part,test)
        self.assertNotRegex(test,r"\b(?:int|bool|void|long)\s+(?:can_see|hybrid_power_parent|"
                            r"save_char_obj_checked|sorcerer_vision_toggle)\s*\([^;]*\)\s*\{")

    def test_docs_are_explicit_about_unattributed_legacy_flags(self):
        docs=(ROOT/"docs/HYBRIDS.md").read_text()
        for part in ("Vision ownership","legacy raw flags","not guessed or cleared",
                     "ordinary player-save timing","no new fee","Extra","Record4/save epoch3/handoff2"):
            self.assertIn(part,docs)

    def test_broad_resets_only_add_shadow_source_cleanup(self):
        for path,sha,count in (("combat_death.c","72ef15ef3b963af036544d58777669b686ec752a",2),
                               ("act_wiz.c","3f8804e378d7e184e024378964aa4fb87b848b39",2)):
            text=(SRC/path).read_text()
            text=without_progress_metrics_stage(text)
            if path=="combat_death.c":text=without_cloak_death_stage(text)
            native,n=re.subn(r"^    strip_shadowsight\((?:ch|victim)\);\n","",text,flags=re.M)
            self.assertEqual(n,count)
            data=native.encode()
            self.assertEqual(hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest(),sha)
        test=(ROOT/"tests/test_hybrid_vision.c").read_text()
        self.assertIn("strip_shadowsight(ch)",test)
        self.assertIn("effective(ch)==5",test)

if __name__=="__main__":
    unittest.main()
