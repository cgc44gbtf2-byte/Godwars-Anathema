"""Guard the limited live adapter boundary; real behavior is tested in C."""
import re
import unittest
from test_class_layout import ROOT, SRC, make_values


class HybridAccessCatalog(unittest.TestCase):
    def test_class_predicates_remain_original_only(self):
        text=(SRC/"merc.h").read_text()
        for name in ("VAMPIRE","WEREWOLF","SORCERER","HIGHLANDER","DROW",
                     "NINJA","DRAGON","MAGE","MONK","ELEM"):
            self.assertRegex(text,rf"#define IS_{name}\(ch\)\s*\(\(ch\)->class == CLASS_{name} && !IS_NPC\(ch\)\)")

    def test_all_eight_numeric_hooks_have_one_owner(self):
        owners={
            "combat_initiative.c":("SPEED",),
            "combat_defense.c":("SOAK","AVOID","BREAK"),
            "combat_damage.c":("DAMCAP",),
            "act_info.c":("HITROLL","DAMROLL","ARMOR"),
        }
        for owner,metrics in owners.items():
            text=(SRC/owner).read_text()
            for metric in metrics:
                self.assertEqual(text.count("HYBRID_PASSIVE_"+metric),1)
        self.assertEqual((SRC/"act_info.c").read_text().count("|| hybrid_secondary_unwounded(ch)"),3)

    def test_adapters_never_swap_primary_or_write_flags(self):
        for name in ("access.c","passives.c"):
            text=(SRC/"classes/hybrid"/name).read_text()
            self.assertNotRegex(text,r"->(?:class|pcdata|polyaff|vampaff|itemaffect|wolf|celerity)\s*=(?!=)")
            self.assertNotRegex(text,r"\b(?:SET_BIT|REMOVE_BIT|WAIT_STATE|hurt_person|one_hit|number_range|number_percent)\s*\(")
        access=(SRC/"classes/hybrid/access.c").read_text()
        self.assertIn("hybrid_parent_rank_read",access)
        self.assertIn("hybrid_player_save_valid",access)
        self.assertIn("!s->secondary_build",access)

    def test_conversion_and_other_handlers_stay_unwired(self):
        preview=(SRC/"classes/hybrid/selfbrid.c").read_text()
        self.assertNotIn("hybrid_player_attach(",preview)
        self.assertNotIn("hybrid_training_commit(",preview)
        highland=(SRC/"classes/highlander/highland.c").read_text()
        self.assertEqual(highland.count("if (!hybrid_power_parent(ch,CLASS_HIGHLANDER))"),3)
        self.assertIn("if (!IS_HIGHLANDER(ch))",highland) # forge/timers not widened

    def test_native_regression_is_in_default_build(self):
        self.assertEqual(make_values(ROOT,"TESTS").count("tests/test_hybrid_access"),1)
        selected=make_values(ROOT,"ENGINE_OBJECTS")
        for name in ("access","passives"):
            self.assertEqual(selected.count(f"tests/stun_engine/classes/hybrid/{name}.o"),1)

    def test_no_new_save_or_handoff_format(self):
        text=(SRC/"classes/hybrid/state.c").read_text()
        self.assertIn('AnathemaHybrid 4 ',text)
        import json
        compat=json.loads((ROOT/"deploy/update_compat.json").read_text())
        self.assertEqual(compat["save_epoch"],4)
        self.assertEqual(compat["handoff"],2)


if __name__=="__main__":
    unittest.main()
